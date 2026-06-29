// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/format/syntax.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

auto Tetrodotoxin::Format::format(Formatter& ctx, const Ast::Param& param)
    -> void {
  View::Vector<Ast::Attribute> attrs = param.get_attributes();

  for (Count i = 0; i < attrs.get_size(); i++) {
    Tetrodotoxin::Format::format(ctx, attrs[i]);
    ctx << " "_view;
  }

  ctx << Class::Type::AddressOp;
  ctx << param.get_name();
  ctx << " "_view << Class::Type::Define << " "_view;
  Tetrodotoxin::Format::format(ctx, param.get_type());
}
