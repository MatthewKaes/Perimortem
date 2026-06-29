// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/format/formatter.hpp"
#include "tetrodotoxin/format/syntax.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

auto Tetrodotoxin::Format::format(
    Formatter& ctx,
    const Ast::Definition& definition) -> void {
  Tetrodotoxin::Format::format(
      ctx, definition, definition.get_name().get_size(),
      Tetrodotoxin::Format::measure(definition.get_type_ref()));
}

auto Tetrodotoxin::Format::format_header(
    Formatter& ctx,
    const Ast::Definition& definition,
    Count name_width) -> void {
  View::Bytes sigil_text = definition.get_sigil().get_source_text();

  if (!sigil_text.is_empty()) {
    ctx << sigil_text;
    ctx << " "_view;
  }

  ctx << definition.get_name();

  if (!definition.get_type_ref().is_empty()) {
    ctx.emit_padding(name_width, definition.get_name().get_size());
    ctx << " "_view << Class::Type::Define << " "_view;
    Tetrodotoxin::Format::format(ctx, definition.get_type_ref());
  }
}

auto Tetrodotoxin::Format::format(
    Formatter& ctx,
    const Ast::Definition& definition,
    Count name_width,
    Count type_width) -> void {
  Tetrodotoxin::Format::format_header(ctx, definition, name_width);

  switch (definition.get_form().get_type()) {
  case Class::Type::EndStatement:
    ctx << Class::Type::EndStatement;
    break;

  case Class::Type::Assign:
    ctx.emit_padding(
        type_width, Tetrodotoxin::Format::measure(definition.get_type_ref()));
    ctx << " "_view << Class::Type::Assign << " "_view;
    if (definition.is_alias()) {
      Tetrodotoxin::Format::format(ctx, definition.get_alias_target());
    } else {
      Tetrodotoxin::Format::format(ctx, definition.get_value());
    }
    ctx << Class::Type::EndStatement;
    break;

  default:
    break;
  }
}
