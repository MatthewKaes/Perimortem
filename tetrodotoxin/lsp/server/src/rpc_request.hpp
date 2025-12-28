// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "memory/arena.hpp"
#include "memory/byte_view.hpp"
#include "storage/formats/json.hpp"
#include "storage/formats/rpc_header.hpp"

namespace Tetrodotoxin::Lsp {

struct RpcResponse {
  Perimortem::Memory::ByteView result;
};

class RpcRequest {
 public:
  RpcRequest(Perimortem::Memory::Arena& arena,
             const Perimortem::Memory::ByteView& source)
      : arena(arena), header(arena, source), source(source) {}

  auto load_params() -> void;
  auto rpc_error(Perimortem::Memory::ByteView error) const -> RpcResponse;

  inline constexpr auto is_valid() const -> bool {
    return !header.get_version().empty() && !header.get_method().empty() &&
           header.get_params_offset() > 0;
  }

  auto get_method() const -> std::string_view {
    return header.get_method().get_view();
  }
  auto get_arena() const -> Perimortem::Memory::Arena& { return arena; }
  auto get_source() const -> const Perimortem::Memory::ByteView& {
    return source;
  }
  auto get_params() const -> const Perimortem::Storage::Json::Node& {
    return call_params;
  }

 private:
  Perimortem::Memory::Arena& arena;
  Perimortem::Storage::Json::RpcHeader header;
  Perimortem::Storage::Json::Node call_params;
  const Perimortem::Memory::ByteView& source;
};

}  // namespace Tetrodotoxin::Lsp