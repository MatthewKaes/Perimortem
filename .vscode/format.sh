#!/usr/bin/env bash

set -euo pipefail

repository="$(git -C "$(dirname "${BASH_SOURCE[0]}")" rev-parse --show-toplevel)"
cd "$repository"

# ripgrep honors the repository ignore rules, keeping generated Bazel trees and
# external dependencies out of the formatter input. LLVM is fetched by Bazel;
# no formatter from PATH is used.
mapfile -d '' sources < <(rg --files --null -g '*.cpp' -g '*.hpp')
if ((${#sources[@]} == 0)); then
  exit 0
fi

bazel run @llvm//:clang-format -- -i "${sources[@]}"
