// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/flow/loop_control.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

auto Language::Flow::LoopControl::interpret(
    Allocator::Arena& domain,
    Cursor& cursor,
    const Block& lexical_context) -> Option<LoopControl&> {
  auto transaction = cursor.branch();
  Token opening = transaction.current();
  Kind kind;
  if (transaction.matches(Code::Type::Break)) {
    kind = Kind::Break;
  } else if (transaction.matches(Code::Type::Continue)) {
    kind = Kind::Continue;
  } else {
    return {};
  }
  transaction.consume();

  auto target = lexical_context.get_enclosing_loop();
  if (!target) {
    transaction.create_token_error(
        opening, "Library loop control requires one enclosing loop."_view);
    return {};
  }

  Token closing = transaction.require(
      Code::Type::EndStatement,
      "Library loop control requires one terminating `;`."_view);
  BAIL_IF(!closing);

  LoopControl& result =
      domain.construct_from<LoopControl>([&]() -> LoopControl {
        return LoopControl(
            kind, *target, Anchor::create(opening, Span(opening, closing)));
      });
  cursor.join(transaction);
  return result;
}
