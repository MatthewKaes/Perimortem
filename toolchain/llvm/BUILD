"Pinned LLVM development SDK facade for the Library compiler."

package(default_visibility = ["//visibility:private"])

# Library deliberately consumes this through implementation_deps. The facade
# keeps the external repository name and its component targets out of product
# BUILD files, while narrow visibility makes accidental use elsewhere a
# build-time error.
alias(
    name = "sdk",
    actual = "@llvm_22_1_8//:sdk",
    visibility = ["//tetrodotoxin:__pkg__"],
)

exports_files([
    "MANIFEST.md",
    "README.md",
])
