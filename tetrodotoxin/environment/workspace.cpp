// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/workspace.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/dialect.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

Environment::Workspace::Workspace()
    : arena(),
      installed_names(arena),
      installed_dialects(arena),
      retained_monographs(arena),
      dialects(arena),
      source_monographs(arena) {}

Environment::Workspace::~Workspace() {
  // Monographs may retain both their host Dialect and Arena backed semantic
  // facts, so finish their complete destruction phase first.
  for (Count i = 0; i < retained_monographs.get_size(); i++) {
    retained_monographs[i]->~Monograph();
  }

  // Dialect state remains usable until every hosted Monograph is gone. The
  // Arena is the first member and therefore releases its pages last.
  for (Count i = 0; i < installed_dialects.get_size(); i++) {
    installed_dialects[i]->~Dialect();
  }
}

auto Environment::Workspace::import_source(
    View::Bytes semantic_name,
    View::Bytes diagnostic_path,
    View::Bytes contents,
    Ttx::Lexical::Errors& errors) -> Bool {
  // Stage 1: Retain every byte view needed by tokenization or later semantic
  // facts before either parser object begins borrowing it.
  //
  // TODO: W01 should let confined Package input read source bytes directly into
  // this Arena and enter the retained import path without proxying them again.
  // Direct caller owned View input must continue to be copied here.
  View::Bytes owned_diagnostic_path = arena.proxy(diagnostic_path);
  View::Bytes owned_contents = arena.proxy(contents);

  // An existing semantic identity is never reparsed or overwritten. Errors
  // snapshots the retained direct input before this transaction ends.
  if (source_monographs.contains(semantic_name)) {
    Dynamic::Bytes message_storage;
    Stream::Textual<Dynamic::Bytes> message(message_storage);

    message << "Semantic source "_view << semantic_name
            << " is already imported into the Workspace."_view;

    errors.set_source_context(owned_diagnostic_path, owned_contents);
    errors.create_general_error(message_storage);
    errors.clear_source_context();
    return false;
  }

  // Stage 2: Parse the universal source envelope using only Workspace owned
  // bytes. Cursor installs the exact diagnostic identity on Errors.
  Tokenizer tokenizer(arena, owned_contents, owned_diagnostic_path);
  Cursor cursor(tokenizer, errors);

  // Every source begins with authored Documentation. Absence is a parse
  // failure and creates no semantic publication.
  if (cursor.get_code() != Code::Type::Comment) {
    cursor.require(
        Code::Type::Comment,
        "Source is missing required documentation comment. Provide at least an "
        "explicit empty comment."_view);
    return false;
  }

  // Stage 3: Parse the top level Documentation and dispatch the exact authored
  // Dialect name from the same retained source. If the Dialect is empty then
  // the Dialect parser already produced a context appropriate error so don't
  // log a second layer of diagnostics.
  const Documentation& documentation = Language::Parser::Comment::parse(cursor);
  Token dialect_instruction = cursor.current();
  View::Bytes dialect_name = Language::Parser::Dialect::parse(cursor);
  if (dialect_name.is_empty()) {
    return false;
  }

  // Stage 4: Locate the Dialect in the installed group. If it isn't installed
  // then log an error message that informs the user both what was the invalid
  // Dialect provided along with what is currently installed as a hint.
  auto* dialect_entry = dialects.find(dialect_name);
  if (dialect_entry == nullptr) {
    Managed::Bytes message_storage(arena);
    Managed::Bytes hint_storage(arena);
    Stream::Textual<Managed::Bytes> message(message_storage);
    Stream::Textual<Managed::Bytes> hint(hint_storage);
    const Count installed_count = installed_names.get_size();

    message << "Unknown dialect "_view << dialect_name
            << " can't be used to interpret this source."_view;

    if (installed_count == 0) {
      hint << "No dialects are installed."_view;
    } else if (installed_count == 1) {
      hint << "Installed dialect: "_view << installed_names[0] << "."_view;
    } else {
      hint << "Installed dialects: "_view;
      for (Count i = 0; i < installed_count; i++) {
        if (i != 0) {
          hint << ", "_view;
        }

        hint << installed_names[i];
      }

      hint << "."_view;
    }

    cursor.create_token_error(
        dialect_instruction, message_storage, hint_storage);
    return false;
  }

  // Stage 5: Dialect interpretation constructs the output Monograph in this
  // Arena. For now all sources imported into the Workspace have a unified
  // lifetime.
  Option<Language::Dialect::Monograph&> interpreted =
      dialect_entry->value.interpret(arena, cursor, documentation, *this);
  if (!interpreted) {
    return false;
  }

  // Stage 6: Retain the completed Monograph exactly once, then copy and publish
  // its semantic name. Only successful interpretation reaches publication, so
  // a failed semantic name remains reusable.
  Language::Dialect::Monograph& monograph = *interpreted;
  View::Bytes owned_semantic_name = arena.proxy(semantic_name);

  if (!retained_monographs.contains(&monograph)) {
    retained_monographs.insert(&monograph);
  }

  source_monographs.launder(owned_semantic_name, monograph);
  return true;
}

auto Environment::Workspace::get_name() const -> View::Bytes {
  return "Workspace"_view;
}

auto Environment::Workspace::get_documentation() const -> const Documentation& {
  return Documentation::get_empty();
}

auto Environment::Workspace::resolve() const -> const Abstract& {
  return *this;
}

auto Environment::Workspace::resolve_context(View::Bytes route) const
    -> const Abstract& {
  return source_monographs.visit(
      route,
      [](const Language::Dialect::Monograph& selected) -> const Abstract& {
        return selected;
      },
      []() -> const Abstract& { return Invalid::get_invalid(); });
}
