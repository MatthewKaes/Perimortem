// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/block.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/return.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

static auto interpret_call_statement(
    Allocator::Arena& domain,
    Language::Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context) -> Option<Language::Access::Call&> {
  auto transaction = cursor.branch();
  auto expression = Language::Parser::Expression::parse(
      domain, materializations, transaction, source_context);
  BAIL_IF(!expression);

  auto call = expression->select<Language::Access::Call>();
  if (!call) {
    auto expression_anchor = expression->get_anchor();
    if (!expression_anchor) {
      transaction.create_token_error(
          "Library expression statements require authored source."_view);
      return {};
    }

    transaction.create_expression_error(
        *expression_anchor,
        "Library expression statements require one complete invocation."_view,
        "Use the value in another expression or invoke one Callable."_view);
    return {};
  }

  BAIL_IF(!transaction.require(
      Code::Type::EndStatement,
      "Library expression statements require one terminating `;`."_view));

  // Statement admission is a Block grammar decision. The Call remains the
  // exact semantic identity, and its position in this Block is the complete
  // fact that its result Layout is intentionally discarded.
  cursor.join(transaction);
  return *call;
}

auto Language::Block::interpret(
    Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    Ttx::Model::Callable& lexical_context,
    const Ttx::Model::Type& access_scope) -> Option<Block&> {
  auto transaction = cursor.branch();
  Token opening = transaction.require(
      Code::Type::ScopeStart,
      "Library Function signatures require a body beginning with `{`."_view);
  BAIL_IF(!opening);

  Block& block = domain.construct_from<Block>(
      [&]() -> Block { return Block(domain, lexical_context, access_scope); });

  Tetrodotoxin::Language::Parser::Comment::parse(transaction);
  while (!transaction.matches(Code::Type::ScopeEnd)) {
    if (transaction.matches(Code::Type::Terminal)) {
      transaction.create_token_error(
          "Library Function body reached the end of source before `}`."_view);
      return {};
    }

    if (transaction.matches(Code::Type::Return)) {
      auto returned =
          Return::interpret(domain, materializations, transaction, block);
      BAIL_IF(!returned);

      Tetrodotoxin::Language::Parser::Comment::parse(transaction);
      if (!transaction.matches(Code::Type::ScopeEnd)) {
        transaction.create_token_error(
            "A Library Function return must be the final body form."_view);
        return {};
      }

      Token closing = transaction.consume();
      block.anchor = Anchor::create(opening, Span(opening, closing));
      block.statements.insert(*returned);

      cursor.join(transaction);
      return block;
    }

    auto call =
        interpret_call_statement(domain, materializations, transaction, block);
    BAIL_IF(!call);
    block.statements.insert(*call);
    Tetrodotoxin::Language::Parser::Comment::parse(transaction);
  }

  Token closing = transaction.consume();
  block.anchor = Anchor::create(opening, Span(opening, closing));
  cursor.join(transaction);
  return block;
}

auto Language::Block::link(
    Tetrodotoxin::Language::Monograph& source,
    Materializations& materializations) -> Bool {
  if (linked) {
    return True;
  }

  Bool failed = False;
  Bool returned = False;
  for (Reference<Abstract> statement : statements.get_view()) {
    failed |= !statement.get().visit<Return>(
        [&](Return& selected) {
          returned = True;
          return selected.link(
              source, *this, materializations, access_scope,
              lexical_context.get_results());
        },
        [&](Abstract& not_return) {
          return not_return.visit<Access::Call>(
              [&](Access::Call& call) {
                // Block is the lexical context so later local declarations can
                // intercept names without changing Function parameters or host
                // access authority.
                return call.link(source, *this, materializations, access_scope);
              },
              [](Abstract&) { return False; });
        });
  }

  if (!returned && !lexical_context.get_results().is_empty()) {
    source.report(
        anchor,
        "Function result Layout requires a terminal return statement."_view,
        "Return the complete ordered values required by the Function "
        "signature."_view);
    failed = True;
  }

  linked = !failed;
  return linked;
}

auto Language::Block::finalize() -> void {
  // A retained Call is the effect to execute, not a discarded value to fold.
  // Return alone owns an Expression whose optional constant observation is
  // useful to later consumers without replacing that source edge.
  for (Reference<Abstract> statement : statements.get_view()) {
    statement.get().visit<Return>(
        [](Return& returned) { returned.finalize(); }, [](Abstract&) {});
  }
}

auto Language::Block::resolve_context(View::Bytes route) const
    -> const Abstract& {
  return lexical_context.resolve_context(route);
}
