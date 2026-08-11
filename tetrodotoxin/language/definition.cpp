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

auto Language::Definition::parse(Cursor& cursor, Abstract& host)
    -> Option<Definition&> {
  auto transaction = cursor.branch();
  const Documentation& documentation =
      Language::Parser::Comment::parse(transaction);
  Token opening = transaction.current();
  Bool has_attributes = transaction.matches(Code::Type::Attribute);
  auto attributes = Language::Attribute::parse(transaction);
  BAIL_IF(has_attributes && attributes.is_empty());

  Token visibility_token = transaction.current();
  Visibility visibility = Visibility::Private;
  switch (visibility_token.get_code().get_type()) {
  case Code::Type::Public:
    visibility = Visibility::Public;
    break;
  case Code::Type::Private:
    visibility = Visibility::Private;
    break;
  case Code::Type::Expose:
    visibility = Visibility::Exposed;
    break;
  default:
    transaction.create_token_error(
        "Definitions require one authored visibility before their name."_view);
    return {};
  }
  transaction.consume();

  Managed::Vector<Token> modifiers(cursor.get_arena());
  while (transaction.get_code().is_evaluation_modifier()) {
    modifiers.insert(transaction.consume());
  }
  if (transaction.get_code().is_publication_modifier()) {
    transaction.create_token_error(
        transaction.current(),
        "Definitions retain exactly one authored visibility."_view);
    return {};
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
            documentation, attributes, modifiers.get_view(), visibility,
            visibility_token, name, name_token, qualifier, host,
            Anchor::create(name_token, Span(opening, qualifier)), False);
      });
  cursor.join(transaction);
  return definition;
}

auto Language::Definition::create_synthetic(
    Allocator::Arena& domain,
    const Documentation& documentation,
    Abstract& host,
    View::Bytes reserved_name,
    Visibility visibility,
    Anchor anchor) -> Definition& {
  return domain.construct_from<Definition>([&]() -> Definition {
    return Definition(
        documentation, {}, {}, visibility, {}, reserved_name, {}, {}, host,
        anchor, True);
  });
}

auto Language::Definition::complete(Token focus, Token closing) -> Bool {
  if (anchor_complete || !is_authored() || !focus || !closing ||
      !anchor.get_span()) {
    return False;
  }

  anchor = Anchor::create(focus, Span(anchor.get_span().get_start(), closing));
  anchor_complete = True;
  return True;
}
