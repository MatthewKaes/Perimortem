// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/syntax/ast/param.hpp"
#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/type/ref.hpp"

namespace Tetrodotoxin::Syntax::Ast {

// Describes the return shape of a function. Three forms:
//   Value:   Vec4D or [Vec4D]     - single type, brackets optional
//   Named:   [.name : Type, ...]  - named-field layout  (uses ':' not '=')
//   Unnamed: [Type, Type, ...]    - positional tuple of 2 or more types
class Layout {
 public:
  static auto parse(Context& ctx) -> Layout;

  auto get_value_type() const -> const Type::Ref& { return value_type; }
  auto get_params() const -> Perimortem::Core::View::Vector<Param> {
    return params;
  }
  auto get_types() const -> Perimortem::Core::View::Vector<Type::Ref> {
    return types;
  }
  auto is_value() const -> Bool {
    return params.is_empty() && types.is_empty();
  }
  auto is_named() const -> Bool { return !params.is_empty(); }
  auto is_unnamed() const -> Bool { return !types.is_empty(); }

 private:
  Type::Ref value_type;
  Perimortem::Core::View::Vector<Param> params;
  Perimortem::Core::View::Vector<Type::Ref> types;
};

}  // namespace Tetrodotoxin::Syntax::Ast
