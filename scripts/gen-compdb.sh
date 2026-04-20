#!/bin/sh
set -e

echo "Generating compile_commands.json with Bear..."

# Clean first to ensure all compilation commands are captured
make clean
bear -- make all test

echo "compile_commands.json generated successfully."
