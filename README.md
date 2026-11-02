# Sensor Monitor

A C++17 command-line program that calculates the arithmetic mean of each sensor
type in a text file.

## Build and test

Run these commands from the repository root on Linux. Building the program
requires a C++17 compiler and CMake 3.16 or newer. No particular processor or
Linux distribution is required.

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -DSENSOR_STRICT=ON -DBUILD_TESTING=OFF
cmake --build build/release --parallel
build/release/sensor_monitor data/sensor_data.txt
```

The Release configuration enables optimization. Strict mode treats GCC and
Clang warnings as errors. Four-space indentation and
same-line braces follow the original source style. Code contains no comments;
design explanations live in the documentation.

The supplied fixture produces:

```text
HUMIDITY avg: 46.5
PRESSURE avg: 100.6333333
TEMP avg: 23.05
```

To run the tests, install GoogleTest and Python 3 through your package manager,
then configure a separate build:

```sh
cmake -S . -B build/tests -DCMAKE_BUILD_TYPE=Release -DSENSOR_STRICT=ON -DBUILD_TESTING=ON
cmake --build build/tests --parallel
ctest --test-dir build/tests --output-on-failure
```

## Input and errors

Each nonblank line contains exactly `SENSOR_TYPE VALUE`, separated by whitespace.
Types are arbitrary, case-sensitive tokens. Values are finite `double` values in
decimal or scientific notation, with an optional sign. Blank lines are ignored,
including the blank line in the supplied fixture. CRLF and a final line without
a newline are accepted.

Extra fields, missing fields, trailing numeric junk, hexadecimal numbers,
nonfinite values, and conversion overflow/underflow are rejected. The first
invalid line fails the whole operation with the filename and physical line
number on stderr. No partial averages are printed. The assessment leaves this
policy unspecified; rejecting incomplete data avoids silently misleading means.

Empty or whitespace-only input succeeds without output. Exactly one file argument
is required. Exit status is `0` on success, `1` for data or I/O errors, and `2` for
usage errors. Results are sorted by type and printed with ten significant digits,
using a decimal point independently of locale.

## Design

`readReadings(std::istream&)` returns value-owned readings. `calculateAverages`
returns a new map on every call, removing raw-pointer ownership and stale state.
The application layer owns file access, formatting, diagnostics, and exit status;
`main` only forwards arguments and standard streams.

Readings are retained in memory to keep this small assessment straightforward.
For `n` readings and `k` types, storage is O(n + k) and grouping takes O(n log k).
Each sensor type uses a power-of-two scale based on its largest absolute reading.
Compensated addition retains small terms lost by ordinary addition. Dividing
before restoring the scale avoids overflowing a sum of extreme readings, using
standard `double` arithmetic without assuming an extra-wide numeric type.
Power-of-two scaling preserves representable differences during cancellation.
This is floating-point arithmetic, not exact decimal arithmetic; extremely small
contributions relative to the scale can still underflow.

## Additional quality checks

Quality tooling is optional. Install GCC, Ninja, Clang/LLVM 19, Python 3.10+ with
virtual-environment support, and Mull 0.36.0 with its matching LLVM 19 compiler
plugin. Follow the [Mull installation instructions](https://mull-project.com/getting-started/installation/)
for your system. The bootstrap checks prerequisites and installs pinned Python
tools in `build/linux-tools/venv`.

The scripts default to `g++` and `clang++-19`; set `CXX` and `CLANGXX` to override
those commands. The mutation plugin is discovered in standard library locations;
set `MULL_PLUGIN` to its file path if installed elsewhere. Mutation testing
discovers the compiler's standard-library directory automatically.

```sh
bash tools/bootstrap-quality.sh
bash tools/run-checks.sh sanitizers
bash tools/run-checks.sh coverage
bash tools/run-checks.sh mutation
bash tools/run-checks.sh release-clang
bash tools/run-checks.sh release
```

Coverage uses GCC/gcov and gcovr; mutation testing uses Clang 19 and Mull 0.36.0.
Separate build directories prevent instrumentation from mixing. The coverage
command also tests the CRAP reporting adapter against known formula values and
missing/ambiguous coverage records.

Reports are written to:

- `build/linux-coverage-gcc/coverage.html`, `coverage.json`, and `summary.json`.
- `build/linux-coverage-gcc/crap.md` and `crap.json`.
- `build/linux-mutation/reports/mutations.json`, with repository-relative paths.
- `test-results.txt` inside each build directory.

Behavioral tests, sanitizer findings, and compiler warnings fail their checks.
Coverage requires 100% of reported production lines, functions, and branches;
compiler-generated unreachable and exception-throw branches are filtered.
Mutation analysis requires all generated mutants to be killed and retains the
report for inspection. No production source lines or individual mutants are
manually suppressed. CRAP uses per-function line coverage and is explicitly an
approximation of the original basis-path metric.

See [quality results](docs/quality/REPORT.md), [assessment results](RESULTS.md),
and the [ring-buffer explanation](technical_writing/RING_BUFFER.md).


