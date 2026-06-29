// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/expression/addressable.hpp"

#include "tetrodotoxin/format/expression.hpp"
#include "tetrodotoxin/format/formatter.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

auto Tetrodotoxin::Format::format(
    Formatter& ctx,
    const Expression::Addressable& expr) -> void {
  switch (expr.get_class().get_type()) {
  case Class::Type::Self:
    ctx << Class::Type::Self;
    break;

  case Class::Type::Package:
    ctx << Class::Type::Package;
    break;

  case Class::Type::Discard:
    ctx << Class::Type::Discard;
    break;

  default:
    ctx << expr.get_text();
    break;
  }
}

auto Tetrodotoxin::Format::measure(const Expression::Addressable& expr)
    -> Count {
  switch (expr.get_class().get_type()) {
  case Class::Type::Self:
    return Class::get_source_text(Class::Type::Self).get_size();

  case Class::Type::Package:
    return Class::get_source_text(Class::Type::Package).get_size();

  case Class::Type::Discard:
    return Class::get_source_text(Class::Type::Discard).get_size();

  default:
    return expr.get_text().get_size();
  }
}
