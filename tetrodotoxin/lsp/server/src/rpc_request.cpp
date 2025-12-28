// Perimortem Engine
// Copyright © Matt Kaes

#include "rpc_request.hpp"

using namespace Tetrodotoxin::Lsp;
using namespace Perimortem::Memory;
using namespace Perimortem::Storage::Json;

static const Perimortem::Storage::Json::Node null;

auto RpcRequest::load_params() -> void {
  if (!is_valid()) {
    return;
  }

  // Delay parsing of params in the event we drop the request.
  call_params.from_source(arena, source, header.get_params_offset());
}

auto RpcRequest::rpc_error(Perimortem::Memory::ByteView error) const
    -> RpcResponse {
  RpcResponse response;
  // Escape any error quotes.
  ManagedString sanatized_error(arena);
  sanatized_error.proxy(error);
  sanatized_error.convert('"', '`');

  ManagedString err(arena);
  err.append("{\"jsonrpc\": \"");
  err.append(header.get_version());
  err.append("\",\"id\":\"");
  err.append(header.get_id());
  err.append("\",\"error\":\"");
  err.append(sanatized_error);
  err.append("\"}");

  response.result = err;
  return response;
}
