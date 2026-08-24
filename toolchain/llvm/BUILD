# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"Pinned LLVM development SDK facade for the LLVM Terminal."

package(default_visibility = ["//visibility:private"])

# The LLVM Terminal is the only product owner that consumes this facade. Keeping
# the external repository behind one narrow target prevents SDK headers and
# compiler flags from becoming ambient inputs to semantic code.
alias(
    name = "sdk",
    actual = "@llvm_22_1_8//:sdk",
    visibility = ["//tetrodotoxin/terminal:__pkg__"],
)

exports_files([
    "MANIFEST.md",
    "README.md",
])
