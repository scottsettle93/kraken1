## Build environment

Run all configuration, compilation, tests, coverage, and mutation analysis on
Linux. Run commands directly from the repository root. Use separate build
directories for Release, sanitizers, coverage, and mutation analysis.

## Code style

Code comments are forbidden. Put explanations in Markdown documentation.
Preserve the existing style: four-space indentation and opening braces on the
same line. Apply it to production code and tests.

## Final verification and Git

Finish with an optimized Release build, warnings treated as errors, and the full
test suite run against that build. Do not run Git commands or create commits.

## Agent skills

### Issue tracker

Issues and specs live in local Markdown under `.scratch/<feature>/`.
Before reading or writing tickets, read `docs/agents/issue-tracker.md`.

### Triage labels

Use the five default triage roles.
Before triaging, read `docs/agents/triage-labels.md`.

### Domain docs

Single-context layout: root `GLOSSARY.md` and `docs/adr/`.
Before exploring the codebase, read `docs/agents/domain.md`.
