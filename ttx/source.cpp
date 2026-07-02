// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/source.hpp"

#include "perimortem/memory/managed/vector.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;

auto Ttx::Source::parse(
    Parse::Cursor& cursor,
    View::Bytes source_path) -> Source {
  Source result;
  result.source_path = source_path;
  result.source = cursor.get_source();
  result.documentation = parse_documentation(cursor);
  result.dialect_name = parse_dialect(cursor);

  // Imports are still part of the envelope, but only after the dialect header
  // succeeds. Without a dialect name the rest of the file has no selected
  // owner, so source parsing leaves the cursor wherever the required header
  // failed and lets Tetrodotoxin decide whether to continue.
  Managed::Vector<Parse::Import> imports(cursor.get_arena());
  while (!result.dialect_name.is_empty() &&
         cursor.matches(Ttx::Lexical::Class::Type::Import)) {
    Count error_count = cursor.get_errors().get_size();
    Parse::Import import = Parse::Import::parse(cursor);
    // Keep parsing after a bad import so one source pass can report multiple
    // envelope mistakes. Only fully parsed imports are preserved for
    // resolution.
    if (!import.is_empty() && cursor.get_errors().get_size() == error_count) {
      imports.insert(import);
    }
  }

  result.imports = imports.get_view();
  return result;
}

auto Ttx::Source::parse_documentation(Parse::Cursor& cursor) -> Documentation {
  Managed::Vector<View::Bytes> lines(cursor.get_arena());

  // Source documentation is the leading consecutive comment run. Later
  // comments belong to the selected dialect body and are preserved for that
  // dialect to attach to types, members, functions, or dialect specific nodes.
  while (cursor.matches(Ttx::Lexical::Class::Type::Comment)) {
    lines.insert(cursor.consume().get_text());
  }

  return Documentation(lines.get_view());
}

auto Ttx::Source::parse_dialect(Parse::Cursor& cursor) -> Parse::SymbolPath {
  // The dialect header is required because it selects the parser that owns the
  // rest of the file. Recovering past a missing or malformed header would only
  // guess at the body grammar.
  Count starting_error_count = cursor.get_errors().get_size();
  cursor.require(
      Ttx::Lexical::Class::Type::Dialect,
      "Expected source dialect header."_view);
  if (cursor.get_errors().get_size() != starting_error_count) {
    return Parse::SymbolPath();
  }

  cursor.require(
      Ttx::Lexical::Class::Type::Define,
      "Expected `:` after `dialect`."_view);
  if (cursor.get_errors().get_size() != starting_error_count) {
    return Parse::SymbolPath();
  }

  Parse::SymbolPath dialect = Parse::SymbolPath::parse(cursor);
  if (cursor.get_errors().get_size() != starting_error_count) {
    return Parse::SymbolPath();
  }

  cursor.require(
      Ttx::Lexical::Class::Type::EndStatement,
      "Expected `;` after dialect header."_view);
  if (cursor.get_errors().get_size() != starting_error_count) {
    return Parse::SymbolPath();
  }

  return dialect;
}
