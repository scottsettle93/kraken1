import copy
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from crap_report import crap_score, measure_functions


class CrapReportTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = pathlib.Path(self.directory.name)
        (self.root / "src").mkdir()
        (self.root / "src/example.cpp").write_text(
            "int choose(int x) {\n"
            "    if (x > 0) {\n"
            "        return 1;\n"
            "    }\n"
            "    return 0;\n"
            "}\n"
        )
        self.coverage = {
            "gcovr/format_version": "0.15",
            "files": [{
                "file": "src/example.cpp",
                "functions": [{"name": "choose", "lineno": 1}],
                "lines": [
                    {"line_number": 2, "function_name": "choose", "count": 1},
                    {"line_number": 3, "function_name": "choose", "count": 1},
                    {"line_number": 5, "function_name": "choose", "count": 1},
                ],
            }],
        }

    def test_known_formula_values(self):
        self.assertEqual(crap_score(1, 0), 2)
        self.assertEqual(crap_score(5, 0), 30)
        self.assertEqual(crap_score(10, 0.5), 22.5)
        self.assertEqual(crap_score(10, 1), 10)
        for complexity, fraction in [(0, 0.5), (2, -0.1), (2, 1.1)]:
            with self.subTest(complexity=complexity, fraction=fraction):
                with self.assertRaises(ValueError):
                    crap_score(complexity, fraction)

    def test_fully_covered_function(self):
        report = measure_functions(self.root, self.coverage)
        self.assertEqual(len(report), 1)
        self.assertEqual(report[0]["complexity"], 2)
        self.assertEqual(report[0]["lines_total"], 3)
        self.assertEqual(report[0]["crap_line"], 2)

    def test_unexecuted_function_is_zero_coverage(self):
        for line in self.coverage["files"][0]["lines"]:
            line["count"] = 0
        result = measure_functions(self.root, self.coverage)[0]
        self.assertEqual(result["line_coverage"], 0)
        self.assertEqual(result["crap_line"], 6)

    def test_partial_coverage_is_local_to_function(self):
        self.coverage["files"][0]["lines"][0]["count"] = 0
        result = measure_functions(self.root, self.coverage)[0]
        self.assertAlmostEqual(result["line_coverage"], 2 / 3)
        self.assertAlmostEqual(result["crap_line"], 2 + 4 / 27)

    def test_missing_file_or_function_is_an_error(self):
        for coverage in [
            {"gcovr/format_version": "0.15", "files": []},
            copy.deepcopy(self.coverage),
        ]:
            if coverage["files"]:
                coverage["files"][0]["functions"] = []
            with self.subTest(coverage=coverage):
                with self.assertRaises(ValueError):
                    measure_functions(self.root, coverage)

    def test_ambiguous_function_mapping_is_an_error(self):
        self.coverage["files"][0]["functions"].append({"name": "other", "lineno": 2})
        with self.assertRaises(ValueError):
            measure_functions(self.root, self.coverage)

    def test_unknown_schema_is_an_error(self):
        self.coverage["gcovr/format_version"] = "future"
        with self.assertRaises(ValueError):
            measure_functions(self.root, self.coverage)

    def test_duplicate_files_or_lines_are_errors(self):
        duplicate_files = copy.deepcopy(self.coverage)
        duplicate_files["files"].append(copy.deepcopy(duplicate_files["files"][0]))
        duplicate_lines = copy.deepcopy(self.coverage)
        duplicate_lines["files"][0]["lines"].append(
            copy.deepcopy(duplicate_lines["files"][0]["lines"][0])
        )
        for coverage in [duplicate_files, duplicate_lines]:
            with self.subTest(coverage=coverage):
                with self.assertRaises(ValueError):
                    measure_functions(self.root, coverage)

    def test_excluded_lines_are_not_assumed_covered(self):
        self.coverage["files"][0]["lines"][0]["gcovr/excluded"] = True
        self.coverage["files"][0]["lines"][1]["count"] = 0
        result = measure_functions(self.root, self.coverage)[0]
        self.assertEqual(result["lines_total"], 2)
        self.assertEqual(result["line_coverage"], 0.5)

    def test_missing_executable_lines_are_an_error(self):
        self.coverage["files"][0]["lines"] = []
        with self.assertRaises(ValueError):
            measure_functions(self.root, self.coverage)


if __name__ == "__main__":
    unittest.main()
