// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/bootstrap/model/type.hpp"
#include "ttx/model/addressable.h"

namespace Ttx::Model {

// Addressable names typed data that another semantic object can reach. Its
// total Type answer remains Unknown until the exact nonempty Type edge exists.
// Fields, receivers, locals, external symbols, interpreted endpoints, and
// runtime objects can all share this edge while keeping their richer behavior
// with the Dialect that defines them. That Dialect also decides whether an
// address can be written, invoked as a receiver, or observed only as a value.
class Addressable : public Concept::Abstract {
 public:
  TTX_CONTRACT(Addressable, Abstract);

  static auto prove(Concept::Abstract& candidate)
      -> Perimortem::Core::Option<Addressable&>;
  static auto prove(const Concept::Abstract& candidate)
      -> Perimortem::Core::Option<const Addressable&>;

  virtual constexpr auto get_type() const
      -> const Ttx::Concept::Abstract& override = 0;

  auto negotiate_interface(const ttx_abstract* requirement) const
      -> ttx_interface override;
};

}  // namespace Ttx::Model
