// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/parser/declaration.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/access/type.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/documentations/merged.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

auto Parser::Declaration::parse_visibility(Cursor& cursor)
    -> Option<Visibility> {
  if (cursor.matches(Code::Type::Public)) {
    cursor.consume();
    return Visibility::Public;
  }
  if (cursor.matches(Code::Type::Private)) {
    cursor.consume();
    return Visibility::Private;
  }

  cursor.create_token_error(
      "Library declarations require `public` or `private` visibility."_view);
  return {};
}

static auto can_bind(const Types::Structure& host, const Abstract& declaration)
    -> Bool {
  return host.visit<Types::Source>(
      [&](const Types::Source& source) {
        return source.can_bind_static(declaration);
      },
      [&](const Abstract&) { return host.can_bind_member(declaration); });
}

static auto bind(
    Types::Structure& host,
    Monograph& source,
    Abstract& declaration,
    Visibility visibility) -> Bool {
  return host.visit<Types::Source>(
      [&](Types::Source&) {
        return source.bind_static(declaration, visibility);
      },
      [&](Abstract&) { return host.bind_member(declaration, visibility); });
}

static auto retain_field(Types::Structure& host, Field::Source field) -> Bool {
  return host.visit<Types::Source>(
      [&](Types::Source& source) { return source.retain_static_field(field); },
      [&](Abstract&) { return host.retain_field(field); });
}

static auto parse_alias(
    Allocator::Arena& domain,
    Cursor& cursor,
    const Documentation& documentation,
    Monograph& source,
    Types::Structure& host) -> Bool {
  auto visibility = Parser::Declaration::parse_visibility(cursor);
  BAIL_IF(!visibility);

  Token name_token = cursor.require(
      Code::Type::Type,
      "Library Aliases require an authored Type shaped name."_view);
  BAIL_IF(!name_token);
  BAIL_IF(!cursor.require(
      Code::Type::Define,
      "Library Alias names require `:` before `alias`."_view));
  BAIL_IF(!cursor.require(
      Code::Type::Alias, "Library Alias declarations require `alias`."_view));
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "Library Alias declarations require `=` before their Type route."_view));

  auto route = Tetrodotoxin::Library::Language::Access::Type::parse(cursor);
  BAIL_IF(!route);

  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Library Alias declarations require one terminating `;`."_view);
  BAIL_IF(!terminator);

  // Structure owns authenticated Type lookup for both its local declarations
  // and its enclosing source chain. The parser only proves the terminal Type,
  // leaving category routing and collision policy on that same owner.
  const Abstract& resolved = host.resolve_type(*route);
  auto target = resolved.select<Ttx::Model::Type>();
  if (!target) {
    cursor.create_expression_error(
        route->get_anchor(),
        "Library Alias Type route did not resolve to one stable Type."_view);
    return False;
  }

  const Documentation* alias_documentation = &target->get_documentation();
  if (!documentation.is_empty()) {
    alias_documentation = &domain.construct<Ttx::Model::Documentations::Merged>(
        documentation, target->get_documentation());
  }

  View::Bytes name = name_token.caculate_text(cursor.get_source_text());
  Ttx::Model::Alias candidate(name, *target, *alias_documentation);
  if (!can_bind(host, candidate)) {
    cursor.create_expression_error(
        Anchor::create(name_token, Span(name_token)),
        "Library Alias name collides with an occupied Type name."_view);
    return False;
  }

  auto& alias =
      domain.construct<Ttx::Model::Alias>(name, *target, *alias_documentation);
  if (!bind(host, source, alias, *visibility)) {
    cursor.create_expression_error(
        Anchor::create(name_token, Span(name_token)),
        "Library Alias could not enter its Structure Type surface."_view);
    return False;
  }

  return True;
}

static auto parse_type(
    Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Documentation& documentation,
    Monograph& source,
    Types::Structure& host) -> Bool {
  Token declaration_kind = cursor.peek(3);
  if (declaration_kind.get_code() == Code::Type::Alias) {
    return parse_alias(domain, cursor, documentation, source, host);
  }

  if (declaration_kind.get_code() == Code::Type::Addressable &&
      declaration_kind.caculate_text(cursor.get_source_text()) == "enum"_view) {
    auto enumeration = Types::Enumeration::interpret(
        domain, cursor, documentation, source, host);
    BAIL_IF(!enumeration);
    if (!bind(host, source, *enumeration, enumeration->get_visibility())) {
      cursor.create_expression_error(
          enumeration->get_name_anchor(),
          "Duplicate Type name in this Library Structure."_view);
      return False;
    }

    return True;
  }

  if (declaration_kind.get_code() == Code::Type::Addressable) {
    View::Bytes kind = declaration_kind.caculate_text(cursor.get_source_text());
    if (kind == "struct"_view || kind == "object"_view) {
      auto structure = Types::Structure::interpret(
          domain, cursor, documentation, source, materializations, host);
      BAIL_IF(!structure);
      if (!bind(host, source, *structure, structure->get_visibility())) {
        auto name_anchor = structure->get_name_anchor();
        if (name_anchor) {
          cursor.create_expression_error(
              *name_anchor,
              "Duplicate Type name in this Library Structure."_view);
        } else {
          cursor.create_token_error(
              "Duplicate Type name in this Library Structure."_view);
        }
        return False;
      }

      return True;
    }
  }

  cursor.create_token_error(
      declaration_kind,
      "Library Type declarations require `alias`, `enum`, `struct`, or "
      "`object`."_view);
  return False;
}

auto Parser::Declaration::parse(
    Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    Monograph& source,
    Types::Structure& host) -> Bool {
  auto transaction = cursor.branch();
  const Documentation& documentation =
      Tetrodotoxin::Language::Parser::Comment::parse(transaction);

  if (transaction.matches(Code::Type::Public) ||
      transaction.matches(Code::Type::Private)) {
    Code::Type declaration = transaction.peek(1).get_code().get_type();
    if (declaration == Code::Type::Type) {
      BAIL_IF(!parse_type(
          domain, materializations, transaction, documentation, source, host));

      cursor.join(transaction);
      return True;
    }

    if (declaration == Code::Type::Func) {
      auto function = Function::reserve(
          domain, transaction, documentation, source, host, materializations);
      BAIL_IF(!function || !function->complete(transaction));
      if (!bind(host, source, *function, function->get_visibility())) {
        transaction.create_token_error(
            function->get_name_token(),
            "Duplicate Callable name in this Library Structure."_view);
        return False;
      }

      cursor.join(transaction);
      return True;
    }
  }

  auto field = Field::interpret(
      domain, materializations, transaction, documentation, host);
  BAIL_IF(!field);
  if (!retain_field(host, *field)) {
    transaction.create_expression_error(
        field->get_anchor(),
        "Duplicate Field name in this Library Structure."_view);
    return False;
  }

  cursor.join(transaction);
  return True;
}
