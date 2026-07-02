// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/parse/import.hpp"

#include "perimortem/core/null_terminated.hpp"

auto Ttx::Parse::Import::parse(Cursor& cursor) -> Import {
  Import result;
  if (!cursor.matches(Lexical::Class::Type::Import)) {
    return result;
  }

  // Track the error count for this one import. If any required token fails, the
  // import is not returned to resolution and the cursor skips to the next
  // statement so later imports can still be checked.
  Count starting_error_count = cursor.get_errors().get_size();
  cursor.consume();
  const Lexical::Token& local_name =
      cursor.require(Lexical::Class::Type::Type, "Expected import local name."_view);
  if (cursor.get_errors().get_size() != starting_error_count) {
    cursor.recover_to_statement();
    return result;
  }

  result.local_name = local_name.get_text();
  cursor.require(Lexical::Class::Type::Define, "Expected `:` after import name."_view);
  if (cursor.get_errors().get_size() != starting_error_count) {
    cursor.recover_to_statement();
    return result;
  }

  result.dialect_name = SymbolPath::parse(cursor);
  if (cursor.get_errors().get_size() != starting_error_count) {
    cursor.recover_to_statement();
    return result;
  }

  cursor.require(
      Lexical::Class::Type::Assign, "Expected `=` after import dialect."_view);
  if (cursor.get_errors().get_size() != starting_error_count) {
    cursor.recover_to_statement();
    return result;
  }

  // A parenthesized source selector is the compact layout form for direct file
  // imports. The source envelope recognizes only `.source` here because
  // package graph policy belongs to Tetrodotoxin resolution, not the parser.
  if (cursor.matches(Lexical::Class::Type::PackingStart)) {
    cursor.consume();
    cursor.require(
        Lexical::Class::Type::AddressOp, "Expected `.source` import field."_view);
    const Lexical::Token& field = cursor.require(
        Lexical::Class::Type::Addressable, "Expected import source field name."_view);
    if (field.get_text() != "source"_view) {
      cursor.error(
          "Expected import source field named `source`."_view,
          "Use `(.source = \"path/to/file.ttx\")` for file imports."_view);
      cursor.recover_to_statement();
      return result;
    }

    cursor.require(
        Lexical::Class::Type::Assign, "Expected `=` after import source field."_view);
    if (cursor.get_errors().get_size() != starting_error_count) {
      cursor.recover_to_statement();
      return result;
    }

    const Lexical::Token& source = cursor.require(
        Lexical::Class::Type::String, "Expected string import source path."_view);
    if (cursor.get_errors().get_size() != starting_error_count) {
      cursor.recover_to_statement();
      return result;
    }

    result.file_path = inner_text(source.get_text());
    cursor.require(
        Lexical::Class::Type::PackingEnd, "Expected `)` after import source."_view);
    if (cursor.get_errors().get_size() != starting_error_count) {
      cursor.recover_to_statement();
      return result;
    }
  } else {
    // Otherwise the import source is a package path such as `TTX::Graphics`.
    // Mapping that path to an actual package file happens after the source
    // envelope has been parsed.
    result.package_path = SymbolPath::parse(cursor);
    if (cursor.get_errors().get_size() != starting_error_count) {
      cursor.recover_to_statement();
      return result;
    }
  }

  cursor.require(
      Lexical::Class::Type::EndStatement, "Expected `;` after import."_view);
  if (cursor.get_errors().get_size() != starting_error_count) {
    cursor.recover_to_statement();
  }

  return result;
}

constexpr auto Ttx::Parse::Import::inner_text(
    Perimortem::Core::View::Bytes text)
    -> Perimortem::Core::View::Bytes {
  // String tokens still include their quotes at this layer. Strip only the
  // outer pair so resolution receives the authored path bytes.
  if (text.get_size() >= 2 && text[0] == '"' &&
      text[text.get_size() - 1] == '"') {
    return text.slice(1, text.get_size() - 2);
  }

  return text;
}
