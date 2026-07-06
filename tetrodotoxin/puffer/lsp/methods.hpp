// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/view/bytes.hpp"

#include "perimortem/utility/pair.hpp"

#include "tetrodotoxin/puffer/lsp/documents.hpp"
#include "tetrodotoxin/puffer/lsp/rpc/executor.hpp"

namespace Tetrodotoxin::Puffer::Lsp {

auto initialize(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto format(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto did_open(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto did_change(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto did_close(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto semantic_tokens(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;

using Method = Perimortem::Utility::Pair<
    Perimortem::Core::View::Bytes,
    Rpc::DispatchFunc>;

inline constexpr Perimortem::Core::Static::Vector<Method, 6> method_table = {{
  {"initialize"_view, initialize},
  {"format"_view, format},
  {"textDocument/didOpen"_view, did_open},
  {"textDocument/didChange"_view, did_change},
  {"textDocument/didClose"_view, did_close},
  {"textDocument/semanticTokens/full"_view, semantic_tokens},
}};

static constexpr Count default_executor_count = 4;

using Executor = Rpc::Executor<method_table, default_executor_count>;

}  // namespace Tetrodotoxin::Puffer::Lsp
