// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/dialects/alias.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/documentations/merged.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;

auto Tetrodotoxin::Model::Dialects::Alias::unresolved_target_error(
    Cursor& cursor,
    const Token& target) -> void {
  Managed::Bytes message(cursor.get_arena());
  Stream::Textual<Managed::Bytes> output(message);
  output << "Alias target `"_view
         << target.caculate_text(cursor.get_source_text())
         << "` could not be resolved."_view;
  cursor.create_token_error(target, message);
}

auto Tetrodotoxin::Model::Dialects::Alias::resolve_target(
    Cursor& cursor,
    View::Vector<Reference<Abstract>> visible) -> const Abstract& {
  if (!cursor.matches(Code::Type::Type) &&
      !cursor.matches(Code::Type::Addressable)) {
    cursor.create_token_error("Expected an Alias target."_view);
    return Invalid::get_invalid();
  }

  const Token segment = cursor.consume();
  const View::Bytes name = segment.caculate_text(cursor.get_source_text());
  Reference<Abstract> selected(Invalid::get_invalid());

  // The first segment searches visible contexts from nearest to farthest. Once
  // selected, every later segment is resolved only by that Abstract. A route
  // therefore cannot fall back into unrelated outer Sources halfway through.
  for (Count i = 0; selected.get().is<Invalid>() && i < visible.get_size();
       i++) {
    selected = Reference<Abstract>(visible[i].get().resolve_context(name));
  }

  if (selected.get().is<Invalid>()) {
    unresolved_target_error(cursor, segment);
    return Invalid::get_invalid();
  }

  if (!cursor.matches(Code::Type::TypeAccessOp)) {
    return selected.get();
  }

  cursor.consume();
  const Static::Vector<Reference<Abstract>, 1> nested = {{selected.get()}};
  return resolve_target(cursor, nested);
}

auto Tetrodotoxin::Model::Dialects::Alias::evaluate(
    Cursor& cursor,
    View::Bytes name,
    const Documentation& documentation,
    View::Vector<Token>,
    View::Vector<Reference<Abstract>> visible) -> const Abstract& {
  cursor.consume();
  const Token assign = cursor.require(
      Code::Type::Assign, "Expected `=` before Alias target."_view);
  if (!assign.is_valid()) {
    return Invalid::get_invalid();
  }

  const Abstract& target = resolve_target(cursor, visible);
  if (target.is<Invalid>()) {
    return Invalid::get_invalid();
  }

  const Token end_statement = cursor.require(
      Code::Type::EndStatement, "Expected `;` after Alias."_view);
  if (!end_statement.is_valid()) {
    return Invalid::get_invalid();
  }

  // Authored documentation wraps the target view without changing canonical
  // identity. An empty prefix can borrow the target Documentation directly.
  const Documentation& visible_documentation =
      documentation.is_empty()
          ? target.get_documentation()
          : cursor.get_arena().construct<Ttx::Model::Documentations::Merged>(
                documentation, target.get_documentation());
  return cursor.get_arena().construct<Ttx::Model::Alias>(
      name, target, visible_documentation);
}
