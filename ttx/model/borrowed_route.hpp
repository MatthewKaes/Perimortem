// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/constant.hpp"

namespace Ttx {

// BorrowedRoute gives source-owned bytes the same atomic route behavior as an
// owning Route without allocating a second copy. The source Arena must outlive
// every graph and Context which borrows the handle, matching the lifetime of
// the names from which parser-owned route metadata is derived.
class BorrowedRoute final : public Constant {
 public:
  constexpr explicit BorrowedRoute(ttx_borrowed_bytes bytes)
      : binding({.operations = {}}), bytes(bytes) {}

  auto name() const -> ttx_borrowed_bytes override;
  void route(ttx_abstract self, ttx_route_result result) const override;

 protected:
  auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation override;

 private:
  struct Binding {
    ttx_route_ops operations;
  };

  static auto select(ttx_route self) -> const BorrowedRoute&;
  static auto TTX_CALL candidate(ttx_route self) -> ttx_abstract;
  static auto TTX_CALL get_bytes(ttx_route self) -> ttx_borrowed_bytes;

  mutable Binding binding;
  const ttx_borrowed_bytes bytes;
};

}  // namespace Ttx
