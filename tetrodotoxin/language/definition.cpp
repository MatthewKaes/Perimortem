// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/language/definition.hpp"

#include "perimortem/memory/managed/vector.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

auto Language::Definition::parse(
    Cursor& cursor,
    const Documentation& documentation,
    Abstract& host) -> Option<Definition&> {
  // Definition owns the source envelope shared by every concrete declaration.
  // Dispatch has already committed to this declaration grammar. The semantic
  // owner is constructed only after its complete common prefix is accepted.
  Token opening = cursor.current();
  Bool has_attributes = cursor.matches(Code::Type::Attribute);
  auto attributes = Language::Attribute::parse(cursor);
  BAIL_IF(has_attributes && attributes.is_empty());

  Token visibility_token = cursor.current();
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
    cursor.create_token_error(
        "Definitions require one authored visibility before their name."_view);
    return {};
  }
  cursor.consume();

  Managed::Vector<Token> modifiers(cursor.get_arena());
  while (cursor.get_code().is_evaluation_modifier()) {
    modifiers.insert(cursor.consume());
  }
  if (cursor.get_code().is_publication_modifier()) {
    cursor.create_token_error(
        cursor.current(),
        "Definitions retain exactly one authored visibility."_view);
    return {};
  }

  Token name_token = cursor.current();
  if (name_token.get_code() != Code::Type::Addressable &&
      name_token.get_code() != Code::Type::Type) {
    cursor.create_token_error(
        "Definitions require one authored name after their modifiers."_view);
    return {};
  }
  cursor.consume();

  BAIL_IF(!cursor.require(
      Code::Type::Define,
      "Definitions require `:` between their name and qualifier."_view));

  Token qualifier = cursor.current();
  if (qualifier.get_code() == Code::Type::Terminal ||
      qualifier.get_code() == Code::Type::EndStatement ||
      qualifier.get_code() == Code::Type::ScopeEnd) {
    cursor.create_token_error(
        "Definitions require one qualifier after `:`."_view);
    return {};
  }

  View::Bytes name = name_token.caculate_text(cursor.get_source_text());
  // Documentation, Attributes, Tokens, and the name remain source backed. The
  // Cursor Arena gives the Definition exactly the lifetime of its candidate
  // semantic graph without copying those facts into another owner.
  Definition& definition =
      cursor.get_arena().construct_from<Definition>([&]() -> Definition {
        return Definition(
            documentation, attributes, modifiers.get_view(), visibility,
            visibility_token, name, name_token, qualifier, host,
            Anchor::create(name_token, Span(opening, qualifier)), False);
      });
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

auto Language::Definition::create_authored(
    Cursor& cursor,
    const Documentation& documentation,
    Abstract& host,
    View::Vector<Attribute> attributes,
    View::Vector<Token> modifiers,
    Visibility visibility,
    Token visibility_token,
    View::Bytes name,
    Token name_token,
    Token qualifier,
    Anchor anchor) -> Definition& {
  return cursor.get_arena().construct_from<Definition>([&]() -> Definition {
    return Definition(
        documentation, attributes, modifiers, visibility, visibility_token,
        name, name_token, qualifier, host, anchor, True);
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
