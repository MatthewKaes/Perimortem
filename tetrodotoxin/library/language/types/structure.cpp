// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/structure.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "ttx/concept/reference.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library::Language;

auto Types::Structure::interpret(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Option<Structure&> {
  Allocator::Arena& domain = cursor.get_arena();
  if (definition.get_name_token().get_code() != Code::Type::Type) {
    cursor.create_token_error(
        definition.get_name_token(),
        "Library Structure definitions require a Type shaped name."_view);
    return {};
  }
  if (definition.get_visibility() ==
      Tetrodotoxin::Language::Visibility::Exposed) {
    cursor.create_token_error(
        definition.get_visibility_token(),
        "Library Structures accept only `public` or `private` visibility."_view);
    return {};
  }
  if (!definition.get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_modifiers().get_data()[0],
        "Library Structures do not accept evaluation modifiers."_view);
    return {};
  }

  Token kind_token = cursor.require(
      Code::Type::Struct,
      "Library Structure definitions require the `struct` qualifier."_view);
  BAIL_IF(!kind_token);

  Structure& structure = domain.construct_from<Structure>(
      [&]() -> Structure { return Structure(domain, definition); });
  BAIL_IF(!structure.interpret_body(cursor, definition, kind_token));
  return structure;
}

auto Types::Structure::interpret_body(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition,
    Token kind_token) -> Bool {
  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart,
      "Library Composite bodies require an opening `{`."_view));

  // The exact Type exists before shared body grammar so nested Functions retain
  // its final Arena identity. A rejected body leaves that private object
  // unreachable from the enclosing Composite.
  while (!cursor.matches(Code::Type::ScopeEnd)) {
    if (cursor.matches(Code::Type::Terminal)) {
      cursor.create_token_error(
          "Library Composite body reached the end of source before `}`."_view);
      return False;
    }

    const Ttx::Concept::Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    auto nested =
        Tetrodotoxin::Language::Definition::parse(cursor, documentation, *this);
    BAIL_IF(!nested || !interpret_definition(cursor, *nested));
  }

  Token closing = cursor.consume();
  BAIL_IF(!definition.complete(kind_token, closing));

  // Field identity and source order are complete with the authored body even
  // though each Field Type links later. Publishing that real Layout here makes
  // emptiness an immutable Type fact before Signatures negotiate their shape.
  complete_field_layout();
  return True;
}

auto Types::Structure::resolve_context(View::Bytes route) const
    -> const Ttx::Concept::Abstract& {
  return Composite::resolve_context(route);
}

auto Types::Structure::create_default(Allocator::Arena& arena) const
    -> Option<Model::Pack&> {
  BAIL_IF(get_layout().is_empty());

  // Structure owns this exact instance Field inventory, so selecting Field is
  // owner-local construction rather than a consumer category switch. Authored
  // Layout order is filled from each Field initializer before asking that
  // Field's exact Type for its default.
  Managed::Vector<Ttx::Concept::Reference<Model::Pack>> values(arena);
  values.reset(get_layout().get_size());
  for (const Ttx::Concept::Reference<Ttx::Concept::Abstract>& candidate :
       get_addressables()) {
    auto field = candidate.get().select<Field>();
    if (!field || field->get_writability() != Writability::Internal) {
      continue;
    }

    auto initializer = field->get_initializer();
    if (initializer) {
      values.insert(const_cast<Model::Pack&>(*initializer));
      continue;
    }

    auto value = field->get_type().create_default(arena);
    BAIL_IF(!value);
    values.insert(*value);
  }

  return Expressions::Initializer::create_synthetic(
      arena, *this, values.get_view());
}
