// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/language/declarations.hpp"

#include "tetrodotoxin/render/language/alias.hpp"
#include "tetrodotoxin/render/language/binding.hpp"
#include "tetrodotoxin/render/language/stage.hpp"
#include "tetrodotoxin/render/language/structure.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Render;

auto Language::Declarations::retain(
    Managed::Vector<Reference<Abstract>>& declarations,
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  BAIL_IF(linked);
  for (const Reference<Abstract>& retained : declarations.get_view()) {
    BAIL_IF(retained.get().get_name() == declaration.get_name());
  }

  declarations.insert(declaration);
  published.insert(
      &declaration, visibility != Tetrodotoxin::Language::Visibility::Private);
  return True;
}

auto Language::Declarations::retain_addressable(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  return retain(addressables, declaration, visibility);
}

auto Language::Declarations::retain_callable(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  return retain(callables, declaration, visibility);
}

auto Language::Declarations::retain_type(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  return retain(types, declaration, visibility);
}

auto Language::Declarations::link(Cursor& cursor, Abstract& context) -> Bool {
  if (linked) {
    return True;
  }

  Bool valid = True;
  for (const Reference<Abstract>& entry : types.get_view()) {
    Abstract& declaration = entry.get();
    auto alias = declaration.select<Alias>();
    auto structure = declaration.select<Structure>();
    if (alias) {
      valid &= alias->link(cursor, context);
    } else if (structure) {
      valid &= structure->link(cursor);
    }
  }
  for (const Reference<Abstract>& entry : addressables.get_view()) {
    auto binding = entry.get().select<Binding>();
    valid &= binding && binding->link(cursor, context);
  }
  for (const Reference<Abstract>& entry : callables.get_view()) {
    auto stage = entry.get().select<Stage>();
    valid &= stage && stage->link(cursor);
  }

  linked = valid;
  return valid;
}

auto Language::Declarations::link_restored(Abstract& context) -> Bool {
  if (linked) {
    return True;
  }

  for (const Reference<Abstract>& entry : types.get_view()) {
    Abstract& declaration = entry.get();
    auto alias = declaration.select<Alias>();
    auto structure = declaration.select<Structure>();
    BAIL_IF(
        (!alias && !structure) || (alias && !alias->link_restored(context)) ||
        (structure && !structure->link_restored()));
  }
  for (const Reference<Abstract>& entry : addressables.get_view()) {
    auto binding = entry.get().select<Binding>();
    BAIL_IF(!binding || !binding->link_restored(context));
  }
  for (const Reference<Abstract>& entry : callables.get_view()) {
    auto stage = entry.get().select<Stage>();
    BAIL_IF(!stage || !stage->link_restored());
  }

  linked = True;
  return True;
}

auto Language::Declarations::resolve(
    View::Vector<Reference<Abstract>> declarations,
    View::Bytes name,
    Tetrodotoxin::Language::Visibility visibility) const -> const Abstract& {
  for (const Reference<Abstract>& declaration : declarations) {
    if (declaration.get().get_name() != name) {
      continue;
    }
    auto publication = published.find(&declaration.get());
    if (visibility == Tetrodotoxin::Language::Visibility::Private ||
        (publication && publication->value)) {
      return declaration.get();
    }
  }
  return Invalid::get_invalid();
}

auto Language::Declarations::resolve_addressable(
    View::Bytes name,
    Tetrodotoxin::Language::Visibility visibility) const -> const Abstract& {
  return resolve(addressables, name, visibility);
}

auto Language::Declarations::resolve_callable(
    View::Bytes name,
    Tetrodotoxin::Language::Visibility visibility) const -> const Abstract& {
  return resolve(callables, name, visibility);
}

auto Language::Declarations::resolve_type(
    View::Bytes name,
    Tetrodotoxin::Language::Visibility visibility) const -> const Abstract& {
  return resolve(types, name, visibility);
}
