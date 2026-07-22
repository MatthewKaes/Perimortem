"""Canonical multi-Scene TTX application sources."""

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "sources",
    srcs = glob(["*.ttx"]),
)

# The Scene evaluator and runtime are not implemented yet. Keep the intended
# application target visible without pretending that the current App runtime
# can execute this package.
#
# ttx_application(
#     name = "scene_demo",
#     package = "package.ttx",
#     resources = [
#         "//:.data/icon.png",
#         "//:.data/logo.png",
#     ],
# )
