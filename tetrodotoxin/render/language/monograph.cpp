// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/language/monograph.hpp"

#include "tetrodotoxin/render/language/alias.hpp"
#include "tetrodotoxin/render/language/binding.hpp"
#include "tetrodotoxin/render/language/stage.hpp"
#include "tetrodotoxin/render/language/structure.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Render;

auto Language::Monograph::create(
    Allocator::Arena& arena,
    const Abstract& language,
    const Documentation& documentation,
    Abstract& context) -> Monograph& {
  return arena.construct_from<Monograph>(
      [&]() { return Monograph(arena, language, documentation, context); });
}

static auto retain_declaration(
    Managed::Vector<Reference<Abstract>>& declarations,
    Managed::Vector<Reference<Abstract>>& published,
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  for (const Reference<Abstract>& retained : declarations.get_view()) {
    BAIL_IF(retained.get().get_name() == declaration.get_name());
  }
  declarations.insert(declaration);
  if (visibility != Tetrodotoxin::Language::Visibility::Private) {
    published.insert(declaration);
  }
  return True;
}

static auto resolve_named(
    View::Vector<Reference<Abstract>> declarations,
    View::Bytes name) -> const Abstract& {
  for (const Reference<Abstract>& declaration : declarations) {
    if (declaration.get().get_name() == name) {
      return declaration.get();
    }
  }
  return Invalid::get_invalid();
}

auto Language::Monograph::retain_addressable(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  BAIL_IF(linked);
  return retain_declaration(
      addressables, published_addressables, declaration, visibility);
}

auto Language::Monograph::retain_callable(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  BAIL_IF(linked);
  return retain_declaration(
      callables, published_callables, declaration, visibility);
}

auto Language::Monograph::retain_type(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  BAIL_IF(linked);
  return retain_declaration(types, published_types, declaration, visibility);
}

auto Language::Monograph::link(Cursor& cursor) -> Bool {
  if (linked) {
    return True;
  }

  Bool valid = True;
  for (const Reference<Abstract>& entry : types.get_view()) {
    Abstract& declaration = entry.get();
    auto alias = declaration.select<Alias>();
    auto structure = declaration.select<Structure>();
    if (alias) {
      valid &= alias->link(cursor, *this);
    } else if (structure) {
      valid &= structure->link(cursor);
    }
  }
  for (const Reference<Abstract>& entry : addressables.get_view()) {
    auto binding = entry.get().select<Binding>();
    valid &= binding && binding->link(cursor, *this);
  }
  for (const Reference<Abstract>& entry : callables.get_view()) {
    auto stage = entry.get().select<Stage>();
    valid &= stage && stage->link(cursor);
  }
  linked = valid;
  return valid;
}

auto Language::Monograph::finalize(Cursor&) -> Bool {
  finalized = linked;
  return finalized;
}

auto Language::Monograph::link_restored() -> Bool {
  if (linked) {
    return True;
  }

  for (const Reference<Abstract>& entry : types.get_view()) {
    Abstract& declaration = entry.get();
    auto alias = declaration.select<Alias>();
    auto structure = declaration.select<Structure>();
    BAIL_IF(
        (!alias && !structure) || (alias && !alias->link_restored(*this)) ||
        (structure && !structure->link_restored()));
  }
  for (const Reference<Abstract>& entry : addressables.get_view()) {
    auto binding = entry.get().select<Binding>();
    BAIL_IF(!binding || !binding->link_restored(*this));
  }
  for (const Reference<Abstract>& entry : callables.get_view()) {
    auto stage = entry.get().select<Stage>();
    BAIL_IF(!stage || !stage->link_restored());
  }

  linked = True;
  return True;
}

auto Language::Monograph::finalize_restored() -> Bool {
  finalized = linked;
  return finalized;
}

auto Language::Monograph::resolve_context(View::Bytes name) const
    -> const Abstract& {
  const Abstract& local = resolve_named(published_types, name);
  return local.is<Invalid>()
             ? Tetrodotoxin::Language::Monograph::resolve_context(name)
             : local;
}

auto Language::Monograph::resolve_local_context(View::Bytes name) const
    -> const Abstract& {
  return resolve_named(types, name);
}

auto Language::Monograph::resolve_access(const Abstract&, View::Bytes name)
    const -> const Abstract& {
  return resolve_named(published_addressables, name);
}

auto Language::Monograph::resolve_call(const Abstract&, View::Bytes name) const
    -> const Abstract& {
  return resolve_named(published_callables, name);
}
