// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/environment/workspace.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/language/parser/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Environment;
using namespace Tetrodotoxin::Language;

auto Workspace::import_source(
    View::Bytes route,
    View::Bytes contents,
    const Documentation& documentation,
    Ttx::Lexical::Errors& errors) -> Bool {
  // Temporarily set the error context to the workspace.
  errors.set_source_context("<Tetrodotoxin Workspace>"_view, View::Bytes());

  // Report errors if we try to import a source that already exists.
  if (source_monographs.contains(route)) {
    Static::Bytes<256> message_stroage;
    Writer::Textual error(message_stroage);

    error << "The source route "_view << route
          << " is already imported into the workspace."_view;

    errors.create_general_error(error);
    return false;
  }

  // TODO: For now just create all sources in the same arena.
  // Resolution caching might require either multiple workspaces or separate
  // arenas in order to support retained mode but for now everything is a
  // oneshot immediate mode.
  Allocator::Arena& graph_arena = arena;
  // TODO: The tokenizer just uses the `route` but we should root that in
  // Perimortem::System::Path using the root directory so emitted errors
  // actually point to real source relative to the workspace's invocation.
  Tokenizer tokenizer(graph_arena, contents, route);
  Cursor cursor(tokenizer, errors);

  // All source documents _require_ a doc comment to start by the Tetrodotoxin
  // spec so make sure it's the first token in the stream.
  if (cursor.get_code() != Code::Type::Comment) {
    cursor.require(
        Code::Type::Comment,
        "Source is missing required documentation comment. Provide at least an "
        "explicit empty comment."_view);
    return false;
  }

  // Fetch the comment and then parse the dialect.
  // If we get an empty dialect then the Dialect parser emitted a proper context
  // aware error and we can just exit early.
  auto comment = Parser::Comment::parse(cursor);
  auto starting_token = cursor.current();
  auto dialect = Parser::Dialect::parse(cursor);
  if (dialect.is_empty()) {
    return false;
  }

  // Now see if we have the dialect actually registered to interpret the rest of
  // the actual source.
  //
  // We don't do a `visit` + return pattern here since we need to do futher
  // processing if the source ends up being a `Package`. We don't check against
  // the string `Package` directly since dialects are installed under toolchain
  // names and multiple `Package` dialects can be supported for a toolchain.
  if (!dialects.contains(dialect)) {
    Static::Bytes<256> message_stroage;
    Writer::Textual error(message_stroage);

    error << "Unknown dialect "_view << dialect
          << " can't be used to interpret this source."_view;
    auto hint_location = error.get_location();
    error << "Installed dialects include "_view;
    for (Count i = 0; i < installed_dialects.get_size(); i++) {
      // Slightly better readability
      if (i == installed_dialects.get_size() - 1) {
        error << "and "_view;
      }

      error << installed_dialects[i];
      if (i != installed_dialects.get_size() - 1) {
        error << ", "_view;
      }
    }

    View::Bytes full_message = error;
    cursor.create_expression_error(
        starting_token, cursor.current(), full_message.slice(0, hint_location),
        full_message.slice(hint_location, 0));
    return false;
  }

  // Already validated that the key must exist so this is safe but still a
  // sketchy access pattern.
  auto entry = dialects.find(dialect);
  auto monograph = entry->value.interpret(graph_arena, cursor, comment, *this);
  if (!monograph) {
    return false;
  }

  // Side step the object model since we manage monograph space.
  source_monographs.launder(route, *monograph);
  auto& raw_monograph = *monograph;
  if (!raw_monograph.is<Tetrodotoxin::Package::Language::Monograph>()) {
  }

  return true;
}

auto Workspace::resolve_context(View::Bytes route) const -> const Abstract& {
  return source_monographs.visit(
      route,
      [](const Dialect::Monograph& selected) -> const Abstract& {
        return selected;
      },
      []() -> const Abstract& { return Invalid::get_invalid(); });
}
