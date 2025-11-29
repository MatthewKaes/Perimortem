// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "memory/managed_string.hpp"

namespace Perimortem::Storage::Json {

class RpcHeader {
 public:
  RpcHeader(const Perimortem::Memory::ManagedString& contents);

  inline constexpr auto get_version() const
      -> const Perimortem::Memory::ManagedString {
    return json_rpc;
  }
  inline constexpr auto get_method() const
      -> const Perimortem::Memory::ManagedString {
    return method;
  }
  inline constexpr auto get_id() const -> uint32_t { return id; }
  inline constexpr auto get_params_offset() const -> uint32_t {
    return params_offset;
  }

 private:
  auto parse_rpc(const Perimortem::Memory::ManagedString& contents,
                 uint32_t& position) -> void;
  auto parse_id(const Perimortem::Memory::ManagedString& contents,
                uint32_t& position) -> void;
  auto parse_method(const Perimortem::Memory::ManagedString& contents,
                    uint32_t& position) -> void;
  auto parse_params(const Perimortem::Memory::ManagedString& contents,
                    uint32_t& position) -> void;

  Perimortem::Memory::ManagedString json_rpc;
  Perimortem::Memory::ManagedString method;
  uint32_t id;
  uint32_t params_offset;
};

}  // namespace Perimortem::Storage::Json
