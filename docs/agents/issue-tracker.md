# Issue tracker: Local Markdown

Issues and specs for this repo live as Markdown files in `.scratch/`.

## Conventions

- One feature per directory: `.scratch/<feature-slug>/`.
- The spec is `.scratch/<feature-slug>/spec.md`.
- Implementation issues are individual files at
  `.scratch/<feature-slug>/issues/<NN>-<slug>.md`, numbered from `01`.
- Triage state is a `Status:` line near the top of each issue file.
  Use the role strings in `triage-labels.md`.
- Append comments and conversation history under `## Comments`.

## When a skill says "publish to the issue tracker"

Create a file under `.scratch/<feature-slug>/`, creating directories as needed.
Use the spec or individual issue path above.

## When a skill says "fetch the relevant ticket"

Read the referenced file. Resolve an issue number within its feature directory.

## Wayfinding operations

Used by `/wayfinder`. The map is one file with a child file per ticket.

- Map: `.scratch/<effort>/map.md`, containing Notes, Decisions-so-far, and Fog.
- Child ticket: `.scratch/<effort>/issues/NN-<slug>.md`.
  Put the question in the body and record a `Type:` of
  `research`, `prototype`, `grilling`, or `task`.
  Wayfinding uses `Status: claimed` and `Status: resolved` for progress.
- Blocking: record `Blocked by: NN, NN` near the top.
  A ticket is unblocked when every listed ticket is resolved.
- Frontier: scan the effort's issues for open, unblocked, unclaimed tickets;
  first by number wins.
- Claim: set `Status: claimed` and save before starting work.
- Resolve: append the answer under `## Answer`, set `Status: resolved`,
  and add a summary and link to the map's Decisions-so-far.
