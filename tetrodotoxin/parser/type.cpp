// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/parser/type.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/parser/builtins.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/code.hpp"
#include "ttx/lexical/token.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;

static auto unresolved_segment_error(Cursor& cursor, Token segment) -> void {
  Managed::Bytes message(cursor.get_arena());
  Stream::Textual<Managed::Bytes> output(message);
  output << "Type segment `"_view
         << segment.caculate_text(cursor.get_source_text())
         << "` does not resolve in the selected Abstract context."_view;
  cursor.create_token_error(segment, message);
}

static auto wrong_contract_error(
    Cursor& cursor,
    Token start,
    Token end,
    const Abstract& selected) -> void {
  Managed::Bytes message(cursor.get_arena());
  Stream::Textual<Managed::Bytes> output(message);
  output << "Type reference resolves to `"_view << selected.get_name()
         << "`, which does not prove the Type contract."_view;
  cursor.create_expression_error(start, end, message);
}

auto Tetrodotoxin::Parser::Type::parse(Cursor& cursor, const Abstract& context)
    -> Option<const Ttx::Model::Type&> {
  if (!cursor.matches(Code::Type::Type)) {
    Token found = cursor.consume();
    cursor.create_token_error(
        found, "Expected a Type reference."_view,
        "Type names begin with an uppercase ASCII letter."_view);
    return none;
  }

  Token start = cursor.current();
  Token end = cursor.consume();
  View::Bytes name = end.caculate_text(cursor.get_source_text());
  Option<const Ttx::Model::Type&> builtin = Builtins::find(name);
  const Abstract& first = builtin.visit(
      [&context, name](const None&) -> const Abstract& {
        return context.resolve_context(name);
      },
      [](const Ttx::Model::Type& type) -> const Abstract& { return type; });
  Reference<Abstract> selected(first);
  if (selected.get().is<Invalid>()) {
    unresolved_segment_error(cursor, end);
    return none;
  }

  while (cursor.matches(Code::Type::TypeAccessOp)) {
    cursor.consume();
    Token nested = cursor.require(
        Code::Type::Type,
        "Expected a Type name after the `::` access operator."_view);
    if (!nested.is_valid()) {
      return none;
    }

    // require() consumes the nested segment. The selected Abstract alone owns
    // its next lookup, so a failed nested route cannot fall back to the root.
    View::Bytes nested_name = nested.caculate_text(cursor.get_source_text());
    const Abstract& nested_result = selected.get().resolve_context(nested_name);
    if (nested_result.is<Invalid>()) {
      unresolved_segment_error(cursor, nested);
      return none;
    }

    selected = Reference<Abstract>(nested_result);
    end = nested;
  }

  if (cursor.matches(Code::Type::LayoutStart)) {
    cursor.create_token_error(
        "Generic Type arguments are not implemented in this parser slice."_view,
        "Restore generic arguments as the next isolated parser slice."_view);
    return none;
  }

  const Abstract& resolved = selected.get().resolve();
  if (!resolved.is<Ttx::Model::Type>()) {
    wrong_contract_error(cursor, start, end, resolved);
    return none;
  }

  return resolved.assume<Ttx::Model::Type>();
}
