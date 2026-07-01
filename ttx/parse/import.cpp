// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/parse/import.hpp"

#include "perimortem/core/null_terminated.hpp"

auto Ttx::Parse::Import::parse(Cursor& cursor) -> Import {
  Import result;
  if (!cursor.matches(Lexical::Class::Type::Import)) {
    return result;
  }

  Count starting_error_count = cursor.get_error_count();
  cursor.consume();
  const Lexical::Token& local_name =
      cursor.require(Lexical::Class::Type::Type, "Expected import local name."_view);
  if (cursor.get_error_count() != starting_error_count) {
    cursor.recover_to_statement();
    return result;
  }

  result.local_name = local_name.get_text();
  cursor.require(Lexical::Class::Type::Define, "Expected `:` after import name."_view);
  if (cursor.get_error_count() != starting_error_count) {
    cursor.recover_to_statement();
    return result;
  }

  result.dialect_name = TypePath::parse(cursor);
  if (cursor.get_error_count() != starting_error_count) {
    cursor.recover_to_statement();
    return result;
  }

  cursor.require(
      Lexical::Class::Type::Assign, "Expected `=` after import dialect."_view);
  if (cursor.get_error_count() != starting_error_count) {
    cursor.recover_to_statement();
    return result;
  }

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
    if (cursor.get_error_count() != starting_error_count) {
      cursor.recover_to_statement();
      return result;
    }

    const Lexical::Token& source = cursor.require(
        Lexical::Class::Type::String, "Expected string import source path."_view);
    if (cursor.get_error_count() != starting_error_count) {
      cursor.recover_to_statement();
      return result;
    }

    result.file_path = inner_text(source.get_text());
    cursor.require(
        Lexical::Class::Type::PackingEnd, "Expected `)` after import source."_view);
    if (cursor.get_error_count() != starting_error_count) {
      cursor.recover_to_statement();
      return result;
    }
  } else {
    result.package_path = TypePath::parse(cursor);
    if (cursor.get_error_count() != starting_error_count) {
      cursor.recover_to_statement();
      return result;
    }
  }

  cursor.require(
      Lexical::Class::Type::EndStatement, "Expected `;` after import."_view);
  if (cursor.get_error_count() != starting_error_count) {
    cursor.recover_to_statement();
  }

  return result;
}

auto Ttx::Parse::Import::get_local_name() const
    -> Perimortem::Core::View::Bytes {
  return local_name;
}

auto Ttx::Parse::Import::get_dialect_name() const -> const TypePath& {
  return dialect_name;
}

auto Ttx::Parse::Import::get_package_path() const -> const TypePath& {
  return package_path;
}

auto Ttx::Parse::Import::get_file_path() const -> Perimortem::Core::View::Bytes {
  return file_path;
}

auto Ttx::Parse::Import::is_package_source() const -> Bool {
  return !package_path.is_empty();
}

auto Ttx::Parse::Import::is_file_source() const -> Bool {
  return !file_path.is_empty();
}

auto Ttx::Parse::Import::is_empty() const -> Bool {
  return local_name.is_empty();
}

auto Ttx::Parse::Import::inner_text(Perimortem::Core::View::Bytes text)
    -> Perimortem::Core::View::Bytes {
  if (text.get_size() >= 2 && text[0] == '"' &&
      text[text.get_size() - 1] == '"') {
    return text.slice(1, text.get_size() - 2);
  }

  return text;
}
