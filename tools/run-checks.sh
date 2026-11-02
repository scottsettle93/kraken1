set -euo pipefail

if [ "$(uname -s)" != Linux ]; then
    printf '%s\n' 'Run checks on Linux.' >&2
    exit 1
fi
cd "$(dirname "$0")/.."
mode=release
if [ "$#" -gt 0 ]; then
    mode=$1
fi

compiler=${CXX:-g++}
configuration=Debug
sanitizers=OFF
coverage=OFF
mutation=OFF
build_dir="build/linux-$mode"
case "$mode" in
    release)
        configuration=Release
        ;;
    release-clang)
        compiler=${CLANGXX:-clang++-19}
        configuration=Release
        ;;
    sanitizers)
        compiler=${CLANGXX:-clang++-19}
        sanitizers=ON
        export ASAN_OPTIONS=detect_leaks=1
        export UBSAN_OPTIONS=halt_on_error=1
        ;;
    coverage)
        coverage=ON
        build_dir=build/linux-coverage-gcc
        ;;
    mutation)
        compiler=${CLANGXX:-clang++-19}
        mutation=ON
        export MULL_CONFIG="$PWD/.mull.yml"
        export LLVM_PROFILE_FILE="$PWD/$build_dir/%m-%p.profraw"
        ;;
    *)
        printf '%s\n' 'Usage: bash tools/run-checks.sh {release|release-clang|sanitizers|coverage|mutation}' >&2
        exit 2
        ;;
esac

cmake_options=()
if [ -n "${MULL_PLUGIN:-}" ]; then
    cmake_options+=("-DMULL_PLUGIN=$MULL_PLUGIN")
fi
cmake -S . -B "$build_dir" -G Ninja "${cmake_options[@]}" \
    -DCMAKE_CXX_COMPILER="$compiler" -DCMAKE_BUILD_TYPE="$configuration" \
    -DSENSOR_STRICT=ON -DSENSOR_SANITIZERS="$sanitizers" \
    -DSENSOR_COVERAGE="$coverage" -DSENSOR_MUTATION="$mutation"
cmake --build "$build_dir" --parallel 4 --clean-first
if [ "$mode" = coverage ]; then
    find "$build_dir" -name '*.gcda' -delete
fi
ctest --test-dir "$build_dir" --output-on-failure | tee "$build_dir/test-results.txt"

if [ "$mode" = coverage ]; then
    build/linux-tools/venv/bin/python -m unittest discover -s tools/tests -v
    build/linux-tools/venv/bin/gcovr --root . --filter src/ \
        --exclude-unreachable-branches --exclude-throw-branches \
        --fail-under-line 100 --fail-under-function 100 --fail-under-branch 100 \
        --json "$build_dir/coverage.json" --json-summary "$build_dir/summary.json" \
        --html-details "$build_dir/coverage.html" --print-summary "$build_dir"
    build/linux-tools/venv/bin/python tools/crap_report.py \
        --coverage "$build_dir/coverage.json" \
        --output-json "$build_dir/crap.json" --output-markdown "$build_dir/crap.md"
fi

if [ "$mode" = mutation ]; then
    mkdir -p "$build_dir/reports"
    library_path=$(dirname "$(realpath "$("$compiler" -print-file-name=libstdc++.so)")")
    mull-runner --strict --ld-search-path "$library_path" \
        --mutation-score-threshold 100 --include-not-covered --workers 4 --timeout 2000 \
        --reporters Elements --report-dir "$build_dir/reports" \
        --report-name mutations "$build_dir/sensor_tests" \
        2>&1 | tee "$build_dir/mutation.log"
    python3 - "$PWD" "$build_dir/reports/mutations.json" <<'PY'
import json
import sys
from pathlib import Path

root = Path(sys.argv[1]).resolve()
path = Path(sys.argv[2])
report = json.loads(path.read_text())
files = {}
for filename, details in report["files"].items():
    relative = Path(filename).resolve().relative_to(root).as_posix()
    for mutant in details["mutants"]:
        mutant["id"] = mutant["id"].replace(filename, relative)
    files[relative] = details
report["files"] = files
path.write_text(json.dumps(report, indent=2) + "\n")
PY
fi
