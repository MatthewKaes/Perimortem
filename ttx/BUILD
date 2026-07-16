"""
TTX language core.
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "lexical",
    srcs = [
        "lexical/class.cpp",
        "lexical/cursor.cpp",
        "lexical/errors.cpp",
        "lexical/tokenizer.cpp",
    ],
    hdrs = [
        "lexical/class.hpp",
        "lexical/cursor.hpp",
        "lexical/errors.hpp",
        "lexical/source.hpp",
        "lexical/token.hpp",
        "lexical/tokenizer.hpp",
    ],
    include_prefix = "ttx/lexical",
    deps = [
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:utility",
    ],
)

cc_library(
    name = "abstraction",
    hdrs = [
        "abstraction/abstract.hpp",
        "abstraction/alias.hpp",
        "abstraction/invalid.hpp",
        "abstraction/reference.hpp",
    ],
    deps = [
        "//perimortem:core",
        "//perimortem:system",
    ],
)

cc_library(
    name = "model",
    srcs = [
        "model/layouts/fluid.cpp",
        "model/layouts/named.cpp",
        "model/layouts/structured.cpp",
    ],
    hdrs = [
        "model/addressable.hpp",
        "model/argument.hpp",
        "model/attribute.hpp",
        "model/callable.hpp",
        "model/documentation.hpp",
        "model/generic.hpp",
        "model/layout.hpp",
        "model/layouts/fluid.hpp",
        "model/layouts/named.hpp",
        "model/layouts/structured.hpp",
        "model/self.hpp",
        "model/static.hpp",
        "model/type.hpp",
        "model/types/flag.hpp",
        "model/types/real.hpp",
        "model/types/signed.hpp",
        "model/types/terminal.hpp",
        "model/types/unsigned.hpp",
    ],
    deps = [
        ":abstraction",
        "//perimortem:core",
    ],
)

cc_library(
    name = "ttx",
    deps = [
        ":abstraction",
        ":lexical",
        ":model",
        "//perimortem:core",
        "//perimortem:memory",
    ],
)
