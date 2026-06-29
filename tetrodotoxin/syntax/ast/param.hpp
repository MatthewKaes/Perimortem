// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/syntax/ast/attribute.hpp"
#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/type/ref.hpp"

namespace Tetrodotoxin::Syntax::Ast {

// A single named parameter in a function signature: .name : Type::Ref
// Attributes (@builtin = "position" etc.) may annotate individual params.
class Param {
 public:
  static auto parse(Context& ctx) -> Param;

  auto get_name() const -> Perimortem::Core::View::Bytes { return name; }
  auto get_type() const -> const Type::Ref& { return type; }

  auto get_attributes() const -> Perimortem::Core::View::Vector<Attribute> {
    return attributes;
  }

 private:
  Perimortem::Core::View::Vector<Attribute> attributes;
  Perimortem::Core::View::Bytes name;
  Type::Ref type;
};

}  // namespace Tetrodotoxin::Syntax::Ast
