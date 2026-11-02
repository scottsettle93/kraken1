# Quality verification

Verified on Linux.
Build outputs use separate Linux directories. No release test relies on C/C++
`assert` remaining enabled.

## Results

| Check | Result |
| --- | --- |
| GCC 14.2 strict Release (`-O3 -DNDEBUG -Werror`) | 42/42 CTest entries pass |
| Clang 19.1.7 strict Release (`-O3 -DNDEBUG -Werror`) | 42/42 CTest entries pass |
| Clang address, leak, undefined-behavior, and float-cast checks | 42/42 entries pass; no sanitizer findings |
| Program-only strict Release (`BUILD_TESTING=OFF`) | Builds and prints the expected fixture means |
| CRAP adapter regression tests | 10/10 pass |
| Production line coverage | 82/82 = 100% |
| Production function coverage | 5/5 = 100% |
| Production branch coverage | 99/99 = 100% |
| Mull mutations | 20 killed, 0 survived, 0 timeouts; 20/20 = 100% |
| Maximum per-function line-based CRAP | 7.000 |

The 42 CTest entries comprise 41 GoogleTest cases and one CLI integration entry
containing five Python test methods. Tests stay active in optimized builds.
Warnings include `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
-Wformat=2` and are treated as errors.

[Recorded versions](versions.json) identify the compiler and quality-tool
versions. [Source checksums](source-sha256.json) identify the measured source,
tests, build configuration, tools, and fixture.

Separate Codex agents completed the initial [Standards and Spec reviews](review.md),
with zero actionable findings on either axis. The same document records the
subsequent cleanup and portability/privacy review.

## Coverage scope and limitations

GCC/gcov and gcovr 8.8 measure the production `src/` files, including `main`.
Tests and third-party libraries are outside this metric. Compiler-generated
unreachable and exception-throw branches are filtered with gcovr's documented
options; no production source line is explicitly excluded.

All reported production lines, functions, and branches are covered. The coverage
command enforces 100% for each measure; the filtering above is unchanged.

The average calculation uses ordinary double arithmetic. A first pass validates
readings, counts each type, and finds its largest absolute value. A second pass
normalizes readings by a power of two and uses compensated addition. The normalized
mean is bounded by the normalized observed maximum before restoring the scale, keeping extreme finite
averages finite without requiring a wider numeric type. An all-zero group skips
normalization and returns zero. This removes the previous platform restriction.
The compensation follows Algorithms 3.1 and 4.1 in
[Accurate Sum and Dot Product](https://www.tuhh.de/ti3/paper/rump/OgRuOi05.pdf).
It does not provide exact decimal arithmetic; normalization may underflow tiny
relative contributions. Tests include subnormal values and cancellation. A
regression test demonstrates that power-of-two scaling preserves the exact
mean of -0.5 for a cancellation case that arbitrary scaling distorted.

A synthetic batch test passes 16,384 readings through `calculateAverages`,
checking repeated maximum positive values, maximum negative values, and
cancellation after a large intermediate total. Existing tests also check
nonfinite rejection and retention of small terms during cancellation. A custom
output stream throws a nonstandard exception to verify propagation through
`runSensorMonitor`; this exercises the remaining exception-dispatch branch
through the real application interface, without changing production handling.

An initial LLVM-to-gcovr import omitted `main` when multiple binaries were passed
and miscounted closing-brace lines. Direct `llvm-cov report` exposed the mismatch.
The delivered reports therefore use the mature GCC/gcov input path, with
function/file totals cross-checked against all production functions.

See [coverage totals](summary.json) and regenerate the detailed HTML with
`bash tools/run-checks.sh coverage` on Linux.

## Mutation scope and simplification

Mull 0.36.0 instruments the production library, including parsing, calculation,
formatting, argument handling, and file/error handling. Only the trivial `main`
forwarder is outside mutation scope; actual-process integration tests cover it.

The configuration requests default arithmetic/comparison/boundary operators,
arithmetic assignments, logical negation, and experimental conditional negation.
Mull emitted 20 supported mutants for this source/toolchain. Generated mutants,
rather than the configured operator count, define the denominator. Uncovered
mutants are included; none are manually suppressed.

The previous run had an equivalent survivor changing `token.size() > 1` to
`token.size() >= 1`. For a lone `+`, one version rejected the plus and the other
rejected an empty numeric range. Both had identical observable behavior.

The redundant length comparison is now removed. Parsing only receives nonempty
tokens. Reading `token[1]` for a one-character string yields the zero-valued end
character under the [C++17 string access contract](https://timsong-cpp.github.io/cppwp/n4659/string.access).
The existing checks still reject `+`, `++1`, and `+-1`, while accepting valid
positive and negative values. Removing the comparison removed its two generated
mutants: one equivalent and one previously killed. The portable averaging
algorithm generates additional arithmetic mutants. All 20 current mutants are
killed; no mutation was manually excluded.

The [Mull report](mutations.json) retains source, locations, replacements, and
statuses. File keys and mutant identifiers use repository-relative paths.
Its `mutationScore` is 100 and the killed/total ratio is 20/20.
Run `bash tools/run-checks.sh mutation` on Linux to reproduce it. The runner
treats tool warnings as errors and requires a mutation score of 100 while
retaining the report for inspection. The compiler's standard-library location
is discovered at runtime. Saved reports contain no account names, absolute
workspace paths, machine names, or processor details.

## CRAP definition

[Per-function measurements](crap.md) combine Lizard 1.24.3 cyclomatic complexity
with each function's GCC line coverage:

```text
CRAP(line) = complexity² × (1 − covered_lines / executable_lines)³ + complexity
```

This is explicitly a line-based approximation. The [original definition](https://www.artima.com/weblogs/viewpost.jsp?thread=210575)
used basis-path coverage; its historical threshold of 30 is a heuristic, not
a correctness guarantee. The adapter rejects missing, duplicate, ambiguous, or
unsupported-format data instead of assuming full coverage. Its ten tests
exercise known formula values and coverage mapping failures. Function ranges
are also cross-checked against this small source tree. Complex templates,
overlapping lambdas, and generated code would require additional mapping work.

## Reproduction

Run these commands directly from the repository root on Linux:

```sh
bash tools/run-checks.sh sanitizers
bash tools/run-checks.sh coverage
bash tools/run-checks.sh mutation
bash tools/run-checks.sh release-clang
bash tools/run-checks.sh release
```

See installation instructions in [README](../../README.md).
Quality tools are optional for the ordinary build.

Metrics describe these tests and operators; they do not prove absence of bugs.
The acceptance checks are strict compilation, observable behavior, and sanitizer
results, with the numeric assumptions and mutation scope explicitly reviewed.
