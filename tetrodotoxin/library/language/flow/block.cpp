// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/flow/block.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/flow/branch.hpp"
#include "tetrodotoxin/library/language/flow/local.hpp"
#include "tetrodotoxin/library/language/flow/loop_control.hpp"
#include "tetrodotoxin/library/language/flow/match.hpp"
#include "tetrodotoxin/library/language/flow/range_loop.hpp"
#include "tetrodotoxin/library/language/flow/return.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

static auto interpret_statement(
    Cursor& cursor,
    Language::Flow::Block& block,
    Language::Model::Callable& function,
    const Language::Model::Type& access_scope,
    const Documentation& documentation) -> Option<Language::Statement> {
  // A distinct introducer commits to exactly one Statement owner. Only the
  // fallback enters Expression grammar, so a malformed selected form cannot
  // be replayed as another production.
  switch (cursor.get_code().get_type()) {
  case Code::Type::State:
  case Code::Type::Const: {
    auto local = Language::Flow::Local::interpret(cursor, block);
    BAIL_IF(!local);
    return Language::Statement::create(
        *local, documentation, local->get_anchor(),
        [](Language::Flow::Local& selected, Cursor& operation_cursor,
           Language::Flow::Scope& scope) {
          return selected.link(operation_cursor, scope.get_access_scope());
        },
        [](Language::Flow::Local& selected, Cursor& operation_cursor) {
          selected.finalize(operation_cursor);
        },
        [](const Language::Flow::Local&) { return True; },
        [](const Language::Flow::Local& selected) -> Option<View::Bytes> {
          return selected.get_name();
        },
        [](const Language::Flow::Local& selected) -> Option<const Abstract&> {
          return selected.get_linked_type() ? Option<const Abstract&>(selected)
                                            : Option<const Abstract&>();
        });
  }
  case Code::Type::Return: {
    auto returned = Language::Flow::Return::interpret(cursor, block);
    BAIL_IF(!returned);
    return Language::Statement::create(
        *returned, documentation, returned->get_anchor(),
        [](Language::Flow::Return& selected, Cursor& operation_cursor,
           Language::Flow::Scope& scope) {
          return selected.link(
              operation_cursor, scope, scope.get_access_scope(),
              scope.get_function_results());
        },
        [](Language::Flow::Return& selected, Cursor& operation_cursor) {
          selected.finalize(operation_cursor);
        },
        [](const Language::Flow::Return&) { return False; });
  }
  case Code::Type::Break:
  case Code::Type::Continue: {
    auto control = Language::Flow::LoopControl::interpret(cursor, block);
    BAIL_IF(!control);
    return Language::Statement::create(
        *control, documentation, control->get_anchor(),
        [](Language::Flow::LoopControl&, Cursor&, Language::Flow::Scope&) {
          return True;
        },
        [](Language::Flow::LoopControl&, Cursor&) {},
        [](const Language::Flow::LoopControl&) { return False; });
  }
  case Code::Type::If:
  case Code::Type::While: {
    auto branch = Language::Flow::Branch::interpret(
        cursor, block, function, access_scope);
    BAIL_IF(!branch);
    return Language::Statement::create(
        *branch, documentation, branch->get_anchor(),
        [](Language::Flow::Branch& selected, Cursor& operation_cursor,
           Language::Flow::Scope& scope) {
          return selected.link(
              operation_cursor, scope, scope.get_access_scope());
        },
        [](Language::Flow::Branch& selected, Cursor& operation_cursor) {
          selected.finalize(operation_cursor);
        },
        [](const Language::Flow::Branch& selected) {
          return selected.reaches_next_statement();
        });
  }
  case Code::Type::For: {
    auto loop = Language::Flow::RangeLoop::interpret(
        cursor, block, function, access_scope);
    BAIL_IF(!loop);
    return Language::Statement::create(
        *loop, documentation, loop->get_anchor(),
        [](Language::Flow::RangeLoop& selected, Cursor& operation_cursor,
           Language::Flow::Scope& scope) {
          return selected.link(operation_cursor, scope.get_access_scope());
        },
        [](Language::Flow::RangeLoop& selected, Cursor& operation_cursor) {
          selected.finalize(operation_cursor);
        });
  }
  case Code::Type::Match: {
    auto match =
        Language::Flow::Match::interpret(cursor, block, function, access_scope);
    BAIL_IF(!match);
    return Language::Statement::create(
        *match, documentation, match->get_anchor(),
        [](Language::Flow::Match& selected, Cursor& operation_cursor,
           Language::Flow::Scope& scope) {
          return selected.link(
              operation_cursor, scope, scope.get_access_scope());
        },
        [](Language::Flow::Match& selected, Cursor& operation_cursor) {
          selected.finalize(operation_cursor);
        },
        [](const Language::Flow::Match& selected) {
          return selected.reaches_next_statement();
        });
  }
  case Code::Type::ScopeStart: {
    auto nested = Language::Flow::Block::interpret(
        cursor, block, function, access_scope,
        block.get_enclosing_loop().visit(
            []() -> Option<Reference<const Abstract>> { return {}; },
            [](const Abstract& loop) -> Option<Reference<const Abstract>> {
              return Reference<const Abstract>(loop);
            }));
    BAIL_IF(!nested);
    return Language::Statement::create(
        *nested, documentation, nested->get_anchor(),
        [](Language::Flow::Block& selected, Cursor& operation_cursor,
           Language::Flow::Scope&) { return selected.link(operation_cursor); },
        [](Language::Flow::Block& selected, Cursor& operation_cursor) {
          selected.finalize(operation_cursor);
        },
        [](const Language::Flow::Block& selected) {
          return selected.reaches_next_statement();
        });
  }
  default:
    break;
  }

  // Expression owns every operator, including the three write operators.
  // Block supplies only Statement termination and retained membership.
  Token opening = cursor.current();
  auto expression = Language::Parser::Expression::parse(block, cursor);
  BAIL_IF(!expression);
  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Library expression statements require one terminating `;`."_view);
  BAIL_IF(!terminator);

  return Language::Statement::create(
      *expression, documentation,
      Anchor::create(opening, Span(opening, terminator)),
      [](Language::Model::Pack& selected, Cursor& operation_cursor,
         Language::Flow::Scope& scope) {
        return selected.link(operation_cursor, scope, scope.get_access_scope());
      },
      [](Language::Model::Pack& selected, Cursor& operation_cursor) {
        selected.finalize(operation_cursor);
      });
}

auto Language::Flow::Block::interpret(
    Cursor& cursor,
    const Abstract& lexical_context,
    Language::Model::Callable& function,
    const Language::Model::Type& access_scope,
    Option<Reference<const Abstract>> enclosing_loop) -> Option<Block&> {
  Allocator::Arena& domain = cursor.get_arena();
  Code::Type opening_code = cursor.get_code().get_type();
  if (opening_code != Code::Type::ScopeStart &&
      opening_code != Code::Type::Define) {
    cursor.create_token_error(
        "Library Blocks require `{` for several Statements or `:` for one "
        "Statement."_view);
    return {};
  }

  Token opening = cursor.consume();
  Bool single = opening_code == Code::Type::Define;

  // Both spellings construct the same candidate before any Statement enters
  // the graph. The delimiter changes only whether this ordered parse stops
  // after one Statement or at the matching scope end.
  Block& block = domain.construct_from<Block>([&]() -> Block {
    return Block(
        domain, lexical_context, function, access_scope, enclosing_loop);
  });

  while (single || !cursor.matches(Code::Type::ScopeEnd)) {
    if (cursor.matches(Code::Type::Terminal)) {
      cursor.create_token_error(
          single ? "Single Statement Block requires one "
                   "Statement after `:`."_view
                 : "Library Block reached the end of source "
                   "before `}`."_view);
      return {};
    }

    Token statement_start = cursor.current();
    const Documentation& documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    if (cursor.matches(Code::Type::ScopeEnd)) {
      if (documentation.is_empty()) {
        cursor.create_token_error(
            statement_start,
            "Single Statement Block requires one Statement after `:`."_view);
      } else {
        cursor.create_token_error(
            statement_start,
            "Library Statement Documentation requires one following form."_view,
            "Move this comment before the Statement it describes."_view);
      }
      return {};
    }

    auto statement = interpret_statement(
        cursor, block, function, access_scope, documentation);
    BAIL_IF(!statement);

    auto name = statement->get_binding_name();
    if (name) {
      for (const Statement& retained : block.statements.get_view()) {
        auto retained_name = retained.get_binding_name();
        if (retained_name && *retained_name == *name) {
          cursor.create_expression_error(
              statement->get_anchor(),
              "A Library Block cannot declare one Local name twice."_view,
              "Rename this Local within the current lexical scope."_view);
          return {};
        }
      }
    }

    block.statements.insert(*statement);
    if (single) {
      break;
    }
  }

  Token closing = single ? cursor.peek(-1) : cursor.consume();
  block.anchor = Anchor::create(opening, Span(opening, closing));
  return block;
}

auto Language::Flow::Block::link(Cursor& cursor) -> Bool {
  if (linked) {
    return True;
  }

  Bool failed = False;
  Bool unreachable_reported = False;
  linked_prefix_size = 0;
  auto ordered = statements.get_view();
  // One ordered pass establishes lexical visibility and reachability together.
  // The prefix advances before each owner links, so no per-Statement context
  // wrapper or concrete-kind recovery is needed.
  for (Count index = 0; index < ordered.get_size(); index++) {
    linked_prefix_size = index;
    Statement& statement = statements.at(index);
    failed |= !statement.link(cursor, *this);

    if (!unreachable_reported && !statement.reaches_next() &&
        index + 1 < ordered.get_size()) {
      cursor.create_expression_error(
          ordered.get_data()[index + 1].get_anchor(),
          "Library Statement cannot be reached from the preceding flow."_view,
          "Remove it or make the preceding control flow continue."_view);
      unreachable_reported = True;
      failed = True;
    }
  }
  linked_prefix_size = ordered.get_size();

  linked = !failed;
  return linked;
}

auto Language::Flow::Block::finalize(Cursor& cursor) -> void {
  // Statement captured the exact owner operation when grammar selected it, so
  // finalization preserves source order without rediscovering concrete kinds.
  for (Count index = 0; index < statements.get_size(); index++) {
    statements.at(index).finalize(cursor);
  }
}

auto Language::Flow::Block::lower(Llvm::Builder& body) const -> Bool {
  if (!body.begin_block(*this, anchor)) {
    return False;
  }

  for (const Statement& statement : statements.get_view()) {
    if (!statement.lower(body)) {
      return False;
    }
  }

  return body.end_block(*this);
}

auto Language::Flow::Block::reaches_next_statement() const -> Bool {
  auto ordered = statements.get_view();
  return ordered.is_empty() ||
         ordered.get_data()[ordered.get_size() - 1].reaches_next();
}

auto Language::Flow::Block::resolve_context(View::Bytes route) const
    -> const Abstract& {
  // The active prefix is a source-order link cursor: a Local becomes queryable
  // only after every preceding Statement links. Keeping this phase fact on the
  // real Block avoids a wrapper context for every Statement membership.
  auto ordered = statements.get_view();
  Count visible = linked_prefix_size < ordered.get_size() ? linked_prefix_size
                                                          : ordered.get_size();
  for (Count index = visible; index > 0; index--) {
    const Statement& statement = ordered.get_data()[index - 1];
    auto name = statement.get_binding_name();
    if (!name || *name != route) {
      continue;
    }

    auto binding = statement.get_binding();
    if (!binding) {
      return Invalid::get_invalid();
    }

    return *binding;
  }

  return lexical_context.resolve_context(route);
}
