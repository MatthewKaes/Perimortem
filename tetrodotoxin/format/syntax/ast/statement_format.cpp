// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/format/expression.hpp"
#include "tetrodotoxin/format/formatter.hpp"
#include "tetrodotoxin/format/syntax.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

class StatementAlign {
 public:
  Count name = 0;
  Count type = 0;
};

static auto compute_stmt_align(
    View::Vector<Ast::Statement*> stmts,
    Count start,
    Count end) -> StatementAlign {
  StatementAlign align;

  for (Count i = start; i < end; i++) {
    const Ast::Statement* stmt = stmts[i];

    if (!stmt) {
      continue;
    }

    if (!stmt->is_aligned_declaration()) {
      continue;
    }

    Count name_width = stmt->get_name().get_size();

    if (name_width > align.name) {
      align.name = name_width;
    }

    if (stmt->get_value()) {
      Count type_width = Tetrodotoxin::Format::measure(stmt->get_type_ref());

      if (type_width > align.type) {
        align.type = type_width;
      }
    }
  }

  return align;
}

static auto format_if_like(Formatter& ctx, const Ast::Statement& stmt) -> void {
  ctx << stmt.get_class().get_source_text();
  ctx << " "_view;
  if (stmt.get_cond()) {
    Tetrodotoxin::Format::format(ctx, stmt.get_cond());
  }
  ctx << " "_view;

  View::Vector<Ast::Statement*> body = stmt.get_body();
  ctx << Class::Type::ScopeStart;
  ctx.emit_newline();
  ctx.indent++;
  Bool old_in_line = ctx.in_line;
  ctx.in_line = False;
  Tetrodotoxin::Format::format_block(ctx, body);
  ctx.in_line = old_in_line;
  ctx.indent--;
  ctx.do_indent();
  ctx << Class::Type::ScopeEnd;

  View::Vector<Ast::Statement*> else_body = stmt.get_else_body();

  if (else_body.is_empty()) {
    return;
  }

  ctx << " "_view;
  ctx << Class::Type::Else;
  ctx << " "_view;

  if (else_body.get_size() == 1 &&
      else_body[0]->get_class() == Class::Type::If) {
    ctx.in_line = True;
    Tetrodotoxin::Format::format(ctx, *else_body[0]);
    ctx.in_line = False;
    return;
  }

  ctx << Class::Type::ScopeStart;
  ctx.emit_newline();
  ctx.indent++;
  Bool old_else_in_line = ctx.in_line;
  ctx.in_line = False;
  Tetrodotoxin::Format::format_block(ctx, else_body);
  ctx.in_line = old_else_in_line;
  ctx.indent--;
  ctx.do_indent();
  ctx << Class::Type::ScopeEnd;
}

auto Tetrodotoxin::Format::format(
    Formatter& ctx,
    const Ast::Statement& statement,
    Count name_width,
    Count type_width) -> void {
  Bool has_indent = !ctx.in_line;
  Bool has_newline = !ctx.in_line;

  if (has_indent) {
    ctx.do_indent();
  }

  if (statement.is_aligned_declaration()) {
    View::Bytes sigil_text = statement.get_sigil().get_source_text();

    if (!sigil_text.is_empty()) {
      ctx << sigil_text;
      ctx << " "_view;
    }

    ctx << statement.get_name();
    ctx.emit_padding(name_width, statement.get_name().get_size());
    ctx << " "_view << Class::Type::Define << " "_view;
    Tetrodotoxin::Format::format(ctx, statement.get_type_ref());

    if (statement.get_value()) {
      ctx.emit_padding(
          type_width, Tetrodotoxin::Format::measure(statement.get_type_ref()));
      ctx << " "_view << Class::Type::Assign << " "_view;
      Tetrodotoxin::Format::format(ctx, statement.get_value());
    }

    ctx << Class::Type::EndStatement;
    if (has_newline) {
      ctx.emit_newline();
    }
    return;
  }

  switch (statement.get_class().get_type()) {
  case Class::Type::Assign:
  case Class::Type::AddAssign:
  case Class::Type::SubAssign:
    if (statement.get_cond()) {
      Tetrodotoxin::Format::format(ctx, statement.get_cond());
    }
    ctx << " "_view;
    ctx << statement.get_operator().get_source_text();
    ctx << " "_view;
    if (statement.get_value()) {
      Tetrodotoxin::Format::format(ctx, statement.get_value());
    }
    ctx << Class::Type::EndStatement;
    break;

  case Class::Type::If:
  case Class::Type::CompileIf:
    format_if_like(ctx, statement);
    break;

  case Class::Type::For:
    ctx << statement.get_class().get_source_text();
    ctx << " "_view;
    Tetrodotoxin::Format::format(ctx, statement.get_layout());
    ctx << " "_view;
    ctx << Class::Type::In;
    ctx << " "_view;
    if (statement.get_cond()) {
      Tetrodotoxin::Format::format(ctx, statement.get_cond());
    }
    ctx << " "_view << Class::Type::ScopeStart;
    ctx.emit_newline();
    ctx.indent++;
    Tetrodotoxin::Format::format_block(ctx, statement.get_body());
    ctx.indent--;
    ctx.do_indent();
    ctx << Class::Type::ScopeEnd;
    break;

  case Class::Type::While:
    ctx << statement.get_class().get_source_text();
    ctx << " "_view;
    if (statement.get_cond()) {
      Tetrodotoxin::Format::format(ctx, statement.get_cond());
    }
    ctx << " "_view << Class::Type::ScopeStart;
    ctx.emit_newline();
    ctx.indent++;
    Tetrodotoxin::Format::format_block(ctx, statement.get_body());
    ctx.indent--;
    ctx.do_indent();
    ctx << Class::Type::ScopeEnd;
    break;

  case Class::Type::Match: {
    ctx << statement.get_class().get_source_text();
    ctx << " "_view;
    if (statement.get_cond()) {
      Tetrodotoxin::Format::format(ctx, statement.get_cond());
    }
    ctx << " "_view << Class::Type::ScopeStart;
    ctx.emit_newline();
    ctx.indent++;

    View::Vector<Ast::MatchCase> cases = statement.get_cases();

    for (Count i = 0; i < cases.get_size(); i++) {
      ctx.do_indent();
      ctx << Class::Type::Case;
      ctx << " "_view;

      if (cases[i].pattern) {
        Tetrodotoxin::Format::format(ctx, cases[i].pattern);
      } else {
        ctx << Class::Type::Discard;
      }

      ctx << " "_view << Class::Type::Define << " "_view
          << Class::Type::ScopeStart;
      ctx.emit_newline();
      ctx.indent++;
      Tetrodotoxin::Format::format_block(ctx, cases[i].body);
      ctx.indent--;
      ctx.do_indent();
      ctx << Class::Type::ScopeEnd;
      ctx.emit_newline();
    }

    ctx.indent--;
    ctx.do_indent();
    ctx << Class::Type::ScopeEnd;
    break;
  }

  case Class::Type::Return:
    ctx << statement.get_class().get_source_text();
    if (statement.get_value()) {
      ctx << " "_view;
      Tetrodotoxin::Format::format(ctx, statement.get_value());
    }
    ctx << Class::Type::EndStatement;
    break;

  case Class::Type::Break:
  case Class::Type::Continue:
    ctx << statement.get_class().get_source_text();
    ctx << Class::Type::EndStatement;
    break;

  case Class::Type::EndStatement:
    if (statement.get_value()) {
      Tetrodotoxin::Format::format(ctx, statement.get_value());
    }
    ctx << Class::Type::EndStatement;
    break;

  default:
    if (statement.get_value()) {
      Tetrodotoxin::Format::format(ctx, statement.get_value());
    }
    ctx << Class::Type::EndStatement;
    break;
  }

  if (has_newline) {
    ctx.emit_newline();
  }
}

auto Tetrodotoxin::Format::format_block(
    Formatter& ctx,
    View::Vector<Ast::Statement*> stmts) -> void {
  Count i = 0;
  Bool first = True;
  const Ast::Statement* previous_stmt = nullptr;

  auto should_blank = [&](const Ast::Statement* previous,
                          const Ast::Statement* current) -> Bool {
    if (first) {
      return False;
    }

    if ((previous && previous->is_block()) ||
        (current && current->is_block())) {
      return True;
    }

    if (current && current->is_aligned_declaration() &&
        (!previous || !previous->is_declaration())) {
      return True;
    }

    return False;
  };

  while (i < stmts.get_size()) {
    const Ast::Statement* stmt = stmts[i];

    if (!stmt) {
      i++;
      continue;
    }

    if (!stmt->is_aligned_declaration()) {
      if (should_blank(previous_stmt, stmt)) {
        ctx.ensure_blank();
      }

      Tetrodotoxin::Format::format(ctx, *stmt);
      previous_stmt = stmt;
      first = False;
      i++;
      continue;
    }

    Count group_end = i + 1;

    while (group_end < stmts.get_size()) {
      const Ast::Statement* next = stmts[group_end];

      if (!next || !next->is_aligned_declaration()) {
        break;
      }

      group_end++;
    }

    StatementAlign align = compute_stmt_align(stmts, i, group_end);

    if (should_blank(previous_stmt, stmt)) {
      ctx.ensure_blank();
    }

    for (Count j = i; j < group_end; j++) {
      if (stmts[j]) {
        Tetrodotoxin::Format::format(ctx, *stmts[j], align.name, align.type);
      }
    }

    previous_stmt = stmts[group_end - 1];
    first = False;
    i = group_end;
  }
}

auto Tetrodotoxin::Format::format(
    Formatter& ctx,
    const Ast::Statement& statement) -> void {
  Tetrodotoxin::Format::format(
      ctx, statement, statement.get_name().get_size(),
      Tetrodotoxin::Format::measure(statement.get_type_ref()));
}
