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
    : domain(domain), documentation(documentation), diagnostics(domain) {}

auto Language::Monograph::link() -> Bool {
  return True;
}

auto Language::Monograph::finalize() -> Bool {
  return True;
}

auto Language::Monograph::report(
    Perimortem::Utility::Option<Anchor> anchor,
    View::Bytes message,
    View::Bytes hint) -> void {
  // A diagnostic can outlive the transaction that built either input view.
  // Copying both values once here keeps Diagnostic passive and lets every
  // later consumer borrow the same ordered facts.
  Diagnostic diagnostic(anchor, domain.proxy(message), domain.proxy(hint));
  diagnostics.insert(diagnostic);
}

auto Language::Monograph::get_diagnostics() const -> View::Vector<Diagnostic> {
  return diagnostics;
}
