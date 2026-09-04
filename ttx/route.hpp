// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <cstdint>
#include <vector>

#include "ttx/abstract.hpp"

namespace Ttx {

// Route owns one complete immutable byte sequence used as an atomic concept
// question. Readable text is conventional rather than required, and embedded
// data never changes the fact that the complete sequence is one route.
class Route final : public Abstract {
 public:
  explicit Route(std::vector<uint8_t> bytes);
  Route(std::vector<uint8_t> bytes, uint64_t authority, uint64_t value);

  auto name() const -> ttx_borrowed_bytes override;
  void route(ttx_abstract self, ttx_route_result result) const override;

 protected:
  auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation override;

 private:
  struct Binding {
    ttx_route_ops operations;
    const Route* owner;
  };

  static auto select(ttx_route self) -> const Route&;
  static auto TTX_CALL candidate(ttx_route self) -> ttx_abstract;
  static auto TTX_CALL get_bytes(ttx_route self) -> ttx_borrowed_bytes;

  const Binding binding;
  const std::vector<uint8_t> bytes;
};

}  // namespace Ttx
