// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/table.hpp"
#include "perimortem/memory/managed/vector.hpp"
#include "perimortem/memory/view/bytes.hpp"

#include "storage/formats/json.hpp"
#include "storage/formats/rpc_header.hpp"

namespace Tetrodotoxin::Lsp {

using ReponseObject =
    Perimortem::Memory::View::Table<Perimortem::Storage::Json::Node>;
using ReponseEntry =
    Perimortem::Memory::View::Table<Perimortem::Storage::Json::Node>::Entry;
using ReponseArray =
    Perimortem::Memory::View::Vector<Perimortem::Storage::Json::Node>;

using RpcResponse = Perimortem::Storage::Json::Node;

class RpcRequest {
 public:
  RpcRequest(Perimortem::Memory::Allocator::Arena& arena,
             const Perimortem::Memory::View::Bytes source)
      : arena(arena), header(arena, source), source(source) {}

  auto load_params() -> void;
  auto rpc_error(Perimortem::Memory::View::Bytes error) const -> RpcResponse;
  auto rpc_result(ReponseObject result) const -> RpcResponse;

  inline constexpr auto is_valid() const -> bool {
    return !header.get_version().empty() && !header.get_method().empty() &&
           header.get_params_offset() > 0;
  }

  inline constexpr auto create_object(
      std::initializer_list<ReponseObject::Entry> items = {}) const
      -> ReponseObject {
    Perimortem::Memory::Managed::Table<Perimortem::Storage::Json::Node> object(
        arena);
    for (const auto& item : items) {
      object.insert(item.name, item.data);
    }
    return object;
  }

  inline constexpr auto create_array(
      std::initializer_list<const Perimortem::Storage::Json::Node> items = {})
      const -> ReponseArray {
    Perimortem::Memory::Managed::Vector<Perimortem::Storage::Json::Node>  array(arena);
    for (const auto& item : items) {
      array.insert(item);
    }
    return array;
  }

  auto get_method() const -> const Perimortem::Memory::View::Bytes {
    return header.get_method();
  }
  auto get_params() const -> const Perimortem::Storage::Json::Node& {
    return call_params;
  }

  auto get_source() const -> const Perimortem::Memory::View::Bytes {
    return source;
  }

  auto get_arena() const -> Perimortem::Memory::Allocator::Arena& { return arena; }

 private:
  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Storage::Json::JsonRpc header;
  Perimortem::Storage::Json::Node call_params;
  Perimortem::Storage::Json::Node response;
  Perimortem::Memory::View::Bytes source;
};

}  // namespace Tetrodotoxin::Lsp