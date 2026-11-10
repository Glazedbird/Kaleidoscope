"""Verify IR with LLVM, link it, execute it, and exercise semantic recovery."""
import pathlib
import subprocess
import sys
import tempfile
import unittest

BINARY = str(pathlib.Path(sys.argv.pop(1)).resolve())
LLVM_BIN = pathlib.Path(sys.argv.pop(1))

def run(source):
    return subprocess.run([BINARY, '--emit-ir'], input=source, text=True,
                          capture_output=True, timeout=5)

class CodegenTests(unittest.TestCase):
    def verify(self, source):
        result = run(source)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, '')
        checked = subprocess.run([str(LLVM_BIN / 'llvm-as'), '-o', '/dev/null'],
                                 input=result.stdout, text=True,
                                 capture_output=True, timeout=5)
        self.assertEqual(checked.returncode, 0, checked.stderr)
        return result.stdout

    def test_module_and_execution(self):
        ir = self.verify('extern add(a b); def add(x y) x+y; '
                         'def less(x y) x<y; def calc(x) add(x*2, 3)-1; '
                         'calc(4); 1+2;')
        self.assertIn('@__anon_expr.0', ir)
        self.assertIn('@__anon_expr.1', ir)
        self.assertIn('fcmp ult', ir)
        self.assertIn('uitofp', ir)
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory)
            (path / 'module.ll').write_text(ir)
            (path / 'driver.c').write_text('''extern double calc(double);
extern double less(double, double);
int main(void) { return !(calc(4) == 10 && less(1, 2) == 1 && less(2, 1) == 0); }
''')
            linked = subprocess.run([str(LLVM_BIN / 'clang'), str(path / 'module.ll'),
                                     str(path / 'driver.c'), '-o', str(path / 'run')],
                                    text=True, capture_output=True, timeout=15)
            self.assertEqual(linked.returncode, 0, linked.stderr)
            self.assertEqual(subprocess.run([str(path / 'run')], timeout=5).returncode, 0)

    def test_declarations_recursion_and_empty_module(self):
        for source in ['', 'extern f(x); extern f(y); def f(z) z; extern f(a);',
                       'def recurse(x) recurse(x);', '1; 2; 3;']:
            with self.subTest(source=source): self.verify(source)

    def test_errors(self):
        cases = [('x;', 'Unknown variable'), ('missing(1);', 'Unknown function'),
                 ('extern f(x); f();', 'Incorrect # arguments'),
                 ('extern f(x); def f(x y) x;', 'Conflicting function'),
                 ('def f(x) x; def f(y) y;', 'cannot be redefined'),
                 ('def f(x x) x;', 'Duplicate argument'),
                 ('extern f(x x);', 'Duplicate argument'),
                 ('1 + ;', 'expecting an expression')]
        for source, diagnostic in cases:
            with self.subTest(source=source):
                result = run(source + 'def recovered() 42; recovered();')
                self.assertEqual(result.returncode, 1, result.stderr)
                self.assertIn(diagnostic, result.stderr)
                self.assertEqual(result.stdout, '')
                self.assertEqual(result.stderr.count('Error:'), 1)

    def test_failed_definition_preserves_declaration_and_callers(self):
        result = run('extern f(a); def caller(x) f(x); '
                     'def f(b) unknown; def f(c) c+1; caller(2);')
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stderr.count('Error:'), 1, result.stderr)
        self.assertIn('Unknown variable', result.stderr)
        self.assertEqual(result.stdout, '')

    def test_failed_new_definition_can_be_retried(self):
        result = run('def f(x) unknown; def f(y) y; f(2);')
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stderr.count('Error:'), 1, result.stderr)

if __name__ == '__main__': unittest.main()
