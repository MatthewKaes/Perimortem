// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/monograph.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

Language::Monograph::~Monograph() {}

Language::Monograph::Monograph(
    Allocator::Arena& domain,
    const Abstract& language,
    const Documentation& documentation,
    Abstract& context)
    : domain(domain),
      documentation(documentation),
      context(context),
      language(language) {}

auto Language::Monograph::get_layer(const Abstract& requested) const
    -> Option<const Monograph&> {
  if (&requested == &language) {
    return *this;
  }

  return {};
}

auto Language::Monograph::compose(Cursor&) -> Bool {
  return True;
}

auto Language::Monograph::link(Cursor&) -> Bool {
  return True;
}

auto Language::Monograph::finalize(Cursor&) -> Bool {
  return True;
}

auto Language::Monograph::compose_restored() -> Bool {
  return True;
}

auto Language::Monograph::link_restored() -> Bool {
  return True;
}

auto Language::Monograph::finalize_restored() -> Bool {
  return True;
}

auto Language::Monograph::resolve_context(View::Bytes route) const
    -> const Abstract& {
  // A base Monograph contributes no synthetic lookup surface. Concrete roots
  // answer their own names first and use this boundary only for the borrowed
  // outer context supplied by the source transaction.
  return context.resolve_context(route);
}
