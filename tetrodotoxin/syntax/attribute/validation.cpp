// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/attribute/validation.hpp"

#include "tetrodotoxin/syntax/ast/function.hpp"
#include "tetrodotoxin/syntax/ast/import.hpp"
#include "tetrodotoxin/syntax/ast/layout.hpp"
#include "tetrodotoxin/syntax/ast/param.hpp"
#include "tetrodotoxin/syntax/type/declaration.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Lexical;
using namespace Tetrodotoxin::Syntax;

static auto validate_layout_attrs(
    Context& ctx,
    const Dialect::Dialect& dialect,
    const Ast::Layout& layout) -> void {
  View::Vector<Ast::Param> params = layout.get_params();

  for (Count i = 0; i < params.get_size(); i++) {
    View::Vector<Ast::Attribute> attrs = params[i].get_attributes();

    for (Count j = 0; j < attrs.get_size(); j++) {
      dialect.validate_layout_attribute(ctx, attrs[j]);
    }
  }
}

static auto validate_function_attrs(
    Context& ctx,
    const Dialect::Dialect& dialect,
    const Ast::Function& func) -> void {
  validate_layout_attrs(ctx, dialect, func.get_params());
  validate_layout_attrs(ctx, dialect, func.get_returns());
}

static auto get_scope_type(const Type::Declaration* type) -> Class::Type {
  if (!type) {
    return Class::Type::EndOfStream;
  }

  const Ast::Definition& def = type->get_definition();
  if (def.get_type_ref().get_segments().is_empty()) {
    return Class::Type::EndOfStream;
  }

  return def.get_type_ref().get_segments()[0].klass.get_type();
}

static auto validate_member_attrs(
    Context& ctx,
    const Dialect::Dialect& dialect,
    Class::Type scope_type,
    const Ast::Member* member) -> void {
  if (!member) {
    return;
  }

  Attribute::Target target = {
    .scope_type = scope_type,
    .member = member,
    .type = nullptr,
  };

  View::Vector<Ast::Attribute> attrs = member->get_attributes();

  for (Count i = 0; i < attrs.get_size(); i++) {
    dialect.validate_member_attribute(ctx, target, attrs[i]);
  }
}

static auto validate_type_attrs(
    Context& ctx,
    const Dialect::Dialect& dialect,
    Class::Type scope_type,
    const Type::Declaration* type) -> void {
  if (!type) {
    return;
  }

  Attribute::Target target = {
    .scope_type = scope_type,
    .member = nullptr,
    .type = type,
  };

  View::Vector<Ast::Attribute> attrs = type->get_attributes();

  for (Count i = 0; i < attrs.get_size(); i++) {
    dialect.validate_member_attribute(ctx, target, attrs[i]);
  }

  Class::Type child_scope = get_scope_type(type);

  View::Vector<Ast::Member*> members = type->get_body().get_members();
  for (Count i = 0; i < members.get_size(); i++) {
    validate_member_attrs(ctx, dialect, child_scope, members[i]);
  }

  View::Vector<Ast::Function*> functions = type->get_body().get_functions();
  for (Count i = 0; i < functions.get_size(); i++) {
    if (functions[i]) {
      validate_function_attrs(ctx, dialect, *functions[i]);
    }
  }

  View::Vector<Type::Declaration*> types = type->get_body().get_types();
  for (Count i = 0; i < types.get_size(); i++) {
    validate_type_attrs(ctx, dialect, child_scope, types[i]);
  }
}

auto Attribute::validate_package(
    Context& ctx,
    const Dialect::Dialect& dialect,
    View::Vector<Ast::Import> imports,
    View::Vector<Ast::Member*> members,
    View::Vector<Ast::Function*> functions,
    View::Vector<Type::Declaration*> types) -> void {
  for (Count i = 0; i < imports.get_size(); i++) {
    dialect.validate_import(ctx, imports[i]);
  }

  for (Count i = 0; i < members.get_size(); i++) {
    validate_member_attrs(ctx, dialect, Class::Type::EndOfStream, members[i]);
  }

  for (Count i = 0; i < functions.get_size(); i++) {
    if (functions[i]) {
      validate_function_attrs(ctx, dialect, *functions[i]);
    }
  }

  for (Count i = 0; i < types.get_size(); i++) {
    validate_type_attrs(ctx, dialect, Class::Type::EndOfStream, types[i]);
  }
}
