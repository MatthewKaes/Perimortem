// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/lexical/class.hpp"
#include "tetrodotoxin/syntax/ast/attribute.hpp"
#include "tetrodotoxin/syntax/ast/member.hpp"
#include "tetrodotoxin/syntax/context.hpp"

namespace Tetrodotoxin::Syntax::Type {
class Declaration;
}

namespace Tetrodotoxin::Syntax::Attribute {

struct Target {
  Lexical::Class::Type scope_type = Lexical::Class::Type::EndOfStream;
  const Ast::Member* member = nullptr;
  const Type::Declaration* type = nullptr;
};

enum class FieldKind : Bits_8 {
  Unknown,
  Kind,
  Name,
  Set,
  Slot,
};

auto has_field(
    Perimortem::Core::View::Vector<Pack::Field> fields,
    FieldKind kind) -> Bool;
auto require_named_fields(
    Context& ctx,
    const Ast::Attribute& attr,
    Perimortem::Core::View::Bytes hint) -> void;
auto require_field(
    Context& ctx,
    const Ast::Attribute& attr,
    FieldKind kind,
    Perimortem::Core::View::Bytes hint) -> void;

auto is_value_member(const Ast::Member* member) -> Bool;

auto reject_layout_attribute(Context& ctx, const Ast::Attribute& attr) -> void;
auto ignore_member_attribute(
    Context& ctx,
    const Target& target,
    const Ast::Attribute& attr) -> void;
auto ignore_layout_attribute(Context& ctx, const Ast::Attribute& attr) -> void;

}  // namespace Tetrodotoxin::Syntax::Attribute
