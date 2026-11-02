import pathlib
import subprocess
import sys
import tempfile
import unittest

BINARY = pathlib.Path(sys.argv.pop(1)).resolve()
FIXTURE = pathlib.Path(sys.argv.pop(1)).resolve()


class CommandLineTests(unittest.TestCase):
    def run_monitor(self, *arguments):
        return subprocess.run(
            [str(BINARY), *map(str, arguments)],
            capture_output=True,
            text=True,
            timeout=5,
        )

    def test_supplied_fixture(self):
        result = self.run_monitor(FIXTURE)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(
            result.stdout,
            "HUMIDITY avg: 46.5\nPRESSURE avg: 100.6333333\nTEMP avg: 23.05\n",
        )
        self.assertEqual(result.stderr, "")

    def test_usage_errors(self):
        for arguments in [(), ("one", "two")]:
            with self.subTest(arguments=arguments):
                result = self.run_monitor(*arguments)
                self.assertEqual(result.returncode, 2)
                self.assertEqual(result.stdout, "")
                self.assertIn("Usage:", result.stderr)

    def test_missing_file_and_directory(self):
        with tempfile.TemporaryDirectory() as directory:
            for path in [pathlib.Path(directory) / "absent", pathlib.Path(directory)]:
                with self.subTest(path=path):
                    result = self.run_monitor(path)
                    self.assertEqual(result.returncode, 1)
                    self.assertEqual(result.stdout, "")
                    self.assertIn(str(path), result.stderr)

    def test_empty_file(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "empty.txt"
            path.write_text("", encoding="utf-8")
            result = self.run_monitor(path)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stdout, "")
            self.assertEqual(result.stderr, "")

    def test_invalid_record_does_not_print_partial_averages(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "input with spaces.txt"
            path.write_text("A 1\n\nB 2 trailing\n", encoding="utf-8")
            result = self.run_monitor(path)
            self.assertEqual(result.returncode, 1)
            self.assertEqual(result.stdout, "")
            self.assertIn(f"{path}: line 3:", result.stderr)


if __name__ == "__main__":
    unittest.main()
