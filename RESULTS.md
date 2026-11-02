# Assessment Results

## Issues Found

The original program lost fractional readings by accumulating into integers and
then performing integer division. The five-line example produced `PRESSURE avg:
100` and `TEMP avg: 22` instead of `101.05` and `22.7`.

Linux probes also demonstrated crashes with no arguments, missing files, and
empty files; stack-buffer overflows from long tokens; silent acceptance of
invalid numeric text; and undefined conversions for large or nonfinite values.
The blank line in the supplied fixture reused previous tokens in the observed
run and added a duplicate reading.

Manual pointer ownership leaked the final reading, underflowed the destruction
loop on empty input, and made copying unsafe. Repeated calculations retained old
sensor types. The reader header lacked an include guard, headers exposed
`using namespace std`, and the build had no explicit language standard or tests.

See the [baseline audit](docs/research/baseline-audit.md) for probes, locations,
and the distinction between demonstrated behavior and static observations.

## Changes Made

- Replaced owning pointers with value-owned readings and automatic resource
  management. Parsing accepts an input stream, so files and in-memory test data
  exercise the same behavior.
- Made averaging stateless, used scaled compensated accumulation and an unsigned
  size type for counts, and separated calculation from presentation. The numeric
  algorithm avoids overflowing extreme totals using standard double arithmetic.
- Validated complete records with locale-independent numeric conversion. Blank
  lines are ignored; malformed records fail with a file/line diagnostic and no
  partial averages. This policy is an explicit interpretation of the
  underspecified validation requirement.
- Added argument validation, file/read/write error handling, deterministic
  ordering, and ten-significant-digit output. Empty files return success without
  inventing an average.
- Added C++17 CMake targets, GoogleTest and CLI integration tests, strict
  optimized builds, sanitizer builds, coverage, Mull mutation analysis, and
  per-function line-based CRAP reporting.
- Updated build instructions and completed the separate
  [ring-buffer exercise](technical_writing/RING_BUFFER.md).

The implementation preserves the supplied instructions and data. Code uses the
original four-space, same-line-brace style without comments.

## Validation

All authoritative implementation checks run on Linux. The [quality report](docs/quality/REPORT.md) records exact
measurements, versions, scope, exclusions, and the mutation-driven simplification.

The suite contains 41 GoogleTest cases and five Python CLI test methods,
registered as 42 CTest entries. A separate ten-test Python suite checks the CRAP
formula and coverage-to-function mapping. Assertions use the challenge example,
independent expected values, observable exit codes, diagnostics, and output.

The checks cover decimal arithmetic, grouping, case sensitivity, blanks and
CRLF, long records, malformed values and signs, extreme finite values, repeated
calls, usage, missing files, input/output failures, and formatting.
Synthetic cases exercise 16,384 extreme finite readings and an output stream
that throws an unexpected exception. Reported production line, function, and
branch coverage is 100%; all 20 generated mutations are killed. The quality
commands enforce these coverage and mutation thresholds.

## Future Improvements

For substantially larger files, aggregate while parsing instead of retaining
every reading. For applications that require exact decimal results or stronger
error bounds across extreme cancellation, choose a numerical representation and
accuracy contract appropriate to the domain.

A production service would also need explicit resource limits and a broader
platform/toolchain matrix. The current assessment does not introduce sensor
units, physical-value limits, multiple-file processing, or a new configuration
system without requirements for them.

## AI Tool Usage

OpenAI Codex assisted with repository setup, investigation, research,
implementation, test generation, tooling, and documentation. Background research
agents investigated the baseline, testing tools, and ring-buffer explanation.
The implementation follows the agreed scope, omits code comments, and uses
strict optimized final verification on Linux.

Validation included reproducing baseline failures, running the C++ and CLI
suites, sanitizer checks, inspecting mutation survivors, cross-checking coverage
against the compiler's reports, and testing the CRAP adapter independently.
Testing caught and corrected acceptance of `+-1`. Inspection also led to explicit
output-format normalization and replacement of a faulty LLVM-to-gcovr reporting
path with GCC/gcov. An equivalent surviving mutation exposed a redundant parser
length check; removing that check simplified the code without changing accepted
input. Portability review replaced an unnecessary wider-type requirement with
scaled compensated addition and replaced platform-specific temporary-file APIs
with standard filesystem handling. Boundary tests and stream fault injection
exercise extreme values and exception propagation through the public interfaces.
Saved reports use repository-relative paths and omit machine-specific details.
Review also exposed cancellation error from arbitrary scaling. A regression
test reproduced the error, and power-of-two scaling corrected it before final
verification.

The checks and research documented here were performed by Codex. This document
does not claim independent human review or understanding of the generated work.
