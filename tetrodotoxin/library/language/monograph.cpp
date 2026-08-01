// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/monograph.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Tetrodotoxin;

Library::Language::Monograph::Monograph(
    Allocator::Arena& domain,
    const Documentation& documentation,
    Library::Dialect& host)
    : Tetrodotoxin::Language::Dialect::Monograph(domain, documentation, host),
      functions(domain),
      public_functions(domain) {}

auto Library::Language::Monograph::bind_function(Function& function) -> Bool {
  const View::Bytes name = function.get_name();

  // Both mutations happen after the exact duplicate check. A rejected
  // declaration therefore preserves the first edge and its public position
  // while the failed interpretation discards the unreachable transaction.
  if (name.is_empty() || functions.contains(name)) {
    return False;
  }

  functions.launder(name, function);
  if (function.get_visibility() == Visibility::Public) {
    public_functions.insert(function);
  }

  return True;
}

auto Library::Language::Monograph::get_name() const -> View::Bytes {
  return "Library"_view;
}

auto Library::Language::Monograph::resolve_context(View::Bytes route) const
    -> const Abstract& {
  return functions.visit(
      route,
      [](const Function& function) -> const Abstract& { return function; },
      [&]() -> const Abstract& {
        const auto& library = static_cast<const Library::Dialect&>(host);
        return library.resolve_intrinsic(route);
      });
}

auto Library::Language::Monograph::get_public_functions() const
    -> View::Vector<Reference<Function>> {
  return public_functions;
}
