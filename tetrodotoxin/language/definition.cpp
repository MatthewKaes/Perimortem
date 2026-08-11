// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/language/definition.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

auto Language::Definition::parse(Cursor& cursor) -> Option<Definition&> {
  auto transaction = cursor.branch();
  const Documentation& documentation =
      Language::Parser::Comment::parse(transaction);
  Token opening = transaction.current();
  Bool has_attributes = transaction.matches(Code::Type::Attribute);
  auto attributes = Language::Attribute::parse(transaction);
  BAIL_IF(has_attributes && attributes.is_empty());

  Managed::Vector<Token> modifiers(cursor.get_arena());
  while (transaction.get_code().is_modifier()) {
    modifiers.insert(transaction.consume());
  }

  Token name_token = transaction.current();
  if (name_token.get_code() != Code::Type::Addressable &&
      name_token.get_code() != Code::Type::Type) {
    transaction.create_token_error(
        "Definitions require one authored name after their modifiers."_view);
    return {};
  }
  transaction.consume();

  BAIL_IF(!transaction.require(
      Code::Type::Define,
      "Definitions require `:` between their name and qualifier."_view));

  Token qualifier = transaction.current();
  if (qualifier.get_code() == Code::Type::Terminal ||
      qualifier.get_code() == Code::Type::EndStatement ||
      qualifier.get_code() == Code::Type::ScopeEnd) {
    transaction.create_token_error(
        "Definitions require one qualifier after `:`."_view);
    return {};
  }

  View::Bytes name = name_token.caculate_text(transaction.get_source_text());
  Definition& definition =
      cursor.get_arena().construct_from<Definition>([&]() -> Definition {
        return Definition(
            documentation, attributes, modifiers.get_view(), name, name_token,
            qualifier, Anchor::create(name_token, Span(opening, qualifier)));
      });
  cursor.join(transaction);
  return definition;
}
