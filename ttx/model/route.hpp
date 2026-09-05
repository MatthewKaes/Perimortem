// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <cstdint>
#include <vector>

#include "ttx/concept/constant.hpp"

namespace Ttx {

// Route owns one complete immutable byte sequence used as an atomic concept
// question. Readable text is conventional rather than required. A producer may
// interpret a prefix and the bytes which follow it, but the caller still asks
// one question rather than submitting parameters assembled by TTX.
class Route final : public Constant {
 public:
  explicit Route(std::vector<uint8_t> bytes);

  auto name() const -> ttx_borrowed_bytes override;
  void route(ttx_abstract self, ttx_route_result result) const override;

 protected:
  auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation override;

 private:
  struct Binding {
    ttx_route_ops operations;
  };

  static auto select(ttx_route self) -> const Route&;
  static auto TTX_CALL candidate(ttx_route self) -> ttx_abstract;
  static auto TTX_CALL get_bytes(ttx_route self) -> ttx_borrowed_bytes;

  const Binding binding;
  const std::vector<uint8_t> bytes;
};

}  // namespace Ttx
