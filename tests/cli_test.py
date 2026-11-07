"""Exercise real stdin, diagnostics, exit status, and recovery without hanging."""

import subprocess
import sys
import unittest


BINARY = sys.argv.pop(1)


def run(source, *args):
    return subprocess.run(
        [BINARY, *args], input=source, text=True, capture_output=True, timeout=3
    )


class CliTests(unittest.TestCase):
    def test_valid_input(self):
        result = run("# comment\n; def add(x y) x+y; extern sin(x); add(12, 3.5);")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        self.assertEqual(result.stdout.splitlines(), [
            "Parsed a function definition: add",
            "Parsed an extern: sin",
            "Parsed a top-level expression",
        ])

    def test_empty_input(self):
        for source in ("", "  \n", "# comment at EOF", ";;;\n# comment\n"):
            with self.subTest(source=source):
                result = run(source)
                self.assertEqual((result.returncode, result.stdout, result.stderr),
                                 (0, "", ""))

    def test_errors_and_recovery(self):
        cases = [
            ("1 +", "unknown token when expecting an expression"),
            ("(1 + 2", "expected ')'"),
            ("foo(1 2)", "expected ')' or ',' in argument list"),
            ("foo(1,)", "unknown token when expecting an expression"),
            ("def (x) x", "expected function name in prototype"),
            ("extern foo x", "expected '(' in prototype"),
            ("extern foo(x, y)", "expected ')' in prototype"),
            ("@", "unknown token when expecting an expression"),
        ]
        for source, diagnostic in cases:
            with self.subTest(source=source):
                result = run(source + "; def recovered() 1; recovered();")
                self.assertEqual(result.returncode, 1)
                self.assertIn("Error: " + diagnostic, result.stderr)
                self.assertEqual(result.stdout.splitlines(), [
                    "Parsed a function definition: recovered",
                    "Parsed a top-level expression",
                ])

    def test_eof_during_parsing(self):
        for source in ("def", "extern", "foo(", "(1", "1 +", "def foo()"):
            with self.subTest(source=source):
                result = run(source)
                self.assertEqual(result.returncode, 1)
                self.assertIn("Error:", result.stderr)

    def test_recovery_at_next_declaration(self):
        for statement, output in (
            ("def recovered() 1", "Parsed a function definition: recovered\n"),
            ("extern recovered()", "Parsed an extern: recovered\n"),
        ):
            with self.subTest(statement=statement):
                result = run("@ " + statement)
                self.assertEqual(result.returncode, 1)
                self.assertIn("Error:", result.stderr)
                self.assertEqual(result.stdout, output)

    def test_tokens_mode(self):
        result = run("def foo(x) x + 1; # comment", "--tokens")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.splitlines(), [
            "tok_def", "tok_identifier: foo", "character: '(' (40)",
            "tok_identifier: x", "character: ')' (41)", "tok_identifier: x",
            "character: '+' (43)", "tok_number: 1", "character: ';' (59)",
            "tok_eof",
        ])

    def test_invalid_option(self):
        result = run("", "--unknown")
        self.assertEqual(result.returncode, 1)
        self.assertIn("Usage:", result.stderr)


if __name__ == "__main__":
    unittest.main()
