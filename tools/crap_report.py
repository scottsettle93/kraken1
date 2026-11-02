import argparse
import json
from pathlib import Path

import lizard


def crap_score(complexity, coverage):
    if complexity < 1 or not 0 <= coverage <= 1:
        raise ValueError("complexity must be positive and coverage must be a fraction")
    return complexity ** 2 * (1 - coverage) ** 3 + complexity


def measure_functions(root, coverage):
    root = Path(root).resolve()
    if coverage.get("gcovr/format_version") != "0.15":
        raise ValueError("expected gcovr JSON format 0.15")
    covered_files = {}
    for record in coverage["files"]:
        path = (root / record["file"]).resolve()
        if path in covered_files:
            raise ValueError(f"duplicate coverage file: {path}")
        covered_files[path] = record
    results = []
    for source in sorted((root / "src").rglob("*")):
        if source.suffix not in {".cpp", ".h"}:
            continue
        functions = lizard.analyze_file(str(source)).function_list
        for left, right in zip(functions, functions[1:]):
            if left.end_line >= right.start_line:
                raise ValueError(f"overlapping function ranges in {source}")
        for function in functions:
            record = covered_files.get(source.resolve())
            if record is None:
                raise ValueError(f"missing coverage for {source}")
            matches = [
                item for item in record["functions"]
                if function.start_line <= item["lineno"] <= function.end_line
                and not item.get("gcovr/excluded", False)
            ]
            if len(matches) != 1:
                raise ValueError(f"missing or ambiguous coverage for {function.name}")
            name = matches[0]["name"]
            lines = [
                line for line in record["lines"]
                if line.get("function_name") == name
                and function.start_line <= line["line_number"] <= function.end_line
                and not line.get("gcovr/excluded", False)
            ]
            if not lines or len({line["line_number"] for line in lines}) != len(lines):
                raise ValueError(f"missing or duplicate executable lines for {function.name}")
            line_covered = sum(line["count"] > 0 for line in lines)
            fraction = line_covered / len(lines)
            branches = [
                branch for line in lines for branch in line.get("branches", [])
                if not branch.get("gcovr/excluded", False)
            ]
            results.append({
                "file": str(source.relative_to(root)),
                "function": function.name,
                "start_line": function.start_line,
                "complexity": function.cyclomatic_complexity,
                "lines_covered": line_covered,
                "lines_total": len(lines),
                "line_coverage": fraction,
                "branches_covered": sum(branch["count"] > 0 for branch in branches),
                "branches_total": len(branches),
                "crap_line": crap_score(function.cyclomatic_complexity, fraction),
            })
    if not results:
        raise ValueError("no measured production functions")
    return results


def markdown_report(functions):
    lines = [
        "# Per-function line-based CRAP",
        "",
        "CRAP = CC² × (1 − per-function line coverage)³ + CC.",
        "This uses line coverage, not the original basis-path coverage definition.",
        "Complexity is Lizard's source-level estimate. No missing measurement is assumed covered.",
        "",
        "| Function | Source | CC | Lines | Branches | CRAP (line) |",
        "| --- | --- | ---: | ---: | ---: | ---: |",
    ]
    for item in functions:
        lines.append(
            f"| {item['function']} | {item['file']}:{item['start_line']} "
            f"| {item['complexity']} | {item['lines_covered']}/{item['lines_total']} "
            f"| {item['branches_covered']}/{item['branches_total']} "
            f"| {item['crap_line']:.3f} |"
        )
    lines.extend([
        "",
        "The historical warning threshold of 30 is a heuristic, not a correctness guarantee.",
        "",
    ])
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path("."))
    parser.add_argument("--coverage", type=Path, required=True)
    parser.add_argument("--output-json", type=Path, required=True)
    parser.add_argument("--output-markdown", type=Path, required=True)
    args = parser.parse_args()
    functions = measure_functions(args.root, json.loads(args.coverage.read_text()))
    report = {
        "metric": "CRAP using per-function line coverage",
        "coverage_format": "gcovr 0.15",
        "functions": functions,
        "max_crap_line": max(item["crap_line"] for item in functions),
    }
    args.output_json.write_text(json.dumps(report, indent=2) + "\n")
    args.output_markdown.write_text(markdown_report(functions))
    print(f"Measured {len(functions)} functions; maximum CRAP (line): {report['max_crap_line']:.3f}")


if __name__ == "__main__":
    main()
