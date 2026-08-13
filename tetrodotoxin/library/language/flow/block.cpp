// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/flow/block.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/flow/assignment.hpp"
#include "tetrodotoxin/library/language/flow/branch.hpp"
#include "tetrodotoxin/library/language/flow/local.hpp"
#include "tetrodotoxin/library/language/flow/range_loop.hpp"
#include "tetrodotoxin/library/language/flow/return.hpp"
#include "tetrodotoxin/library/language/model/parser/pack.hpp"
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
  // value flow parser returns its exact Pack. A parenthesized scalar Call is
  // still that Call, while a composed Pack is not treated as an invocation
  // merely because one of its children invokes. Block membership remains the
  // complete fact that this exact Call's result Layout is discarded.
  cursor.join(transaction);
  return *call;
}

static auto link_statement(
    Abstract& statement,
    Tetrodotoxin::Language::Monograph& source,
    Language::Flow::Block& block,
    const Ttx::Model::Type& access_scope,
    Ttx::Model::Callable& function) -> Bool {
  if (auto local = statement.select<Language::Flow::Local>()) {
    return local->link(source, access_scope);
  }
  if (auto assignment = statement.select<Language::Flow::Assignment>()) {
    return assignment->link(source, block, access_scope);
  }
  if (auto branch = statement.select<Language::Flow::Branch>()) {
    return branch->link(source, block, access_scope);
  }
  if (auto loop = statement.select<Language::Flow::RangeLoop>()) {
    return loop->link(source, access_scope);
  }
  if (auto returned = statement.select<Language::Flow::Return>()) {
    return returned->link(source, block, access_scope, function.get_results());
  }
  if (auto call = statement.select<Language::Access::Call>()) {
    // Block owns lexical declaration order while the Function host remains
    // separate access authority.
    return call->link(source, block, access_scope);
  }

  return False;
}

static auto finalize_statement(Abstract& statement) -> void {
  if (auto local = statement.select<Language::Flow::Local>()) {
    local->finalize();
    return;
  }
  if (auto assignment = statement.select<Language::Flow::Assignment>()) {
    assignment->finalize();
    return;
  }
  if (auto branch = statement.select<Language::Flow::Branch>()) {
    branch->finalize();
    return;
  }
  if (auto loop = statement.select<Language::Flow::RangeLoop>()) {
    loop->finalize();
    return;
  }
  if (auto returned = statement.select<Language::Flow::Return>()) {
    returned->finalize();
    return;
  }
  if (auto call = statement.select<Language::Access::Call>()) {
    call->finalize();
  }
}

auto Language::Flow::Block::interpret(
    Allocator::Arena& domain,
    Monograph& source,
    Cursor& cursor,
    const Abstract& lexical_context,
    Ttx::Model::Callable& function,
    const Ttx::Model::Type& access_scope) -> Option<Block&> {
  auto transaction = cursor.branch();
  Token opening = transaction.require(
      Code::Type::ScopeStart,
      "Library Function signatures require a body beginning with `{`."_view);
  BAIL_IF(!opening);

  Block& block = domain.construct_from<Block>([&]() -> Block {
    return Block(domain, lexical_context, function, access_scope);
  });

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

    auto branch = Branch::interpret(
        domain, source, transaction, block, function, access_scope);
    if (branch) {
      block.statements.insert(*branch);
      Tetrodotoxin::Language::Parser::Comment::parse(transaction);
      if (!branch->reaches_next_statement() &&
          !transaction.matches(Code::Type::ScopeEnd)) {
        transaction.create_token_error(
            "A terminal Library branch must be the final body form."_view);
        return {};
      }
      continue;
    }

    auto range_loop = RangeLoop::interpret(
        domain, source, transaction, block, function, access_scope);
    if (range_loop) {
      block.statements.insert(*range_loop);
      Tetrodotoxin::Language::Parser::Comment::parse(transaction);
      continue;
    }

    auto assignment = Assignment::interpret(domain, source, transaction);
    if (assignment) {
      block.statements.insert(*assignment);
      Tetrodotoxin::Language::Parser::Comment::parse(transaction);
      continue;
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

auto Language::Flow::Block::link(Tetrodotoxin::Language::Monograph& source)
    -> Bool {
  if (linked) {
    return True;
  }

  Bool failed = False;
  visible_statement_count = 0;
  auto retained_statements = statements.get_view();
  for (Count i = 0; i < retained_statements.get_size(); i++) {
    visible_statement_count = i;
    Abstract& statement = retained_statements.get_data()[i].get();
    failed |= !link_statement(statement, source, *this, access_scope, function);
  }
  visible_statement_count = retained_statements.get_size();

  linked = !failed;
  return linked;
}

auto Language::Flow::Block::finalize() -> void {
  // A retained Call is the effect to execute, not a discarded value to fold.
  // Return owns its complete Pack and finalizes every real producer through
  // that value flow owner without replacing the terminal statement identity.
  for (Reference<Abstract> statement : statements.get_view()) {
    finalize_statement(statement.get());
  }
}

auto Language::Flow::Block::reaches_next_statement() const -> Bool {
  auto retained = statements.get_view();
  if (retained.is_empty()) {
    return True;
  }

  const Abstract& terminal = retained.get_data()[retained.get_size() - 1].get();
  return terminal.visit<Return>(
      [](const Return&) { return False; },
      [](const Abstract& not_return) {
        return not_return.visit<Branch>(
            [](const Branch& branch) {
              return branch.reaches_next_statement();
            },
            [](const Abstract&) { return True; });
      });
}

auto Language::Flow::Block::resolve_context(View::Bytes route) const
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
