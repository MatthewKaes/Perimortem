// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/format/syntax.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

auto Tetrodotoxin::Format::format(Formatter& ctx, const Ast::Layout& layout)
    -> void {
  if (layout.is_value()) {
    Tetrodotoxin::Format::format(ctx, layout.get_value_type());
    return;
  }

  if (layout.is_named()) {
    View::Vector<Ast::Param> params = layout.get_params();
    ctx << Class::Type::IndexStart;

    if (params.get_size() <= 1) {
      for (Count i = 0; i < params.get_size(); i++) {
        if (i > 0) {
          ctx << Class::Type::PackingOp << " "_view;
        }

        Tetrodotoxin::Format::format(ctx, params[i]);
      }

      ctx << Class::Type::IndexEnd;
      return;
    }

    ctx.emit_newline();
    ctx.indent++;

    for (Count i = 0; i < params.get_size(); i++) {
      ctx.do_indent();
      Tetrodotoxin::Format::format(ctx, params[i]);
      ctx << Class::Type::PackingOp;
      ctx.emit_newline();
    }

    ctx.indent--;
    ctx.do_indent();
    ctx << Class::Type::IndexEnd;
    return;
  }

  if (layout.is_unnamed()) {
    View::Vector<Type::Ref> types = layout.get_types();
    ctx << Class::Type::IndexStart;

    for (Count i = 0; i < types.get_size(); i++) {
      if (i > 0) {
        ctx << Class::Type::PackingOp << " "_view;
      }

      Tetrodotoxin::Format::format(ctx, types[i]);
    }

    ctx << Class::Type::IndexEnd;
  }
}
