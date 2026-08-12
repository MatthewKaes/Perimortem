// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/block.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/local.hpp"
#include "tetrodotoxin/library/language/model/parser/pack.hpp"
#include "tetrodotoxin/library/language/return.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

static auto interpret_call_statement(
    Allocator::Arena& domain,
    Language::Monograph& source,
    Cursor& cursor) -> Option<Language::Access::Call&> {
  auto transaction = cursor.branch();
  Token opening = transaction.current();
  auto pack = Language::Model::Parser::Pack::parse(domain, source, transaction);
  BAIL_IF(!pack);

  auto call = pack->select<Language::Access::Call>();
  if (!call) {
    transaction.create_expression_error(
        Span(opening, transaction.peek(-1)),
        "Library invocation statements require one complete invocation."_view,
        "Use the Pack in another value flow or invoke one Callable."_view);
    return {};
  }

  BAIL_IF(!transaction.require(
      Code::Type::EndStatement,
      "Library invocation statements require one terminating `;`."_view));

  // Statement admission is a Block grammar decision made after the generic
  // value-flow parser returns its exact Pack. A parenthesized scalar Call is
  // still that Call, while a composed Pack is not treated as an invocation
  // merely because one of its children invokes. Block membership remains the
  // complete fact that this exact Call's result Layout is discarded.
  cursor.join(transaction);
  return *call;
}

auto Language::Block::interpret(
    Allocator::Arena& domain,
    Monograph& source,
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

    if (transaction.matches(Code::Type::State) ||
        transaction.matches(Code::Type::Const)) {
      Token local_name = transaction.peek(1);
      if (local_name.get_code() == Code::Type::Addressable) {
        View::Bytes spelling =
            local_name.caculate_text(transaction.get_source_text());
        for (Reference<Abstract> statement : block.statements.get_view()) {
          auto retained = statement.get().select<Local>();
          if (retained && retained->get_name() == spelling) {
            transaction.create_token_error(
                local_name,
                "A Library Block cannot declare one Local name twice."_view);
            return {};
          }
        }
      }

      auto local = Local::interpret(domain, source, transaction, block);
      BAIL_IF(!local);

      block.statements.insert(*local);
      Tetrodotoxin::Language::Parser::Comment::parse(transaction);
      continue;
    }

    if (transaction.matches(Code::Type::Return)) {
      auto returned = Return::interpret(domain, source, transaction);
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

    auto call = interpret_call_statement(domain, source, transaction);
    BAIL_IF(!call);
    block.statements.insert(*call);
    Tetrodotoxin::Language::Parser::Comment::parse(transaction);
  }

  Token closing = transaction.consume();
  block.anchor = Anchor::create(opening, Span(opening, closing));
  cursor.join(transaction);
  return block;
}

auto Language::Block::link(Tetrodotoxin::Language::Monograph& source) -> Bool {
  if (linked) {
    return True;
  }

  Bool failed = False;
  Bool returned = False;
  visible_statement_count = 0;
  auto retained_statements = statements.get_view();
  for (Count i = 0; i < retained_statements.get_size(); i++) {
    visible_statement_count = i;
    Abstract& statement = retained_statements.get_data()[i].get();
    failed |= !statement.visit<Local>(
        [&](Local& local) { return local.link(source, access_scope); },
        [&](Abstract& not_local) {
          return not_local.visit<Return>(
              [&](Return& selected) {
                returned = True;
                return selected.link(
                    source, *this, access_scope, lexical_context.get_results());
              },
              [&](Abstract& not_return) {
                return not_return.visit<Access::Call>(
                    [&](Access::Call& call) {
                      // Block owns lexical declaration order while the
                      // Function host remains separate access authority.
                      return call.link(source, *this, access_scope);
                    },
                    [](Abstract&) { return False; });
              });
        });
  }
  visible_statement_count = retained_statements.get_size();

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
  // Return owns its complete Pack and finalizes every real producer through
  // that value-flow owner without replacing the terminal statement identity.
  for (Reference<Abstract> statement : statements.get_view()) {
    statement.get().visit<Local>(
        [](Local& local) { local.finalize(); },
        [](Abstract& not_local) {
          not_local.visit<Return>(
              [](Return& returned) { returned.finalize(); },
              [](Abstract& not_return) {
                not_return.visit<Access::Call>(
                    [](Access::Call& call) { call.finalize(); },
                    [](Abstract&) {});
              });
        });
  }
}

auto Language::Block::resolve_context(View::Bytes route) const
    -> const Abstract& {
  auto retained_statements = statements.get_view();
  Count visible = visible_statement_count < retained_statements.get_size()
                      ? visible_statement_count
                      : retained_statements.get_size();
  for (Count i = visible; i > 0; i--) {
    auto local = retained_statements.get_data()[i - 1].get().select<Local>();
    if (!local || local->get_name() != route) {
      continue;
    }

    const Abstract& resolved = local->resolve();
    if (resolved.is<Invalid>()) {
      return Invalid::get_invalid();
    }

    return *local;
  }

  return lexical_context.resolve_context(route);
}
