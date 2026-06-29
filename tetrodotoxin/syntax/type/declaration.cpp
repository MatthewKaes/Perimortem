// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/type/declaration.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/syntax/ast/function.hpp"
#include "tetrodotoxin/syntax/ast/member.hpp"
#include "tetrodotoxin/syntax/ast/value.hpp"
#include "tetrodotoxin/syntax/pack.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

static auto get_decl_type(const Ast::Definition& definition) -> Class::Type {
  if (definition.get_type_ref().get_segments().is_empty()) {
    return Class::Type::EndOfStream;
  }

  return definition.get_type_ref().get_segments()[0].klass.get_type();
}

auto Type::Declaration::is_type_definition(const Ast::Definition& definition)
    -> Bool {
  switch (get_decl_type(definition)) {
  case Class::Type::Enum:
  case Class::Type::Shader:
  case Class::Type::Object:
  case Class::Type::Struct:
  case Class::Type::Foreign:
    return True;

  default:
    return False;
  }
}

auto Type::Declaration::make_enum_definition(
    View::Bytes name,
    const Type::Ref& storage_type,
    Expression::Expression* value) -> Ast::Definition {
  Ast::Definition definition;
  definition.sigil = Class::Type::ConstDynamic;
  definition.name = name;
  definition.form = Class::Type::Assign;
  definition.type_ref = storage_type;
  definition.value = Ast::Value::from_expr(value);
  return definition;
}

static auto validate_enum_storage_type(Context& ctx, const Type::Ref& type)
    -> void {
  Count storage_count = type.get_params().get_size();

  if (storage_count == 1 && !type.get_params()[0].is_size_literal()) {
    return;
  }

  ctx.error(
      "Expected enum storage type"_view,
      "Enum declarations use exactly one storage type, e.g. "
      "Enum[Bits_8](.red = 1)"_view);
}

auto Type::Declaration::parse_enum_members(Context& ctx, const Type::Ref& type)
    -> View::Vector<Ast::Member*> {
  Pack* values = Pack::parse(ctx);
  View::Vector<Pack::Field> fields =
      values ? values->get_fields() : View::Vector<Pack::Field>();
  Managed::Vector<Ast::Member*> members(ctx.get_arena());

  for (Count i = 0; i < fields.get_size(); i++) {
    if (fields[i].name.is_empty()) {
      if (fields[i].value == nullptr) {
        continue;
      }

      ctx.error(
          "Expected named enum values"_view,
          "Enum pack values use named fields, e.g. '(.red = 1, .green = 2)'"_view);
      continue;
    }

    Ast::Definition definition = make_enum_definition(
        fields[i].name, type.get_params()[0], fields[i].value);

    Ast::DeclarationPrefix prefix;
    Ast::Member* member =
        Ast::Member::parse_declaration(ctx, prefix, definition);
    members.insert(member);
  }

  return members.get_view();
}

auto Type::Declaration::parse_enum_scope(Context& ctx, const Type::Ref& type)
    -> Type::Body {
  Managed::Vector<Ast::Member*> members(ctx.get_arena());
  Managed::Vector<Ast::Function*> functions(ctx.get_arena());
  Managed::Vector<Type::Declaration*> types(ctx.get_arena());

  ctx.require(Class::Type::ScopeStart);

  while (!ctx.matches(Class::Type::ScopeEnd) && !ctx.is_done()) {
    Count start_cursor = ctx.get_cursor();
    Ast::DeclarationPrefix prefix = Ast::DeclarationPrefix::parse(ctx);

    if (Ast::Function::starts(ctx)) {
      Ast::Function* function =
          Ast::Function::parse_declaration(ctx, Class::Type::Enum, prefix);
      if (function && function->get_definition().is_valid()) {
        functions.insert(function);
      }
    } else if (
        (ctx.matches(Class::Type::Addressable) ||
         ctx.matches(Class::Type::Type)) &&
        ctx.get_peek().get_class() == Class::Type::Assign) {
      View::Bytes name = ctx.get_current().get_text();
      ctx.advance();
      ctx.require(Class::Type::Assign);
      Ast::Definition definition;
      definition.sigil = Class::Type::ConstDynamic;
      definition.name = name;
      definition.form = Class::Type::Assign;
      definition.type_ref = type.get_params()[0];
      definition.value = Ast::Value::parse(ctx);
      Ast::Member* member =
          Ast::Member::parse_declaration(ctx, prefix, definition);
      if (member && member->get_definition().is_valid()) {
        members.insert(member);
      }
      ctx.require(Class::Type::EndStatement);
    } else {
      ctx.error(
          "Expected enum value or function"_view,
          "Enum scope entries are value assignments or functions"_view);
      ctx.advance();
    }

    if (!ctx.matches(Class::Type::ScopeEnd) && !ctx.is_done() &&
        ctx.get_cursor() == start_cursor) {
      ctx.advance();
    }
  }

  ctx.require(Class::Type::ScopeEnd);
  return Type::Body::from_parts(
      members.get_view(), functions.get_view(), types.get_view());
}

auto Type::Declaration::parse_declaration(
    Context& ctx,
    Class::Type,
    const Ast::DeclarationPrefix& prefix,
    Ast::Definition definition) -> Type::Declaration* {
  Type::Declaration& node = ctx.get_arena().allocate<Type::Declaration>();
  node.disabled = prefix.disabled;
  node.form = Class::Type::EndStatement;
  node.definition = definition;
  node.body = Type::Body();
  node.attributes = prefix.attributes;
  node.documentation = prefix.documentation;

  if (prefix.external) {
    ctx.error("Expected function after 'external'"_view);
  }

  if (!node.definition.is_valid()) {
    return &node;
  }

  Class::Type type = get_decl_type(node.definition);

  if (type == Class::Type::Enum) {
    validate_enum_storage_type(ctx, node.definition.get_type_ref());
    if (ctx.matches(Class::Type::ScopeStart)) {
      node.body = parse_enum_scope(ctx, node.definition.get_type_ref());
    } else {
      node.body = Type::Body::from_members(
          parse_enum_members(ctx, node.definition.get_type_ref()));
      ctx.require(Class::Type::EndStatement);
    }
    node.form = Class::Type::Enum;
    return &node;
  }

  ctx.require(Class::Type::ScopeStart);
  node.body = Type::Body::parse(ctx, type, Class::Type::ScopeEnd);
  ctx.require(Class::Type::ScopeEnd);
  node.form = Class::Type::ScopeStart;
  return &node;
}
auto Type::Declaration::find_member(View::Bytes name) const
    -> const Ast::Member* {
  View::Vector<Ast::Member*> members = get_scope();
  for (Count i = 0; i < members.get_size(); i++) {
    if (members[i] && members[i]->get_definition().get_name() == name) {
      return members[i];
    }
  }

  return nullptr;
}

auto Type::Declaration::find_function(View::Bytes name) const
    -> const Ast::Function* {
  View::Vector<Ast::Function*> functions = get_scope_functions();
  for (Count i = 0; i < functions.get_size(); i++) {
    if (functions[i] && functions[i]->get_definition().get_name() == name) {
      return functions[i];
    }
  }

  return nullptr;
}

auto Type::Declaration::get_visible_member_count() const -> Count {
  Count count = 0;
  View::Vector<Ast::Member*> members = get_scope();
  for (Count i = 0; i < members.get_size(); i++) {
    if (members[i] && !members[i]->is_disabled()) {
      count++;
    }
  }

  return count;
}

auto Type::Declaration::get_visible_member(Count index) const
    -> const Ast::Member* {
  Count visible_index = 0;
  View::Vector<Ast::Member*> members = get_scope();
  for (Count i = 0; i < members.get_size(); i++) {
    if (!members[i] || members[i]->is_disabled()) {
      continue;
    }

    if (visible_index == index) {
      return members[i];
    }

    visible_index++;
  }

  return nullptr;
}
