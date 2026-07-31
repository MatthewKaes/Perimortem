// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/package/language/source.hpp"

#include "perimortem/system/path.hpp"

#include "tetrodotoxin/package/language/parser/name.hpp"
#include "ttx/lexical/lexicon.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

// A closed String contributes only its inner path bytes. The Source parser
// keeps the Token so path diagnostics remain attached to the authored value.
static auto parse_quoted_path(Cursor& cursor, Token& token)
    -> Option<View::Bytes> {
  token = cursor.require(
      Code::Type::String,
      "Source paths must be closed quoted String values."_view);
  if (!token) {
    return {};
  }

  View::Bytes text = token.caculate_text(cursor.get_source_text());
  if (!Lexicon::validate(Code::Type::String, text)) {
    cursor.create_token_error(
        token, "Source path String is unterminated."_view);
    return {};
  }

  return text.slice(1, text.get_size() - 2);
}

auto Package::Language::Source::parse(
    Allocator::Arena& domain,
    Cursor& cursor,
    Span& span) -> Option<Source> {
  span = Span();

  Token source = cursor.require(
      Code::Type::Source, "Expected a Package `source` statement."_view);
  if (!source) {
    cursor.recover_to_statement();
    return {};
  }

  View::Bytes local_name = Parser::Name::parse_semantic(cursor);
  if (local_name.is_empty()) {
    cursor.recover_to_statement();
    return {};
  }

  Token relation = cursor.require(
      Code::Type::Addressable,
      "Source statements require exact `from` spelling."_view);
  if (!relation) {
    cursor.recover_to_statement();
    return {};
  }

  if (relation.caculate_text(cursor.get_source_text()) != "from"_view) {
    cursor.create_token_error(
        relation, "Source statements require exact `from` spelling."_view);
    cursor.recover_to_statement();
    return {};
  }

  Token path_token;
  auto payload = parse_quoted_path(cursor, path_token);
  if (!payload) {
    cursor.recover_to_statement();
    return {};
  }

  Path normalized_path(*payload);
  if (normalized_path.get_view().is_empty()) {
    cursor.create_token_error(
        path_token, "Source path cannot be empty or lexically invalid."_view);
    cursor.recover_to_statement();
    return {};
  }

  // Keep the range invalid until the normalized path and terminating Token are
  // both complete. Recovery can then consume a boundary without publishing a
  // plausible but incomplete Source statement.
  Token consumed_end_statement = cursor.require(
      Code::Type::EndStatement,
      "Source statements require a terminating `;`."_view);
  if (!consumed_end_statement) {
    cursor.recover_to_statement();
    return {};
  }

  View::Bytes durable_path = domain.proxy(normalized_path.get_view());
  span = Span(source, consumed_end_statement);
  return Source(local_name, durable_path);
}
