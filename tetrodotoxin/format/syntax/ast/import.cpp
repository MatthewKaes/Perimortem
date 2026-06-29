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
    const Ast::Import& import,
    Count name_width) -> void {
  ctx << Class(Class::Type::Import).get_source_text();
  ctx << " "_view;
  ctx << import.get_local_name();
  ctx.emit_padding(name_width, import.get_local_name().get_size());
  ctx << " "_view << Class::Type::Define << " "_view;
  ctx << import.get_class().get_source_text();
  ctx << " "_view << Class::Type::Assign << " "_view;

  if (import.is_file_source()) {
    ctx << Class::Type::PackingStart << " "_view << Class::Type::AddressOp;
    ctx << "source"_view;
    ctx << " "_view << Class::Type::Assign << " "_view;
    ctx << import.get_file_path();
    ctx << " "_view << Class::Type::PackingEnd;
  } else if (import.is_package_source()) {
    Tetrodotoxin::Format::format(ctx, import.get_package_alias());
  }

  ctx << Class::Type::EndStatement;
  ctx.emit_newline();
}

auto Tetrodotoxin::Format::format(Formatter& ctx, const Ast::Import& import)
    -> void {
  Tetrodotoxin::Format::format(ctx, import, import.get_local_name().get_size());
}
