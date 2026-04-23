// Perimortem Engine
// Copyright © Matt Kaes

#include "rpc_request.hpp"

using namespace Tetrodotoxin::Lsp;
using namespace Perimortem::Memory;
using namespace Perimortem::Storage::Json;

auto RpcRequest::load_params() -> void {
  if (!is_valid()) {
    return;
  }

  // Delay parsing of params in the event we drop the request.
  call_params.from_source(arena, source, header.get_params_offset());
}

auto RpcRequest::rpc_error(Core::View::Amorphous error) const -> RpcResponse {
  // Escape any error quotes.
  Managed::Bytes sanatized_error(arena);
  sanatized_error.proxy(error);
  sanatized_error.convert('"', '`');

  auto response = create_object({{"jsonrpc"_view, header.get_version()},
                                 {"id"_view, Node::raw(header.get_id())},
                                 {"error"_view, sanatized_error.get_view()}});

  return Node(response);
}

auto RpcRequest::rpc_result(ReponseObject result) const -> RpcResponse {
  return create_object({{"jsonrpc"_view, header.get_version()},
                        {"id"_view, Node::raw(header.get_id())},
                        {"error"_view, Node(result)}});
}