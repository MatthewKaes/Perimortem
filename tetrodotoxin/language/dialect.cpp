// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/dialect.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/dialect.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

Language::Dialect::Dialect(View::Bytes name) : name(name) {}

Language::Dialect::~Dialect() {}

auto Language::Dialect::find_installed(
    View::Vector<Reference<Dialect>> installed,
    View::Bytes name) -> Option<Dialect&> {
  for (Count i = 0; i < installed.get_size(); i++) {
    Dialect& dialect = installed.get_data()[i].get();
    if (dialect.get_name() == name) {
      return dialect;
    }
  }

  return {};
}

auto Language::Dialect::interpret_source(
    View::Vector<Reference<Dialect>> installed,
    Cursor& cursor,
    Abstract& context) -> Option<Monograph&> {
  // The common envelope is consumed before protocol dispatch so every Dialect
  // receives the same source documentation and Anchor contract. Concrete
  // grammar begins only after that shared ownership boundary.
  Token source_opening = cursor.current();
  if (cursor.get_code() != Code::Type::Comment) {
    cursor.require(
        Code::Type::Comment,
        "Source is missing required documentation comment. Provide at least an "
        "explicit empty comment."_view);
    return {};
  }

  const Documentation& documentation = Parser::Comment::parse(cursor);
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
        hint << installed.get_data()[i].get().get_name();
      }
    }
    hint << "."_view;
    return {};
  }

  return dialect->interpret(cursor, documentation, source_anchor, context);
}

auto Language::Dialect::encode(const Abstract&, Persistence::Profile) const
    -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes> {
  return {};
}

auto Language::Dialect::restore(
    Allocator::Arena&,
    View::Bytes,
    Persistence::Profile,
    const Documentation&,
    Abstract&) -> Option<Monograph&> {
  return {};
}

auto Language::Dialect::resolve_context(View::Bytes) const -> const Abstract& {
  return Invalid::get_invalid();
}
