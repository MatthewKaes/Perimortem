"""
TTX language core.
"""

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "lexical",
    srcs = [
        "lexical/code.cpp",
        "lexical/errors.cpp",
        "lexical/tokenizer.cpp",
    ],
    hdrs = [
        "lexical/code.hpp",
        "lexical/cursor.hpp",
        "lexical/errors.hpp",
        "lexical/lexicon.hpp",
        "lexical/token.hpp",
        "lexical/tokenizer.hpp",
    ],
    deps = [
        "//perimortem:core",
        "//perimortem:memory",
        "//perimortem:serialization",
        "//perimortem:utility",
    ],
)

cc_library(
    name = "concept",
    srcs = [
        "concept/documentation.cpp",
        "concept/invalid.cpp",
    ],
    hdrs = [
        "concept/abstract.hpp",
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
        "model/binding.cpp",
        "model/documentations/comment.cpp",
        "model/generic.cpp",
        "model/layouts/composite.cpp",
        "model/layouts/fluid.cpp",
        "model/layouts/named.cpp",
        "model/layouts/ranged.cpp",
    ],
    hdrs = [
        "model/addressable.hpp",
        "model/addressables/writable.hpp",
        "model/alias.hpp",
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
        "model/documentations/block.hpp",
        "model/documentations/comment.hpp",
        "model/documentations/merged.hpp",
        "model/exports.hpp",
        "model/expression.hpp",
        "model/generic.hpp",
        "model/layouts/composite.hpp",
        "model/layouts/fluid.hpp",
        "model/layouts/named.hpp",
        "model/layouts/ranged.hpp",
        "model/layouts/structured.hpp",
        "model/projection.hpp",
        "model/type.hpp",
        "model/types/bool.hpp",
        "model/types/flag.hpp",
        "model/types/real.hpp",
        "model/types/real_32.hpp",
        "model/types/real_64.hpp",
        "model/types/signed.hpp",
        "model/types/signed_16.hpp",
        "model/types/signed_32.hpp",
        "model/types/signed_64.hpp",
        "model/types/signed_8.hpp",
        "model/types/terminal.hpp",
        "model/types/unsigned.hpp",
        "model/types/unsigned_16.hpp",
        "model/types/unsigned_32.hpp",
        "model/types/unsigned_64.hpp",
        "model/types/unsigned_8.hpp",
    ],
    deps = [
        ":concept",
        "//perimortem:core",
        "//perimortem:memory",
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
