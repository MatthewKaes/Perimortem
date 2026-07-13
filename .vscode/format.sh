#!/usr/bin/env bash

set -euo pipefail

repository="$(git -C "$(dirname "${BASH_SOURCE[0]}")" rev-parse --show-toplevel)"
cd "$repository"

# ripgrep honors the repository ignore rules, keeping generated Bazel trees and
# external dependencies out of the formatter input.
rg --files --null -g '*.cpp' -g '*.hpp' |
  xargs --null --no-run-if-empty clang-format -i
