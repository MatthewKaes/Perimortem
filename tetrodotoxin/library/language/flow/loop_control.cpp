// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/flow/loop_control.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

auto Language::Flow::LoopControl::interpret(
    Cursor& cursor,
    const Block& lexical_context) -> Option<LoopControl&> {
  Allocator::Arena& domain = cursor.get_arena();
  Token opening = cursor.current();
  Kind kind;
  if (cursor.matches(Code::Type::Break)) {
    kind = Kind::Break;
  } else if (cursor.matches(Code::Type::Continue)) {
    kind = Kind::Continue;
  } else {
    cursor.create_token_error(
        "Library loop control requires `break` or `continue`."_view);
    return {};
  }
  cursor.consume();

  auto target = lexical_context.get_enclosing_loop();
  if (!target) {
    cursor.create_token_error(
        opening, "Library loop control requires one enclosing loop."_view);
    return {};
  }

  Token closing = cursor.require(
      Code::Type::EndStatement,
      "Library loop control requires one terminating `;`."_view);
  BAIL_IF(!closing);

  LoopControl& result =
      domain.construct_from<LoopControl>([&]() -> LoopControl {
        return LoopControl(
            kind, *target, Anchor::create(opening, Span(opening, closing)));
      });
  return result;
}

auto Language::Flow::LoopControl::lower(Llvm::Builder& body) const -> Bool {
  if (kind == Kind::Break) {
    return body.break_loop(*this, target.get());
  }

  return body.continue_loop(*this, target.get());
}
