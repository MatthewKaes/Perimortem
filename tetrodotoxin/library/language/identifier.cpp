// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/identifier.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static const Layouts::Fluid identifier_inputs;

auto Language::Identifier::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    Materializations&,
    Core::Option<const Type&>) -> Bool {
  const Abstract& selected = lexical_context.resolve_context(route).resolve();
  auto source_anchor = get_anchor();

  // A later context may fill an unresolved route, but a successful edge
  // is permanent. Repeating the same exact link remains harmless.
  return selected.visit<Addressable>(
      [&](const Addressable& linked) {
        if (addressable) {
          if (&addressable->get() == &linked) {
            return True;
          }

          source.report(
              source_anchor,
              "Expression Identifier cannot change its linked Addressable."_view,
              "Keep one exact address bound to this authored route."_view);
          return False;
        }

        addressable = Reference<const Addressable>(linked);
        return True;
      },
      [&](const Abstract&) {
        source.report(
            source_anchor,
            "Expression Identifier route did not resolve to an "
            "Addressable."_view,
            "Publish the named address in this logical context before "
            "linking."_view);
        return False;
      });
}

auto Language::Identifier::get_documentation() const -> const Documentation& {
  return addressable.visit(
      []() -> const Documentation& { return Documentation::get_empty(); },
      [](const Reference<const Addressable>& selected) -> const Documentation& {
        return selected.get().get_documentation();
      });
}

auto Language::Identifier::get_type() const -> const Abstract& {
  return addressable.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Addressable>& selected) -> const Abstract& {
        return selected.get().get_type();
      });
}

auto Language::Identifier::get_inputs() const -> const Layout& {
  return identifier_inputs;
}

auto Language::Identifier::get_addressable() const
    -> Core::Option<const Addressable&> {
  return addressable.visit(
      []() -> Core::Option<const Addressable&> { return {}; },
      [](const Reference<const Addressable>& selected)
          -> Core::Option<const Addressable&> { return selected.get(); });
}
