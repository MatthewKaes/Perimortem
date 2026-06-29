// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/type/body.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/syntax/ast/declaration.hpp"
#include "tetrodotoxin/syntax/ast/definition.hpp"
#include "tetrodotoxin/syntax/ast/function.hpp"
#include "tetrodotoxin/syntax/ast/member.hpp"
#include "tetrodotoxin/syntax/type/declaration.hpp"

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

auto Type::Body::parse(
    Context& ctx,
    Class::Type scope_type,
    Class::Type end_token) -> Type::Body {
  Type::Body result;
  Managed::Vector<Ast::Member*> members(ctx.get_arena());
  Managed::Vector<Ast::Function*> functions(ctx.get_arena());
  Managed::Vector<Type::Declaration*> types(ctx.get_arena());
  Bool is_package_body = scope_type == Class::Type::EndOfStream;
  Bool seen_package_body = False;

  while (!ctx.matches(end_token) && !ctx.is_done()) {
    Count start_cursor = ctx.get_cursor();
    Ast::DeclarationPrefix prefix = Ast::DeclarationPrefix::parse(ctx);

    if (Ast::Function::starts(ctx)) {
      Ast::Function* function =
          Ast::Function::parse_declaration(ctx, scope_type, prefix);
      if (function && function->get_definition().is_valid()) {
        functions.insert(function);
      }
      if (is_package_body) {
        seen_package_body = True;
      }
    } else {
      Ast::Definition definition = Ast::Definition::parse_header(ctx);

      if (Type::Declaration::is_type_definition(definition)) {
        Class::Type type_class = get_decl_type(definition);
        if (is_package_body && type_class == Class::Type::Foreign &&
            seen_package_body) {
          ctx.error(
              "Expected Foreign declarations before package body"_view,
              "Package declarations are ordered as imports, then Foreign ABI "
              "declarations, then members, functions, and subtypes"_view);
        }

        Type::Declaration* type = Type::Declaration::parse_declaration(
            ctx, scope_type, prefix, definition);
        if (type && type->get_definition().is_valid()) {
          types.insert(type);
        }
        if (is_package_body && type_class != Class::Type::Foreign) {
          seen_package_body = True;
        }
      } else {
        Ast::Member* member =
            Ast::Member::parse_declaration(ctx, prefix, definition);
        if (member && member->get_definition().is_valid()) {
          members.insert(member);
        }
        if (is_package_body) {
          seen_package_body = True;
        }
      }
    }

    if (!ctx.matches(end_token) && !ctx.is_done() &&
        ctx.get_cursor() == start_cursor) {
      ctx.error(
          "Unable to recover type declaration"_view,
          "Skipping one token after an invalid declaration"_view);
      ctx.advance();
    }
  }

  result.members = members.get_view();
  result.functions = functions.get_view();
  result.types = types.get_view();
  return result;
}

auto Type::Body::from_members(View::Vector<Ast::Member*> members)
    -> Type::Body {
  Type::Body result;
  result.members = members;
  result.functions = View::Vector<Ast::Function*>();
  result.types = View::Vector<Type::Declaration*>();
  return result;
}

auto Type::Body::from_parts(
    View::Vector<Ast::Member*> members,
    View::Vector<Ast::Function*> functions,
    View::Vector<Type::Declaration*> types) -> Type::Body {
  Type::Body result;
  result.members = members;
  result.functions = functions;
  result.types = types;
  return result;
}
