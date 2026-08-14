// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/language/diagnostics.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

Language::Diagnostics::Diagnostics(Allocator::Arena& domain)
    : domain(domain), values(domain) {}

auto Language::Diagnostics::report(
    Option<Anchor> anchor,
    View::Bytes message,
    View::Bytes hint) -> void {
  Diagnostic diagnostic(anchor, domain.proxy(message), domain.proxy(hint));
  values.insert(diagnostic);
}

auto Language::Diagnostics::get_values() const -> View::Vector<Diagnostic> {
  return values;
}
