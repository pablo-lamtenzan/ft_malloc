#!/bin/sh
set -e

if ! command -v valgrind >/dev/null 2>&1; then
    echo "Error: valgrind is not installed." >&2
    exit 1
fi

BIN=${1:-project_test}

if [ ! -f "$BIN" ]; then
    echo "Error: Binary $BIN not found." >&2
    echo "Compile the tests using 'make test' first." >&2
    exit 1
fi

echo "Running valgrind on $BIN..."

# Execute with comprehensive leak checking, ensuring an error exit code on leaks
valgrind \
    --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    --error-exitcode=1 \
    "./$BIN"

echo "Valgrind completed successfully. No leaks found!"
