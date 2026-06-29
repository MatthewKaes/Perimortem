// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/dialect/dialect.hpp"

#include "perimortem/core/static/bytes.hpp"

#include "tetrodotoxin/syntax/ast/import.hpp"
#include "tetrodotoxin/syntax/dialect/entity.hpp"
#include "tetrodotoxin/syntax/dialect/library.hpp"
#include "tetrodotoxin/syntax/dialect/shader.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Lexical;
using namespace Tetrodotoxin::Syntax;

auto Dialect::Dialect::validate_member_attribute(
    Context& ctx,
    const Attribute::Target& target,
    const Ast::Attribute& attr) const -> void {
  AttributeHandler handler = find_attribute(attr.get_name());

  if (handler.known) {
    handler.member(ctx, target, attr);
  }
}

auto Dialect::Dialect::validate_layout_attribute(
    Context& ctx,
    const Ast::Attribute& attr) const -> void {
  AttributeHandler handler = find_attribute(attr.get_name());

  if (handler.known) {
    handler.layout(ctx, attr);
  }
}

auto Dialect::Dialect::validate_import(Context&, const Ast::Import&) const
    -> void {}

auto Dialect::is_package_kind(Class kind) -> Bool {
  switch (kind.get_type()) {
  case Class::Type::Library:
  case Class::Type::Shader:
  case Class::Type::Entity:
    return True;

  default:
    return False;
  }
}

auto Dialect::resolve(Class kind) -> const Dialect& {
  static const Library library;
  static const Shader shader;
  static const Entity entity;

  switch (kind.get_type()) {
  case Class::Type::Shader:
    return shader;

  case Class::Type::Entity:
    return entity;

  case Class::Type::Library:
  default:
    return library;
  }
}
