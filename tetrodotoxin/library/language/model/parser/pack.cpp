// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/model/parser/pack.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "ttx/concept/reference.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

auto Language::Model::Parser::Pack::parse(
    Memory::Allocator::Arena& domain,
    Language::Monograph& source,
    Cursor& cursor,
    Bool force_parentheses) -> Core::Option<Language::Model::Pack&> {
  // An ordinary Pack consumer needs the complete value-flow expression so a
  // parenthesized Pack may itself receive postfix access. Expression calls the
  // forced path below only while parsing that parenthesized primary, which
  // breaks the mutual grammar recursion at one explicit delimiter boundary.
  if (!force_parentheses) {
    return Language::Parser::Expression::parse(domain, source, cursor);
  }

  if (!cursor.matches(Code::Type::PackingStart)) {
    cursor.create_token_error(
        "Library Pack requires one opening parenthesis."_view);
    return {};
  }

  auto transaction = cursor.branch();
  Token opening = transaction.consume();
  Memory::Managed::Vector<Reference<Language::Model::Pack>> entries(domain);
  Memory::Managed::Vector<Core::View::Bytes> names(domain);
  Core::Option<Bool> named;

  while (!transaction.matches(Code::Type::PackingEnd)) {
    if (transaction.matches(Code::Type::Terminal)) {
      transaction.create_expression_error(
          Span(opening, transaction.current()),
          "Library Pack requires its closing parenthesis."_view);
      return {};
    }

    Core::Option<Token> name_token;
    if (transaction.matches(Code::Type::AddressOp)) {
      transaction.consume();
      Token name = transaction.require(
          Code::Type::Addressable,
          "Named Library Pack entries require a name after `.`."_view);
      BAIL_IF(!name);
      BAIL_IF(!transaction.require(
          Code::Type::Assign,
          "Named Library Pack entries require `=` before their value."_view));

      Core::View::Bytes spelling =
          name.caculate_text(transaction.get_source_text());
      if (names.get_view().contains([&](Core::View::Bytes retained) {
            return retained == spelling;
          })) {
        transaction.create_token_error(
            name, "Duplicate name in one Library Pack."_view);
        return {};
      }
      name_token = name;
      names.insert(domain.proxy(spelling));
    }

    Bool entry_named = bool(name_token);
    if (!named) {
      named = entry_named;
    } else if (*named != entry_named) {
      transaction.create_token_error(
          "Positional and named entries cannot share one Library Pack."_view);
      return {};
    }

    // Each delimited slot consumes one complete Expression grammar. That
    // grammar may itself start with another parenthesized Pack, so nested
    // groups retain fluid flow while postfix access still binds to the whole
    // inner Pack before this separator is considered.
    auto entry =
        Language::Parser::Expression::parse(domain, source, transaction);
    BAIL_IF(!entry);
    entries.insert(*entry);

    if (transaction.matches(Code::Type::PackingEnd)) {
      break;
    }
    BAIL_IF(!transaction.require(
        Code::Type::PackingOp,
        "Library Pack entries require `,` or the closing parenthesis."_view));
    if (transaction.matches(Code::Type::PackingEnd)) {
      break;
    }
  }

  Token closing = transaction.require(
      Code::Type::PackingEnd,
      "Library Pack requires its closing parenthesis."_view);
  BAIL_IF(!closing);

  if (entries.get_size() == 1 && (!named || !*named)) {
    Language::Model::Pack& selected = entries.at(0).get();
    cursor.join(transaction);
    return selected;
  }

  Core::Option<Anchor> anchor(Anchor::create(opening, Span(opening, closing)));
  Language::Model::Pack& group = Language::Model::Pack::create_group(
      domain, entries.get_view(), names.get_view(), anchor);
  cursor.join(transaction);
  return group;
}
