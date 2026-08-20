"""Canonical source Packages distributed with Tetrodotoxin."""

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "perimortem_memory_sources",
    srcs = glob(["Perimortem.Memory/*.ttx"]),
)

filegroup(
    name = "perimortem_system_sources",
    srcs = glob(["Perimortem.System/*.ttx"]),
)

filegroup(
    name = "standard_sources",
    srcs = [
        ":perimortem_memory_sources",
        ":perimortem_system_sources",
    ],
)
