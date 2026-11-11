"""Check immediate IR output with stdin still open on a real pseudo-terminal."""
import os
import pty
import select
import subprocess
import sys
import time
import unittest

BINARY = sys.argv.pop(1)


class ReplTests(unittest.TestCase):
    def test_terminal_emits_ir_before_eof_and_recovers(self):
        master, slave = pty.openpty()
        process = subprocess.Popen([BINARY], stdin=slave, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        os.close(slave)
        pending = b''

        def read_until(marker):
            nonlocal pending
            deadline = time.monotonic() + 5
            while marker not in pending:
                remaining = deadline - time.monotonic()
                self.assertGreater(remaining, 0, pending.decode())
                readable, _, _ = select.select([process.stderr], [], [], remaining)
                self.assertTrue(readable, 'Timed out waiting for immediate IR: ' + pending.decode())
                chunk = os.read(process.stderr.fileno(), 65536)
                self.assertTrue(chunk, 'REPL exited unexpectedly: ' + pending.decode())
                pending += chunk
            end = pending.index(marker) + len(marker)
            result, pending = pending[:end], pending[end:]
            return result.decode()

        def submit(source):
            os.write(master, (source + '\n').encode())
            result = read_until(b'ready> ')
            self.assertIsNone(process.poll(), 'REPL must still accept more input')
            return result

        try:
            self.assertEqual(read_until(b'ready> '), 'ready> ')
            result = submit('def test(x) 1+2+x;')
            self.assertIn('Read function definition:\ndefine double @test(double %x)', result)
            self.assertIn('%addtmp = fadd double 3.000000e+00, %x', result)
            self.assertIn('ret double %addtmp', result)
            self.assertIn('Read extern:\ndeclare double @sin(double)',
                          submit('extern sin(x);'))
            self.assertIn('call double @test(double 4.000000e+00)', submit('test(4);'))
            for source, value in [('1+2;', '3.000000e+00'), ('2*3;', '6.000000e+00')]:
                result = submit(source)
                self.assertIn('Read top-level expression:\ndefine double @__anon_expr()', result)
                self.assertIn('ret double ' + value, result)
            result = submit('missing;')
            self.assertIn('Error: Unknown variable name', result)
            self.assertNotIn('Read top-level expression:', result)
            self.assertIn('Read function definition:', submit('def recovered() test(5);'))
            result = submit('1 + ;')
            self.assertIn('Error:', result)
            self.assertIn('ret double 7.000000e+00', submit('7;'))
            os.write(master, b'\x04')  # EOF only after all immediate-output checks.
            self.assertEqual(process.wait(timeout=5), 1)  # Errors remain reflected in exit status.
            self.assertEqual(process.stdout.read(), b'')
            self.assertEqual(process.stderr.read(), b'')  # No duplicate module dump at EOF.
        finally:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=5)
            process.stdout.close()
            process.stderr.close()
            os.close(master)

    def test_pipe_uses_tutorial_output_without_prompts(self):
        for options in ([], ['--repl']):
            with self.subTest(options=options):
                result = subprocess.run([BINARY, *options], input='def test(x) 1+2+x; test(4);',
                                        text=True, capture_output=True, timeout=5)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout, '')
                self.assertIn('Read function definition:', result.stderr)
                self.assertIn('Read top-level expression:', result.stderr)
                self.assertNotIn('ready>', result.stderr)
                self.assertNotIn('ModuleID', result.stderr)


if __name__ == '__main__':
    unittest.main()
