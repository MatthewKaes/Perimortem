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
    name = "concept",
    hdrs = [
        "concept/abstract.hpp",
        "concept/alias.hpp",
        "concept/documentation.hpp",
        "concept/invalid.hpp",
        "concept/layout.hpp",
        "concept/reference.hpp",
    ],
    deps = [
        "//perimortem:core",
        "//perimortem:system",
    ],
)

cc_library(
    name = "model",
    srcs = [
        "model/argument.cpp",
        "model/binding.cpp",
        "model/layouts/fluid.cpp",
        "model/layouts/named.cpp",
        "model/layouts/structured.cpp",
        "model/pack.cpp",
        "model/packs/named.cpp",
        "model/packs/positional.cpp",
        "model/projection.cpp",
    ],
    hdrs = [
        "model/addressable.hpp",
        "model/argument.hpp",
        "model/attribute.hpp",
        "model/binding.hpp",
        "model/callable.hpp",
        "model/callables/self.hpp",
        "model/callables/static.hpp",
        "model/constant.hpp",
        "model/constants/bytes.hpp",
        "model/constants/flag.hpp",
        "model/constants/real.hpp",
        "model/constants/signed.hpp",
        "model/constants/unsigned.hpp",
        "model/expression.hpp",
        "model/generic.hpp",
        "model/layouts/fluid.hpp",
        "model/layouts/named.hpp",
        "model/layouts/structured.hpp",
        "model/pack.hpp",
        "model/packs/named.hpp",
        "model/packs/positional.hpp",
        "model/projection.hpp",
        "model/type.hpp",
        "model/types/flag.hpp",
        "model/types/real.hpp",
        "model/types/signed.hpp",
        "model/types/terminal.hpp",
        "model/types/unsigned.hpp",
    ],
    deps = [
        ":concept",
        "//perimortem:core",
    ],
)

cc_library(
    name = "ttx",
    deps = [
        ":concept",
        ":lexical",
        ":model",
        "//perimortem:core",
        "//perimortem:memory",
    ],
)
