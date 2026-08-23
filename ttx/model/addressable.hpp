// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/model/type.hpp"

namespace Ttx::Model {

// Addressable names typed data that another semantic object can reach. Fields,
// receivers, locals, external symbols, interpreted endpoints, and runtime
// objects can all share this edge while keeping their richer behavior with the
// Dialect that defines them. That Dialect also decides whether an address can
// be written, invoked as a receiver, or observed only as a value.
class Addressable : public Concept::Abstract {
 public:
  TTX_CONTRACT(Addressable, Abstract);

  virtual constexpr auto get_type() const -> const Type& = 0;
};

}  // namespace Ttx::Model
