// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/pack.hpp"

namespace Tetrodotoxin::Syntax::Ast {

// A single @-prefixed annotation. Bare attributes are stored as an empty pack:
// @push_constant and @push_constant() have the same AST representation.
class Attribute {
 public:
  static auto parse(Context& ctx) -> Attribute;

  // Attribute name without the leading @
  auto get_name() const -> Perimortem::Core::View::Bytes { return name; }

  // Get's the attributes fields.
  // Always valid with bare attributes defaulting to an empty pack.
  auto get_fields() const -> Perimortem::Core::View::Vector<Pack::Field> {
    return pack ? pack->get_fields()
                : Perimortem::Core::View::Vector<Pack::Field>();
  }
  auto get_pack() const -> const Pack* { return pack; }

 private:
  Perimortem::Core::View::Bytes name;
  Pack* pack = nullptr;
};

}  // namespace Tetrodotoxin::Syntax::Ast
