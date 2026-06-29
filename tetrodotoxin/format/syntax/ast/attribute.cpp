// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/format/formatter.hpp"
#include "tetrodotoxin/format/syntax.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

auto Tetrodotoxin::Format::format(Formatter& ctx, const Ast::Attribute& attr)
    -> void {
  ctx << Class::Type::Attribute;
  ctx << attr.get_name();

  // Syntax sugar: for attributes with empty packs we can drop the empty `()`.
  if (attr.get_fields().is_empty()) {
    return;
  }

  ctx << Class::Type::PackingStart;
  Tetrodotoxin::Format::format_fields_after_open(
      ctx, attr.get_pack()->get_fields(),
      Class::get_source_text(Class::Type::PackingEnd), False, False, False);
}
