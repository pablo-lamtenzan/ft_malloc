#!/bin/sh
set -e

if ! command -v gcovr >/dev/null 2>&1; then
    echo "Error: gcovr is not installed." >&2
    exit 1
fi

echo "Generating coverage report using gcovr..."

# Run standard coverage excluding tests from the metric
gcovr --exclude 'tests/.*'

# Create coverage dir
mkdir -p coverage

# Create an HTML detailed coverage report
gcovr --exclude 'tests/.*' --html-details coverage/index.html

echo "Coverage report generated at coverage/index.html"
