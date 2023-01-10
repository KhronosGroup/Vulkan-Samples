#!/bin/bash

set -ux

for file in `git diff-index --name-only HEAD | grep -iE '\.(cpp|cc|h|hpp)$' ` ; do
    if [ -f $file ]; then
        clang-format -i $file
    fi
done

# case "${1}" in
#   --about )
#     echo "Runs clang-format on source files"
#     ;;
#   * )
#     for file in `git diff-index --cached --name-only HEAD | grep -iE '\.(cpp|cc|h|hpp)$' ` ; do
#       format_file "${file}"
#     donegit d
# esac