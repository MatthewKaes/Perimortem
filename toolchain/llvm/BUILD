"Pinned LLVM development SDK facade for the LLVM backend."

package(default_visibility = ["//visibility:private"])

# The LLVM backend is the only product owner that consumes this facade. Keeping
# the external repository behind one narrow target prevents SDK headers and
# compiler flags from becoming ambient inputs to semantic code.
alias(
    name = "sdk",
    actual = "@llvm_22_1_8//:sdk",
    visibility = ["//backend:__pkg__"],
)

exports_files([
    "MANIFEST.md",
    "README.md",
])
