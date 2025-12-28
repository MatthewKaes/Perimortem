// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "memory/arena.hpp"
#include "memory/managed_string.hpp"

namespace Perimortem::Storage::Json {

class RpcHeader {
 public:
  RpcHeader(Perimortem::Memory::Arena& arena,
            Perimortem::Memory::ByteView contents);

  inline constexpr auto get_version() const
      -> const Perimortem::Memory::ByteView {
    return json_rpc;
  }
  inline constexpr auto get_method() const
      -> const Perimortem::Memory::ByteView {
    return method;
  }
  inline constexpr auto get_id() const -> const Perimortem::Memory::ByteView {
    return id;
  }

  inline constexpr auto get_arena() const -> Perimortem::Memory::Arena    & {
    return arena;
  }

  inline constexpr auto get_params_offset() const -> uint32_t {
    return params_offset;
  }

 private:
  auto parse_rpc(const Perimortem::Memory::ByteView& contents,
                 uint32_t& position) -> void;
  auto parse_id(const Perimortem::Memory::ByteView& contents,
                uint32_t& position) -> void;
  auto parse_method(const Perimortem::Memory::ByteView& contents,
                    uint32_t& position) -> void;
  auto parse_params(const Perimortem::Memory::ByteView& contents,
                    uint32_t& position) -> void;

  Perimortem::Memory::Arena& arena;
  Perimortem::Memory::ByteView json_rpc;
  Perimortem::Memory::ByteView method;
  Perimortem::Memory::ByteView id;
  uint32_t params_offset;
};

}  // namespace Perimortem::Storage::Json
