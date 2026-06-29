// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/format/expression.hpp"
#include "tetrodotoxin/format/syntax.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

static auto measure_expr(const ExpressionNode* expr) -> Count {
  return Tetrodotoxin::Format::measure(expr);
}

auto Tetrodotoxin::Format::measure_fields(View::Vector<Pack::Field> fields)
    -> Count {
  Count width = 0;

  for (Count i = 0; i < fields.get_size(); i++) {
    if (i > 0) {
      width += Class::get_source_text(Class::Type::PackingOp).get_size() + 1;
    }

    if (!fields[i].name.is_empty()) {
      width += Class::get_source_text(Class::Type::AddressOp).get_size() +
               fields[i].name.get_size() + 1 +
               Class::get_source_text(Class::Type::Assign).get_size() + 1;
    } else if (!fields[i].index.is_empty()) {
      width += Class::get_source_text(Class::Type::AddressOp).get_size() +
               fields[i].index.get_size() + 1 +
               Class::get_source_text(Class::Type::Assign).get_size() + 1;
    }

    width += measure_expr(fields[i].value);
  }

  return width;
}

auto Tetrodotoxin::Format::measure_args(View::Vector<ExpressionNode*> args)
    -> Count {
  Count width = 0;

  for (Count i = 0; i < args.get_size(); i++) {
    if (i > 0) {
      width += Class::get_source_text(Class::Type::PackingOp).get_size() + 1;
    }

    width += measure_expr(args[i]);
  }

  return width;
}

static auto should_break_args(
    const Formatter& ctx,
    View::Vector<ExpressionNode*> args,
    Count inline_width) -> Bool {
  if (args.is_empty()) {
    return False;
  }

  return ctx.current_line_width() + inline_width >
         Tetrodotoxin::Format::soft_line_limit;
}

static auto should_break_fields(
    const Formatter& ctx,
    View::Vector<Pack::Field> fields,
    Count inline_width,
    Bool break_named_fields) -> Bool {
  if (fields.is_empty()) {
    return False;
  }

  return ctx.current_line_width() + inline_width >
             Tetrodotoxin::Format::soft_line_limit ||
         (break_named_fields &&
          (Pack::has_named_field(fields) || Pack::has_indexed_field(fields)) &&
          fields.get_size() > 1);
}

static auto format_field(
    Formatter& ctx,
    const Pack::Field& field,
    Bool compact_named_pack = False) -> void {
  if (!field.name.is_empty()) {
    ctx << Class::Type::AddressOp;
    ctx << field.name;
    ctx << " "_view << Class::Type::Assign << " "_view;
  } else if (!field.index.is_empty()) {
    ctx << Class::Type::AddressOp;
    ctx << field.index;
    ctx << " "_view << Class::Type::Assign << " "_view;
  }

  if (field.value) {
    if (compact_named_pack &&
        field.value->get_kind() == Tetrodotoxin::Syntax::Expression::Kind::Pack) {
      const auto* pack = static_cast<const Pack*>(field.value);
      ctx << Class::Type::PackingStart;
      Tetrodotoxin::Format::format_fields_after_open(
          ctx, pack->get_fields(),
          Class::get_source_text(Class::Type::PackingEnd), False, False,
          False);
      return;
    }

    Tetrodotoxin::Format::format(ctx, field.value);
  }
}

static auto choose_positional_pack_columns(View::Vector<ExpressionNode*> args)
    -> Count {
  Count size = args.get_size();

  if (size <= 2) {
    return 1;
  }

  Count rows = 1;

  for (Count candidate = 2; candidate * candidate <= size; candidate++) {
    if (size % candidate == 0) {
      rows = candidate;
    }
  }

  if (rows == 1) {
    rows = 2;
  }

  return (size + rows - 1) / rows;
}

static auto positional_pack_row_width(
    const Formatter& ctx,
    View::Vector<ExpressionNode*> args,
    Count columns,
    Count row,
    Static::Vector<Count, 8>& column_widths) -> Count {
  Count width = 0;
  Count start = row * columns;

  for (Count column = 0; column < columns; column++) {
    Count index = start + column;

    if (index >= args.get_size()) {
      break;
    }

    Count arg_width = measure_expr(args[index]);
    width += arg_width + 1;

    if (column + 1 < columns && index + 1 < args.get_size()) {
      width += 1 + (column_widths[column] - arg_width);
    }
  }

  return width;
}

static auto positional_pack_fits(
    const Formatter& ctx,
    View::Vector<ExpressionNode*> args,
    Count columns,
    Static::Vector<Count, 8>& column_widths) -> Bool {
  Count rows = (args.get_size() + columns - 1) / columns;
  Count indent_width = (ctx.indent + 1) * 2;

  for (Count row = 0; row < rows; row++) {
    if (indent_width +
            positional_pack_row_width(ctx, args, columns, row, column_widths) >
        Tetrodotoxin::Format::soft_line_limit) {
      return False;
    }
  }

  return True;
}

static auto format_positional_pack_args_after_open(
    Formatter& ctx,
    View::Vector<ExpressionNode*> args,
    View::Bytes close) -> void {
  Count columns = choose_positional_pack_columns(args);
  Static::Vector<Count, 8> column_widths;

  if (columns > 8) {
    columns = 8;
  }

  while (columns > 1) {
    for (Count i = 0; i < column_widths.get_size(); i++) {
      column_widths[i] = 0;
    }

    for (Count i = 0; i < args.get_size(); i++) {
      Count column = i % columns;
      Count width = measure_expr(args[i]);

      if (width > column_widths[column]) {
        column_widths[column] = width;
      }
    }

    if (positional_pack_fits(ctx, args, columns, column_widths)) {
      break;
    }

    columns--;
  }

  ctx.emit_newline();
  ctx.indent++;

  for (Count i = 0; i < args.get_size(); i++) {
    Count column = i % columns;

    if (column == 0) {
      ctx.do_indent();
    } else {
      ctx << " "_view;
    }

    Count width = measure_expr(args[i]);
    if (args[i]) {
      Tetrodotoxin::Format::format(ctx, args[i]);
    }
    ctx << Class::Type::PackingOp;

    if (column + 1 == columns || i + 1 == args.get_size()) {
      ctx.emit_newline();
    } else {
      ctx.emit_padding(column_widths[column] + 1, width + 1);
    }
  }

  ctx.indent--;
  ctx.do_indent();
  ctx << close;
}

auto Tetrodotoxin::Format::format_args_after_open(
    Formatter& ctx,
    View::Vector<ExpressionNode*> args,
    View::Bytes close,
    Bool spaced_inline,
    Bool positional_rows) -> void {
  Count inline_width = close.get_size();

  if (spaced_inline && !args.is_empty()) {
    inline_width += 2;
  }

  inline_width += Format::measure_args(args);

  if (should_break_args(ctx, args, inline_width)) {
    if (positional_rows && args.get_size() > 2) {
      format_positional_pack_args_after_open(ctx, args, close);
      return;
    }

    ctx.emit_newline();
    ctx.indent++;

    for (Count i = 0; i < args.get_size(); i++) {
      ctx.do_indent();
      if (args[i]) {
        Tetrodotoxin::Format::format(ctx, args[i]);
      }
      ctx << Class::Type::PackingOp;
      ctx.emit_newline();
    }

    ctx.indent--;
    ctx.do_indent();
    ctx << close;
    return;
  }

  if (spaced_inline && !args.is_empty()) {
    ctx << " "_view;
  }

  for (Count i = 0; i < args.get_size(); i++) {
    if (i > 0) {
      ctx << Class::Type::PackingOp << " "_view;
    }

    if (args[i]) {
      Tetrodotoxin::Format::format(ctx, args[i]);
    }
  }

  if (spaced_inline && !args.is_empty()) {
    ctx << " "_view;
  }

  ctx << close;
}

auto Tetrodotoxin::Format::format_fields_after_open(
    Formatter& ctx,
    View::Vector<Pack::Field> fields,
    View::Bytes close,
    Bool spaced_inline,
    Bool positional_rows,
    Bool break_named_fields) -> void {
  Count inline_width = close.get_size();

  if (spaced_inline && !fields.is_empty()) {
    inline_width += 2;
  }

  inline_width += Format::measure_fields(fields);

  if (should_break_fields(ctx, fields, inline_width, break_named_fields)) {
    ctx.emit_newline();
    ctx.indent++;

    for (Count i = 0; i < fields.get_size(); i++) {
      ctx.do_indent();
      format_field(ctx, fields[i], positional_rows);
      ctx << Class::Type::PackingOp;
      ctx.emit_newline();
    }

    ctx.indent--;
    ctx.do_indent();
    ctx << close;
    return;
  }

  if (spaced_inline && !fields.is_empty()) {
    ctx << " "_view;
  }

  for (Count i = 0; i < fields.get_size(); i++) {
    if (i > 0) {
      ctx << Class::Type::PackingOp << " "_view;
    }

    format_field(ctx, fields[i]);
  }

  if (spaced_inline && !fields.is_empty()) {
    ctx << " "_view;
  }

  ctx << close;
}

auto Tetrodotoxin::Format::format(Formatter& ctx, const Pack& pack) -> void {
  ctx << Class::Type::PackingStart;
  Format::format_fields_after_open(
      ctx, pack.get_fields(), Class::get_source_text(Class::Type::PackingEnd),
      False, True);
}

auto Tetrodotoxin::Format::measure(const Pack& pack) -> Count {
  return Class::get_source_text(Class::Type::PackingStart).get_size() +
         Format::measure_fields(pack.get_fields()) +
         Class::get_source_text(Class::Type::PackingEnd).get_size();
}
