set -eu

if [ "$(uname -s)" != Linux ]; then
    printf '%s\n' 'Run quality checks on Linux.' >&2
    exit 1
fi
cd "$(dirname "$0")/.."
for tool in cmake ninja "${CXX:-g++}" "${CLANGXX:-clang++-19}" python3 mull-runner; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        printf 'Install the prerequisite %s; see README.md.\n' "$tool" >&2
        exit 1
    fi
done

python3 -m venv build/linux-tools/venv
build/linux-tools/venv/bin/python -m pip install -r tools/requirements-quality.txt
mull-runner --version
"${CLANGXX:-clang++-19}" --version
