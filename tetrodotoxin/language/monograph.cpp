// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/language/monograph.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

Language::Monograph::~Monograph() {}

Language::Monograph::Monograph(
    Allocator::Arena& domain,
    const Documentation& documentation)
    : Monograph(
          domain,
          documentation,
          domain.construct<Language::Diagnostics>(domain)) {}

Language::Monograph::Monograph(
    Allocator::Arena& domain,
    const Documentation& documentation,
    Language::Diagnostics& diagnostics)
    : domain(domain), documentation(documentation), diagnostics(diagnostics) {}

auto Language::Monograph::link() -> Bool {
  return True;
}

auto Language::Monograph::finalize() -> Bool {
  return True;
}

auto Language::Monograph::report(
    Perimortem::Core::Option<Anchor> anchor,
    View::Bytes message,
    View::Bytes hint) -> void {
  diagnostics.report(anchor, message, hint);
}

auto Language::Monograph::get_diagnostics() const -> View::Vector<Diagnostic> {
  return diagnostics.get_values();
}
