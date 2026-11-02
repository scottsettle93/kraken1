# C++ testing, mutation testing, and CRAP reporting

## Tool choices

The project uses C++17, CMake, GoogleTest/CTest, GCC coverage, Clang sanitizers,
Mull mutation testing, and a small line-based CRAP adapter. The ordinary program
build requires only a C++17 compiler and CMake; quality tools are optional.

GoogleTest provides assertions and CTest discovery. Installed packages avoid
downloads during configuration. Tests can be disabled with `BUILD_TESTING=OFF`.
[GoogleTest CMake guide](https://google.github.io/googletest/quickstart-cmake.html)

Mull instruments production code using a plugin that must match Clang/LLVM.
The checked combination is Mull 0.36.0 with Clang/LLVM 19. Select packages for
the reviewer's system. The runner accepts a plugin path and discovers the
selected compiler's standard-library location. No processor or distribution
is a project requirement.
[Mull installation](https://mull-project.com/getting-started/installation/),
[Mull setup](https://mull-project.com/getting-started/setup/)

The configuration requests arithmetic, comparison, boundary, and logical
operators. Only generated mutants define the score denominator. Uncovered
mutants are included; individual survivors are not manually excluded.
[Mull mutators](https://mull-project.com/reference/mutators/),
[Mull filters](https://mull-project.com/reference/filters/)

## Coverage

GCC/gcov with gcovr measures production source, including the CLI entry point.
An initial LLVM-to-gcovr import omitted an executable and miscounted lines;
comparison with the compiler's own report exposed the discrepancy. The final
workflow uses GCC/gcov and checks every production function has a measurement.
Tests and third-party libraries are outside this metric.

Separate build directories prevent instrumentation profiles from mixing.
Compiler-generated unreachable and exception-throw branches are filtered;
no production source line is manually excluded.
[gcovr compilation guide](https://gcovr.com/en/stable/guide/compiling.html),
[gcovr JSON format](https://gcovr.com/en/stable/output/json.html)

## CRAP interpretation

The original definition is:

```text
CRAP = complexity² × (1 − coverage)³ + complexity
```

The original authors used basis-path coverage. This project uses per-function
line coverage, so the result is explicitly an approximation. Branch coverage is
reported separately. The historical warning threshold of 30 is a heuristic,
not a correctness guarantee.
[Original definition](https://www.artima.com/weblogs/viewpost.jsp?thread=210575),
[author's retrospective](https://testing.googleblog.com/2011/02/this-code-is-crap.html)

Lizard provides source-level cyclomatic complexity and function ranges. The
adapter joins those ranges to gcovr records and rejects missing, duplicate,
ambiguous, overlapping, or unsupported-format measurements. Missing data never
becomes 100% coverage. Ten tests cover the formula and mapping errors.
Complex templates and generated code would require additional mapping work.
[Lizard documentation](https://github.com/terryyin/lizard/blob/1.24.3/README.rst)

## Portable numeric behavior

The calculation uses power-of-two scaling based on each group's largest magnitude, then
uses compensated addition before dividing and restoring the scale. Compensation
uses the TwoSum error recovery and cascaded summation in Algorithms 3.1 and 4.1
of [Ogita, Rump, and Oishi](https://www.tuhh.de/ti3/paper/rump/OgRuOi05.pdf).
This uses ordinary `double` arithmetic without requiring a wider `long double`.
It improves cancellation behavior but does not provide exact arithmetic:
normalization can underflow sufficiently small relative contributions.

## Reproduction and report hygiene

See [README](../../README.md) for prerequisites and commands and
[the quality report](../quality/REPORT.md) for measured results. Saved mutation
reports use repository-relative paths, and recorded versions contain only tool
versions. Account names, workspace paths, machine identifiers, and processor
details are not needed to reproduce the checks.
