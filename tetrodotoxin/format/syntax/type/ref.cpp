// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/format/syntax.hpp"
#include "tetrodotoxin/syntax/context.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

auto Tetrodotoxin::Format::measure(const Type::Ref& ref) -> Count {
  if (ref.is_empty()) {
    return 0;
  }

  Count width = 0;
  View::Vector<View::Bytes> path = ref.get_name_path();

  for (Count i = 0; i < path.get_size(); i++) {
    if (i > 0) {
      width++;
    }

    width += path[i].get_size();
  }

  if (ref.is_generic()) {
    width += 2;
    View::Vector<Type::Ref> params = ref.get_params();

    for (Count i = 0; i < params.get_size(); i++) {
      if (i > 0) {
        width += 2;
      }

      width += Tetrodotoxin::Format::measure(params[i]);
    }
  }

  return width;
}

auto Tetrodotoxin::Format::format(Formatter& ctx, const Type::Ref& ref)
    -> void {
  if (ref.is_empty()) {
    return;
  }

  View::Vector<Type::Ref::Segment> path = ref.get_segments();

  for (Count i = 0; i < path.get_size(); i++) {
    if (i > 0) {
      ctx << Class::Type::AddressOp;
    }

    View::Bytes source = path[i].klass.get_source_text();
    ctx << (!source.is_empty() ? source : path[i].text);
  }

  if (ref.is_generic()) {
    ctx << Class::Type::IndexStart;

    View::Vector<Type::Ref> params = ref.get_params();

    for (Count i = 0; i < params.get_size(); i++) {
      if (i > 0) {
        ctx << Class::Type::PackingOp << " "_view;
      }

      Tetrodotoxin::Format::format(ctx, params[i]);
    }

    ctx << Class::Type::IndexEnd;
  }
}
