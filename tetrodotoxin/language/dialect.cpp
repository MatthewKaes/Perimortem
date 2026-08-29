// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/dialect.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/dialect.hpp"
#include "tetrodotoxin/language/parser/import.hpp"
#include "ttx/bootstrap/concept/unknown.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

Language::Dialect::Dialect(View::Bytes name) : name(name) {}

Language::Dialect::~Dialect() {}

auto Language::Dialect::find_installed(
    View::Vector<Dialect*> installed,
    View::Bytes name) -> Option<Dialect&> {
  for (Count i = 0; i < installed.get_size(); i++) {
    Dialect& dialect = *installed.get_data()[i];
    if (dialect.get_name() == name) {
      return dialect;
    }
  }

  return {};
}

auto Language::Dialect::interpret_source(
    View::Vector<Dialect*> installed,
    Cursor& cursor,
    Abstract& context) -> Option<Monograph&> {
  // The common envelope is consumed before protocol dispatch so every Dialect
  // receives the same source documentation and Anchor contract. Concrete
  // grammar begins only after that shared ownership boundary.
  Token source_opening = cursor.current();
  if (!cursor.get_code().is_comment()) {
    cursor.require(
        Code::Type::Comment,
        "Source is missing required documentation comment. Provide at least an "
        "explicit empty comment."_view);
    return {};
  }

  const Documentation& documentation = Parser::Comment::parse(cursor);
  if (documentation.is_empty()) {
    cursor.create_token_error(
        source_opening,
        "Source is missing required documentation comment. Raw comments do "
        "not become Documentation."_view);
    return {};
  }
  Token dialect_declaration = cursor.current();
  View::Bytes dialect_name = Parser::Dialect::parse(cursor);
  if (dialect_name.is_empty()) {
    return {};
  }

  Anchor source_anchor = Anchor::create(
      dialect_declaration, Span(source_opening, cursor.peek(-1)));
  Option<Dialect&> dialect = find_installed(installed, dialect_name);
  if (!dialect) {
    // Dispatch is exact installed name routing. Listing the same live instances
    // in the diagnostic avoids a second registry or an implied fallback rule.
    auto report = cursor.create_report(Span(dialect_declaration));
    auto& hint = report.get_hint();

    report << "Unknown dialect "_view << dialect_name
           << " can't be used to interpret this source."_view;
    hint << "Installed dialects: "_view;
    if (installed.is_empty()) {
      hint << "<None>"_view;
    } else {
      for (Count i = 0; i < installed.get_size(); i++) {
        if (i != 0) {
          hint << ", "_view;
        }
        hint << installed.get_data()[i]->get_name();
      }
    }
    hint << "."_view;
    return {};
  }

  Managed::Vector<Import::Description> imports(cursor.get_arena());
  while (Parser::Import::is_next(cursor)) {
    const Documentation& import_documentation = Parser::Comment::parse(cursor);
    auto import = Parser::Import::parse(cursor, import_documentation);
    if (import) {
      imports.insert(*import);
    } else {
      cursor.recover_to_statement();
    }
  }

  auto interpretation =
      dialect->interpret(cursor, documentation, source_anchor, context);
  BAIL_IF(!interpretation);
  for (const Import::Description& import : imports.get_view()) {
    if (!interpretation->retain_import(import, cursor.get_associations())) {
      cursor.create_expression_error(
          import.get_declaration_anchor(),
          "Source repeats one local Import Type name."_view,
          "Give each imported source or Package one distinct local name."_view);
    }
  }
  return *interpretation;
}

auto Language::Dialect::encode(const Abstract&) const
    -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes> {
  return {};
}

auto Language::Dialect::restore(
    Allocator::Arena&,
    View::Bytes,
    const Documentation&,
    Abstract&) -> Option<Monograph&> {
  return {};
}

auto Language::Dialect::produce(
    Allocator::Arena&,
    const Abstract&,
    const Monograph&) const -> const ttx_pack* {
  return nullptr;
}

auto Language::Dialect::resolve_concept(View::Bytes route) const
    -> const Abstract& {
  return Abstract::resolve_concept(route);
}
