// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/utility/pair.hpp"

#include "puffer/lsp/documents.hpp"
#include "puffer/lsp/rpc/executor.hpp"

namespace Puffer::Lsp {

// Each method translates one protocol request into the narrow Documents query
// it owns. The dispatch table fixes the complete server surface at build time.
auto initialize(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto document_formatting(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto did_open(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto did_change(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto did_close(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto semantic_tokens(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response;
auto hover(Documents& documents, const Rpc::Message& message) -> Rpc::Response;

using Method =
    Perimortem::Utility::Pair<Perimortem::Core::View::Bytes, Rpc::DispatchFunc>;

inline constexpr Perimortem::Core::Static::Vector<Method, 7> method_table = {{
  Method{"initialize"_view, initialize},
  {"textDocument/formatting"_view, document_formatting},
  {"textDocument/didOpen"_view, did_open},
  {"textDocument/didChange"_view, did_change},
  {"textDocument/didClose"_view, did_close},
  {"textDocument/semanticTokens/full"_view, semantic_tokens},
  {"textDocument/hover"_view, hover},
}};

using Executor = Rpc::Executor<method_table>;

}  // namespace Puffer::Lsp
