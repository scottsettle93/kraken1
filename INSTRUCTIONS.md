# C++ Technical Assessment – Code Review and Improvement

## Overview

This exercise simulates working with an existing codebase.

You are given a small C++ project that processes sensor readings and calculates simple statistics. The current implementation works for some cases but has several issues in areas such as design, robustness, and maintainability.

Your task is to review the code and improve it.

The goal of this exercise is not just to produce working code, but to demonstrate how you approach unfamiliar code, identify problems, and improve a system.

## Your Tasks

1. **Review the existing code**
2. **Fix any bugs or incorrect behavior**
3. **Improve the design and code quality where appropriate**
4. **Improve robustness (error handling, edge cases, etc.)**
5. **Document the changes you made**

You are encouraged (but not required) to:

* Refactor the code where it improves clarity or structure
* Add tests
* Improve the build configuration
* Add comments where they help readability
* Improve input validation
* Improve performance if necessary

Please focus on **clarity, maintainability, and correctness** rather than adding excessive features.


## Input Format

The program reads a file containing sensor readings.

Each line contains:

```
<SENSOR_TYPE> <VALUE>
```

Example:

```
TEMP 23.4
TEMP 22.8
PRESSURE 101.2
TEMP 21.9
PRESSURE 100.9
```

## Expected Behavior

The program calculates the **average value for each sensor type** and prints the results.

Example output:

```
TEMP avg: 22.7
PRESSURE avg: 101.05
```

## Deliverables

Please submit a **git repository** containing your work.

Your repository should include:

* The updated source code
* Build instructions
* A `RESULTS.md` describing your work

---

## RESULTS Requirements

Your RESULTS file should include:

### Issues Found

List any problems you discovered in the original implementation.

### Changes Made

Explain the improvements you implemented.

### Future Improvements

Describe anything you would improve if you had more time.

## Time Expectation

The speed at which this can be completed is variable, but should take no longer than approximately **4–6 hours**.

You are not expected to produce a perfect solution. We are more interested in your **engineering judgment and approach**.


## AI Tool Usage Policy

The use of AI-assisted tools (e.g., ChatGPT, GitHub Copilot, Claude, etc.) is **permitted** during this assessment. However, there are requirements:

1. **Document all AI usage.** In your submission RESULTS file, include a section titled **"AI Tool Usage"** that describes:
   - Which tool(s) you used
   - What you used them for (e.g., understanding a concept, generating a code snippet, debugging)
   - How you validated or modified the AI-generated output
2. **You must understand every change.** Be prepared to explain any code you submit as if you wrote it yourself.
3. **AI output is not a substitute for judgment.** We are evaluating your engineering reasoning, not your ability to prompt a model. Blindly applying AI suggestions without critical review will be apparent and will count against your submission.

Honest, transparent use of AI tools will not negatively affect your evaluation. Undisclosed use may.

---

## Submission

Please provide a link to your git repository.


Thank you for taking the time to complete this exercise.
