// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/parse/source.hpp"

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/memory/managed/vector.hpp"

auto Ttx::Parse::Source::parse(
    const Lexical::Tokenizer& tokenizer,
    Perimortem::Core::View::Bytes source_path) -> Source {
  Source result;
  Cursor cursor(tokenizer);
  result.source_path = source_path;
  result.source = tokenizer.get_source();
  result.documentation = parse_documentation(cursor);
  result.dialect_name = parse_dialect(cursor);

  Perimortem::Memory::Managed::Vector<Import> imports(cursor.get_arena());
  while (!result.dialect_name.is_empty() &&
         cursor.matches(Lexical::Class::Type::Import)) {
    Count error_count = cursor.get_error_count();
    Import import = Import::parse(cursor);
    if (!import.is_empty() && cursor.get_error_count() == error_count) {
      imports.insert(import);
    }
  }

  result.imports = imports.get_view();
  result.body_token_index = cursor.get_token_index();
  result.body_tokens = cursor.remaining_tokens();
  result.errors = cursor.get_errors();
  return result;
}

auto Ttx::Parse::Source::get_source_path() const
    -> Perimortem::Core::View::Bytes {
  return source_path;
}

auto Ttx::Parse::Source::get_source() const -> Perimortem::Core::View::Bytes {
  return source;
}

auto Ttx::Parse::Source::get_documentation() const -> Documentation {
  return documentation;
}

auto Ttx::Parse::Source::get_dialect_name() const -> const TypePath& {
  return dialect_name;
}

auto Ttx::Parse::Source::get_imports() const
    -> Perimortem::Core::View::Vector<Import> {
  return imports;
}

auto Ttx::Parse::Source::get_body_tokens() const
    -> Perimortem::Core::View::Vector<Lexical::Token> {
  return body_tokens;
}

auto Ttx::Parse::Source::get_body_token_index() const -> Count {
  return body_token_index;
}

auto Ttx::Parse::Source::is_valid() const -> Bool {
  return !has_error() && !dialect_name.is_empty();
}

auto Ttx::Parse::Source::has_error() const -> Bool {
  return !errors.is_empty();
}

auto Ttx::Parse::Source::get_errors() const
    -> Perimortem::Core::View::Vector<Error> {
  return errors;
}

auto Ttx::Parse::Source::get_error_count() const -> Count {
  return errors.get_size();
}

auto Ttx::Parse::Source::parse_documentation(Cursor& cursor) -> Documentation {
  Perimortem::Memory::Managed::Vector<Perimortem::Core::View::Bytes> lines(
      cursor.get_arena());

  while (cursor.matches(Lexical::Class::Type::Comment)) {
    lines.insert(cursor.consume().get_text());
  }

  return Documentation(lines.get_view());
}

auto Ttx::Parse::Source::parse_dialect(Cursor& cursor) -> TypePath {
  cursor.require(
      Lexical::Class::Type::Dialect, "Expected source dialect header."_view);
  if (cursor.has_error()) {
    return TypePath();
  }

  cursor.require(
      Lexical::Class::Type::Define, "Expected `:` after `dialect`."_view);
  if (cursor.has_error()) {
    return TypePath();
  }

  TypePath dialect = TypePath::parse(cursor);
  if (cursor.has_error()) {
    return TypePath();
  }

  cursor.require(
      Lexical::Class::Type::EndStatement,
      "Expected `;` after dialect header."_view);
  return dialect;
}
