// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/flow/branch.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/model/parser/pack.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto select_condition_flag(const Language::Model::Pack& condition)
    -> Option<const Language::Model::Types::Flag&> {
  return condition.get_value_type(0)
      .resolve()
      .select<Language::Model::Types::Flag>();
}

auto Language::Flow::Branch::interpret(
    Cursor& cursor,
    Block& lexical_context,
    Language::Model::Callable& function,
    const Language::Model::Type& access_scope) -> Option<Branch&> {
  Allocator::Arena& domain = cursor.get_arena();
  Token opening = cursor.current();
  Kind kind;
  switch (cursor.get_code().get_type()) {
  case Code::Type::If:
    kind = Kind::If;
    break;
  case Code::Type::While:
    kind = Kind::While;
    break;
  default:
    cursor.create_token_error(
        "Library branches require the `if` or `while` keyword."_view);
    return {};
  }
  cursor.consume();

  auto condition = Model::Parser::Pack::parse(lexical_context, cursor);
  BAIL_IF(!condition);

  Branch& result = domain.construct_from<Branch>([&]() -> Branch {
    return Branch(
        kind, *condition,
        Anchor::create(opening, Span(opening, cursor.peek(-1))));
  });

  // Loop control binds to the exact enclosing owner while syntax still exposes
  // the scope stack. A while body selects this Branch; an if body inherits the
  // nearest loop. Linking therefore never reconstructs lexical ancestry.
  Option<Reference<const Abstract>> enclosing_loop;
  if (kind == Kind::While) {
    enclosing_loop = Reference<const Abstract>(result);
  } else {
    auto inherited = lexical_context.get_enclosing_loop();
    if (inherited) {
      enclosing_loop = Reference<const Abstract>(*inherited);
    }
  }

  auto body = Block::interpret(
      cursor, lexical_context, function, access_scope, enclosing_loop);
  BAIL_IF(!body);
  result.body = Reference<Block>(*body);

  // Else retains its exact Statement so `else if` remains a Branch rather than
  // acquiring fabricated braces or a second control identity.
  if (kind == Kind::If && cursor.matches(Code::Type::Else)) {
    cursor.consume();
    const Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    if (cursor.matches(Code::Type::If)) {
      auto parsed =
          Branch::interpret(cursor, lexical_context, function, access_scope);
      BAIL_IF(!parsed);
      result.alternate = Statement::create(
          *parsed, documentation, parsed->get_anchor(),
          [](Branch& selected, Cursor& operation_cursor, Scope& scope) {
            return selected.link(
                operation_cursor, scope, scope.get_access_scope());
          },
          [](Branch& selected, Cursor& operation_cursor) {
            selected.finalize(operation_cursor);
          },
          [](const Branch& selected) {
            return selected.reaches_next_statement();
          });
    } else if (cursor.matches(Code::Type::ScopeStart)) {
      auto parsed = Block::interpret(
          cursor, lexical_context, function, access_scope, enclosing_loop);
      BAIL_IF(!parsed);
      result.alternate = Statement::create(
          *parsed, documentation, parsed->get_anchor(),
          [](Block& selected, Cursor& operation_cursor, Scope&) {
            return selected.link(operation_cursor);
          },
          [](Block& selected, Cursor& operation_cursor) {
            selected.finalize(operation_cursor);
          },
          [](const Block& selected) {
            return selected.reaches_next_statement();
          });
    } else {
      cursor.create_token_error(
          "Library `else` requires one nested `if` or Block."_view);
      return {};
    }
  }

  Token closing = cursor.peek(-1);
  result.anchor = Anchor::create(opening, Span(opening, closing));
  return result;
}

auto Language::Flow::Branch::link(
    Ttx::Lexical::Cursor& cursor,
    Scope& lexical_context,
    const Language::Model::Type& access_scope) -> Bool {
  if (linked) {
    return True;
  }
  BAIL_IF(!body);

  Model::Pack& retained_condition = condition.get();
  BAIL_IF(!retained_condition.link(cursor, lexical_context, access_scope));
  // Branch observes value flow rather than the exact identities used by
  // postfix access. Prove that distinction before selecting the leading Flag.
  if (&retained_condition.resolve() != &retained_condition) {
    cursor.create_expression_error(
        anchor, "Branch condition did not produce value flow."_view,
        "Use a Type result only as an access receiver."_view);
    return False;
  }
  if (!select_condition_flag(retained_condition)) {
    cursor.create_expression_error(
        anchor, "Branch condition must produce a Flag as its first value."_view,
        "Keep any additional Pack values after one leading Flag value."_view);
    return False;
  }

  Bool failed = !body->get().link(cursor);
  alternate.visit(
      []() {},
      [&](Statement& selected) {
        failed |= !selected.link(cursor, lexical_context);
      });
  BAIL_IF(failed);

  linked = True;
  return True;
}

auto Language::Flow::Branch::finalize(Cursor& cursor) -> void {
  condition.get().finalize(cursor);
  body.visit(
      []() {},
      [&](Reference<Block>& selected) { selected.get().finalize(cursor); });
  alternate.visit(
      []() {}, [&](Statement& selected) { selected.finalize(cursor); });
}

auto Language::Flow::Branch::reaches_next_statement() const -> Bool {
  if (kind == Kind::While || !alternate) {
    return True;
  }

  return body->get().reaches_next_statement() || alternate->reaches_next();
}
