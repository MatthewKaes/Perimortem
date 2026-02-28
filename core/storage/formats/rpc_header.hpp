// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "memory/arena.hpp"
#include "memory/managed/bytes.hpp"

namespace Perimortem::Storage::Json {

class RpcHeader {
 public:
  RpcHeader(Memory::Arena& arena,
            Memory::View::Bytes contents);

  inline constexpr auto get_version() const
      -> const Memory::View::Bytes {
    return json_rpc;
  }
  inline constexpr auto get_method() const
      -> const Memory::View::Bytes {
    return method;
  }
  inline constexpr auto get_id() const -> const Memory::View::Bytes {
    return id;
  }

  inline constexpr auto get_arena() const -> Memory::Arena    & {
    return arena;
  }

  inline constexpr auto get_params_offset() const -> uint32_t {
    return params_offset;
  }

 private:
  auto parse_rpc(const Memory::View::Bytes& contents,
                 uint32_t& position) -> void;
  auto parse_id(const Memory::View::Bytes& contents,
                uint32_t& position) -> void;
  auto parse_method(const Memory::View::Bytes& contents,
                    uint32_t& position) -> void;
  auto parse_params(const Memory::View::Bytes& contents,
                    uint32_t& position) -> void;

  Perimortem::Memory::Arena& arena;
  Memory::View::Bytes json_rpc;
  Memory::View::Bytes method;
  Memory::View::Bytes id;
  uint32_t params_offset;
};

}  // namespace Perimortem::Storage::Json
