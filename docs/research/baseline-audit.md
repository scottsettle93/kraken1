# Baseline evidence for the planning interview

Audited the original supplied implementation. This records the initial investigation,
before implementation changes. Source, configuration, data, and the existing
`build/` directory were not changed during the audit; no software was installed.

## Environment and method

The baseline was exercised on Linux with GCC 14.2. Baseline sources and small
probe fixtures were staged in a temporary directory; compiler outputs and test
executions stayed there. Repository source was read without modification.

Each program invocation used Python `subprocess.run(..., capture_output=True, text=True, timeout=3)`. Each compile had a 30-second timeout. No timeout fired, and no large-input, exhaustion, or unbounded stress test was attempted. From the staged baseline source directory, normal and sanitizer builds used:

```sh
g++ -std=c++17 -g -O0 -Wall -Wextra -Wpedantic -Wconversion \
  -Wsign-conversion -Wshadow src/main.cpp src/SensorReader.cpp \
  src/Statistics.cpp -o sensor-normal

g++ -std=c++17 -g -O0 -Wall -Wextra -Wpedantic -Wconversion \
  -Wsign-conversion -Wshadow -fsanitize=address,undefined,float-cast-overflow \
  -fno-omit-frame-pointer src/main.cpp src/SensorReader.cpp \
  src/Statistics.cpp -o sensor-sanitized
```

Both builds passed. Warnings identify unused `argc` (`main.cpp:6`), signed/unsigned indexing and comparisons (`SensorReader.cpp:10–11`, `Statistics.cpp:8–9`), and conversion from `double` to `int` (`Statistics.cpp:10`). Semantic probes used `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`; a separate leak probe enabled `detect_leaks=1`. Raw commands, outputs, and exit codes were captured alongside the temporary fixtures and harness source. These temporary artifacts are not submission files.

## Demonstrated CLI behavior

The table reports Linux results. Inputs use `\n` to denote actual newlines. Successful runs below exit zero even when the data or answer is wrong. No-argument, nonexistent-path, and empty-file runs all terminate with SIGSEGV in the normal build; sanitizer runs terminate with diagnostics.

| Probe | Observed result | Source location and implication |
| --- | --- | --- |
| Five-line example from `INSTRUCTIONS.md` | `PRESSURE avg: 100`, `TEMP avg: 22`; expected mathematical means are `101.05` and `22.7` | [`Statistics.cpp:5,10,17`](../../src/Statistics.cpp): integer accumulation and division lose fractions. |
| `TEMP 0.9\nTEMP 0.9\n` | `TEMP avg: 0`, rather than `0.9` | `Statistics.cpp:10`: each compound assignment converts the accumulated result back to `int`. |
| `TEMP 1\nTEMP 2\n` | `TEMP avg: 1`, rather than `1.5` | `Statistics.cpp:17`: integer division occurs before assignment to `double`. |
| Repository fixture | `HUMIDITY avg: 46`, `PRESSURE avg: 100`, `TEMP avg: 22` | [`data/sensor_data.txt:4`](../../data/sensor_data.txt) is blank. A reader harness observes 10 readings from nine nonblank records, duplicating the preceding `PRESSURE 101.2`. If blanks are ignored, correct means are `46.5`, approximately `100.633333`, and `23.05`. |
| No arguments; nonexistent path | Crash; UBSan reports null pointer passed to `fgets` argument 3 | [`main.cpp:7`](../../src/main.cpp), [`SensorReader.cpp:16–19`](../../src/SensorReader.cpp): no argument-count or `fopen` check. With no argument, `argv[1]` is the null terminator, not itself an out-of-bounds array access. |
| Empty file | Crash during destruction; UBSan reports reference binding to null pointer | `SensorReader.cpp:10–11`: `size() - 1` underflows when size is zero, entering the deletion loop. |
| `TEMP 2\n\nTEMP 4\n` | `TEMP avg: 2`; blank reuses preceding tokens in this run | `SensorReader.cpp:20–26`: uninitialized local buffers and unchecked `sscanf` conversion count. Observed token reuse is not a portable contract. |
| `TEMP 2\nPRESSURE\n` | `PRESSURE avg: 2`, `TEMP avg: 2` | Same unchecked parsing: the second line has no value but still creates a reading. |
| `TEMP nope\n`; `TEMP 12junk\n`; `TEMP 2 ignored\n` | Respectively `0`, `12`, `2`; all accepted | `SensorReader.cpp:22,26`: `atof` has no validation and token scanning ignores remaining text. |
| 80-character sensor token; 80-character numeric token | ASan `stack-buffer-overflow`, write size 81 | `SensorReader.cpp:20–22`: unrestricted `%s` writes into 64-byte arrays. Normal-build output is corrupted/compiler-dependent. |
| One physical line: `TEMP 1` + 249 spaces + `TEMP 9\n` | `TEMP avg: 5`: one physical line becomes two records | `SensorReader.cpp:18–19`: a 255-character `fgets` chunk is treated as a complete line. |
| `TEMP 2147483648\n`; `TEMP 2147483647\nTEMP 1\n`; `TEMP nan\n`; `TEMP inf\n` | UBSan reports value outside representable `int` range | `Statistics.cpp:10`: out-of-range floating-to-integer conversion, including overflow of the accumulated total. This expression performs floating addition followed by conversion; it is not an integer-addition-overflow diagnostic. |

The mathematical results above follow directly from the readings and the requirement to calculate an average in [`INSTRUCTIONS.md`](../../INSTRUCTIONS.md). Parser error policy is not specified there. Sanitizers do not establish that every other malformed input is safe; the uninitialized-buffer paths in particular are not comprehensively detected by ASan/UBSan.

## Ownership and repeated-call probes

Temporary harnesses linked the unchanged `SensorReader.cpp` and `Statistics.cpp`. These exercise public APIs beyond the current one-file CLI; their desired reuse semantics still need a decision.

| Probe | Evidence | Source location |
| --- | --- | --- |
| Destroy a reader containing `A 2\nA 4\n` | LeakSanitizer reports one direct 40-byte leak allocated by `loadFile`. A separate deletion-counting harness records first reading deleted once, final reading zero times. | [`SensorReader.cpp:9–12,24`](../../src/SensorReader.cpp): loop stops before the final element for every nonempty reader. |
| Copy that reader, destroy the copy, then destroy the original | ASan reports a fatal invalid access during the second `SensorReading` cleanup. | [`SensorReader.h:11–20`](../../src/SensorReader.h), `SensorReader.cpp:9–12`: compiler-generated copy shares owning pointers; both destructors delete the same first reading. |
| `loadFile(A)` followed by `loadFile(B)` | With A containing two readings and B one, reader exposes all three. | `SensorReader.cpp:15–28`: appends without clearing. Append versus replace is unspecified. |
| Calculate `[A=2]`, then `[B=8]`, then an empty vector on one `Statistics` | After the second and third calls, output still contains `A avg: 2` and `B avg: 8`. | [`Statistics.cpp:5–17`](../../src/Statistics.cpp): sums/counts are per-call, but `averages` retains keys from previous calls. |
| Include `SensorReader.h`, then `Statistics.h` in one translation unit | Compilation fails with redefinitions of `SensorReading` and `SensorReader`. | `SensorReader.h:1–21`: no include guard; `Statistics.h:5` includes it again. |

Additional **static observations, not fault-injection results**: raw `FILE*` ownership and allocations are not protected during exceptions (`SensorReader.cpp:16–31`); stream read errors are not distinguished from EOF (`:19,31`); callers can mutate the internal vector and its pointers (`SensorReader.h:17`); both iteration counters and counts use `int` (`SensorReader.cpp:10`, `Statistics.cpp:6,8,11`). Their large-input consequences were not stress-tested. Copy-assignment behavior was not executed separately.

## Baseline build tooling

The direct G++ builds above passed. Original CMake configuration was not checked
in this Linux audit because CMake was not installed at that stage. The original
`CMakeLists.txt` did not declare a C++ standard, warnings, tests, or sanitizer
targets, leaving portability dependent on compiler defaults.

The implementation phase subsequently installed CMake and Clang and verified
strict optimized builds on Linux; see the [quality report](../quality/REPORT.md).

## Decisions the assessment leaves open

The specification requires per-sensor averages, better robustness, build instructions, and `RESULTS.md`, including disclosed AI usage. It does not settle these choices:

1. Should blank lines be ignored, and should malformed records abort the file or be skipped with diagnostics? The supplied fixture contains a blank line.
2. What numeric syntax and range are accepted, including exponents, nonfinite values, trailing text, and extreme finite values? What accuracy and output precision are expected?
3. What should empty input produce, and what exit status/diagnostics should usage, open, parse, and read failures use? Are extra CLI arguments errors?
4. Are sensor names case-sensitive, unrestricted tokens, or an enumerated vocabulary? Are length limits intentional requirements?
5. Is output ordering part of the contract? The existing map sorts lexicographically, while the specification example lists `TEMP` first.
6. Must public reader/statistics objects support repeated calls, copying, or accumulating multiple files? The CLI only invokes each once.
7. What maximum input size and platform/compiler baseline are worth supporting within the assessment's 4–6-hour expectation?

These questions distinguish required correctness fixes from policy and design choices; this report does not choose a final design.
