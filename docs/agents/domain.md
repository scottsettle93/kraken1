# Domain Docs

This repo uses a single-context layout.

## Before exploring

- Read `GLOSSARY.md` at the repo root.
- Read relevant architecture decision records in `docs/adr/`.

If these files do not exist, proceed silently. The `/domain-modeling`
skill creates them when terms or decisions are resolved.

## File structure

- `GLOSSARY.md`: shared domain vocabulary.
- `docs/adr/NNNN-<decision-slug>.md`: architecture decisions.

## Use the glossary's vocabulary

Use glossary terms in issue titles, refactor proposals, hypotheses, and
test names. When a term is missing, reconsider whether it belongs in the
domain or note the gap for `/domain-modeling`.

## Flag ADR conflicts

If a proposal contradicts an existing ADR, identify the ADR and explain
why the decision should be reconsidered.
