// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/lexical/class.hpp"
#include "tetrodotoxin/syntax/ast/attribute.hpp"
#include "tetrodotoxin/syntax/ast/import.hpp"
#include "tetrodotoxin/syntax/attribute/common.hpp"
#include "tetrodotoxin/syntax/context.hpp"

namespace Tetrodotoxin::Syntax::Dialect {

using MemberValidator = void (*)(
    Context& ctx,
    const Attribute::Target& target,
    const Ast::Attribute& attr);
using LayoutValidator = void (*)(Context& ctx, const Ast::Attribute& attr);

struct AttributeHandler {
  MemberValidator member = Attribute::ignore_member_attribute;
  LayoutValidator layout = Attribute::ignore_layout_attribute;
  Bool known = False;
};

class Dialect {
 public:
  virtual ~Dialect() = default;

  virtual auto get_kind() const -> Lexical::Class = 0;
  virtual auto find_attribute(Perimortem::Core::View::Bytes name) const
      -> AttributeHandler = 0;

  auto validate_member_attribute(
      Context& ctx,
      const Attribute::Target& target,
      const Ast::Attribute& attr) const -> void;
  auto validate_layout_attribute(Context& ctx, const Ast::Attribute& attr) const
      -> void;
  auto validate_import(Context& ctx, const Ast::Import& import) const -> void;
};

auto resolve(Lexical::Class kind) -> const Dialect&;
auto is_package_kind(Lexical::Class kind) -> Bool;

}  // namespace Tetrodotoxin::Syntax::Dialect
