// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/interpreter/declarations/field.hpp"

#include "tetrodotoxin/library/interpreter/expressions/initializer.hpp"
#include "tetrodotoxin/library/interpreter/pack.hpp"
#include "tetrodotoxin/library/interpreter/type_reference.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

static auto parse_writability(
    const Tetrodotoxin::Language::Definition& definition,
    Cursor& cursor) -> Option<Language::Writability> {
  auto modifiers = definition.get_modifiers();
  if (modifiers.get_size() > 1) {
    cursor.create_token_error(
        modifiers.get_data()[1],
        "Library Fields accept at most one evaluation modifier."_view);
    return {};
  }

  Language::Writability writability = Language::Writability::Full;
  if (!modifiers.is_empty()) {
    switch (modifiers.get_data()[0].get_code().get_type()) {
    case Code::Type::State:
      writability = Language::Writability::Internal;
      break;
    case Code::Type::Const:
      writability = Language::Writability::Constant;
      break;
    default:
      cursor.create_token_error(
          modifiers.get_data()[0],
          "Library Fields accept only `state` or `const` evaluation."_view);
      return {};
    }
  }

  Tetrodotoxin::Language::Visibility visibility = definition.get_visibility();
  if (visibility == Tetrodotoxin::Language::Visibility::Exposed &&
      writability != Language::Writability::Internal) {
    cursor.create_token_error(
        definition.get_visibility_token(),
        "Library `expose` Fields require the `state` evaluation policy."_view);
    return {};
  }
  return writability;
}

auto Interpreter::Declarations::Field::parse(
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition)
    -> Option<Language::Field&> {
  if (!definition.get_host().is<Language::Model::Type>()) {
    return {};
  }

  auto writability = parse_writability(definition, cursor);
  BAIL_IF(!writability);
  if (definition.get_name_token().get_code() != Code::Type::Addressable) {
    cursor.create_token_error(
        definition.get_name_token(),
        "Library Fields require an addressable name."_view);
    return {};
  }

  Option<Language::TypeReference> type;
  Option<Language::Model::Pack&> initializer;
  if (cursor.matches(Code::Type::Assign)) {
    cursor.consume();
    if (Interpreter::Expressions::Initializer::is_next(cursor)) {
      auto object_initializer =
          Interpreter::Expressions::Initializer::parse(
              definition.get_host(), cursor);
      BAIL_IF(!object_initializer);
      initializer = *object_initializer;
    } else {
      initializer = Interpreter::Pack::parse(
          definition.get_host(), cursor);
    }
    BAIL_IF(!initializer);
  } else {
    auto authored_type =
        Interpreter::TypeReference::parse(
            definition.get_host(), cursor);
    BAIL_IF(!authored_type);
    type = *authored_type;

    if (cursor.matches(Code::Type::Assign)) {
      cursor.consume();
      if (Interpreter::Expressions::Initializer::is_next(cursor)) {
        auto object_initializer =
            Interpreter::Expressions::Initializer::parse(
                definition.get_host(), cursor);
        BAIL_IF(!object_initializer);
        initializer = *object_initializer;
      } else {
        initializer = Interpreter::Pack::parse(
            definition.get_host(), cursor);
      }
      BAIL_IF(!initializer);
    } else if (*writability == Language::Writability::Constant) {
      cursor.create_token_error(
          "Library const Fields require an initializer."_view);
      return {};
    }
  }

  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Library Fields require one terminating `;`."_view);
  BAIL_IF(!terminator);
  BAIL_IF(!definition.complete(definition.get_name_token(), terminator));
  return Language::Field::create_authored(
      cursor.get_arena(), definition, *writability, type, initializer);
}
