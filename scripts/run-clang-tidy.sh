#!/bin/sh
set -e

if [ ! -f compile_commands.json ]; then
    echo "Error: compile_commands.json not found." >&2
    echo "Please generate it using 'make compdb' first." >&2
    exit 1
fi

echo "Running clang-tidy on source and test files..."

# Find all .c files in src/ and tests/ and run clang-tidy using the compilation database
find src tests -name '*.c' -exec clang-tidy -p . {} +

echo "clang-tidy checks completed successfully."
