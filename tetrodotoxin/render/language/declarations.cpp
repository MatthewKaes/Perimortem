// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/language/declarations.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/render/language/alias.hpp"
#include "tetrodotoxin/render/language/binding.hpp"
#include "tetrodotoxin/render/language/monograph.hpp"
#include "tetrodotoxin/render/language/stage.hpp"
#include "tetrodotoxin/render/language/structure.hpp"
#include "ttx/bootstrap/concept/none.hpp"
#include "ttx/bootstrap/concept/unknown.hpp"
#include "ttx/bootstrap/model/layouts/fluid.hpp"
#include "ttx/bootstrap/model/layouts/named.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Render;

auto Language::Declarations::retain(
    Managed::Vector<Abstract*>& declarations,
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  BAIL_IF(linked);
  auto occupied = [&](View::Vector<Abstract*> owned) -> Bool {
    for (const Abstract* retained : owned) {
      if (retained->get_name() == declaration.get_name()) {
        return True;
      }
    }
    return False;
  };
  BAIL_IF(
      occupied(addressables.get_view()) || occupied(callables.get_view()) ||
      occupied(types.get_view()));

  declarations.insert(&declaration);
  published.insert(
      &declaration, visibility != Tetrodotoxin::Language::Visibility::Private);
  return True;
}

auto Language::Declarations::Authority::resolve_concept(View::Bytes name) const
    -> const Abstract& {
  const Abstract& type =
      owner.resolve_type(name, Tetrodotoxin::Language::Visibility::Public);
  if (!type.is<Unknown>() && !type.is<None>()) {
    return type;
  }
  const Abstract& addressable = owner.resolve_addressable(
      name, Tetrodotoxin::Language::Visibility::Public);
  if (!addressable.is<Unknown>() && !addressable.is<None>()) {
    return addressable;
  }
  return owner.resolve_callable(
      name, Tetrodotoxin::Language::Visibility::Public);
}

auto Language::Declarations::Authority::visit_concepts(
    ttx_named_abstract_callable* visitor) const -> void {
  auto retain = [&](View::Vector<Abstract*> declarations) {
    for (const Abstract* declaration : declarations) {
      auto publication = owner.published.find(declaration);
      if (publication && publication->value) {
        visit_concept(visitor, declaration->get_name(), *declaration);
      }
    }
  };
  retain(owner.types.get_view());
  retain(owner.addressables.get_view());
  retain(owner.callables.get_view());
}

auto Language::Declarations::visit_concepts(
    ttx_named_abstract_callable* visitor) const -> void {
  Ttx::Concept::Abstract::visit_concept(visitor, "static"_view, authority);
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
  for (Abstract* entry : types.get_view()) {
    Abstract& declaration = *entry;
    auto alias = declaration.select<Alias>();
    auto structure = declaration.select<Structure>();
    if (alias) {
      valid &= alias->link(cursor, context);
    } else if (structure) {
      valid &= structure->link(cursor);
    }
  }
  for (Abstract* entry : addressables.get_view()) {
    auto binding = entry->select<Binding>();
    valid &= binding && binding->link(cursor, context);
  }
  for (Abstract* entry : callables.get_view()) {
    auto stage = entry->select<Stage>();
    valid &= stage && stage->link(cursor);
  }

  linked = valid;
  return valid;
}

auto Language::Declarations::link_restored(Abstract& context) -> Bool {
  if (linked) {
    return True;
  }

  for (Abstract* entry : types.get_view()) {
    Abstract& declaration = *entry;
    auto alias = declaration.select<Alias>();
    auto structure = declaration.select<Structure>();
    BAIL_IF(
        (!alias && !structure) || (alias && !alias->link_restored(context)) ||
        (structure && !structure->link_restored()));
  }
  for (Abstract* entry : addressables.get_view()) {
    auto binding = entry->select<Binding>();
    BAIL_IF(!binding || !binding->link_restored(context));
  }
  for (Abstract* entry : callables.get_view()) {
    auto stage = entry->select<Stage>();
    BAIL_IF(!stage || !stage->link_restored());
  }

  linked = True;
  return True;
}

auto Language::Declarations::resolve_lexical_context(
    const Abstract& context,
    View::Bytes name) -> const Abstract& {
  auto monograph = context.select<Language::Monograph>();
  if (monograph) {
    return monograph->resolve_lexical_context(name);
  }

  auto structure = context.select<Language::Structure>();
  if (structure) {
    const Abstract& local = structure->resolve_local_context(name);
    return local.is<Unknown>() || local.is<None>()
               ? resolve_lexical_context(
                     structure->get_definition().get_host(), name)
               : local;
  }

  return context.resolve_concept(name);
}

auto Language::Declarations::resolve(
    View::Vector<Abstract*> declarations,
    View::Bytes name,
    Tetrodotoxin::Language::Visibility visibility) const -> const Abstract& {
  for (const Abstract* declaration : declarations) {
    if (declaration->get_name() != name) {
      continue;
    }
    auto publication = published.find(declaration);
    if (visibility == Tetrodotoxin::Language::Visibility::Private ||
        (publication && publication->value)) {
      return *declaration;
    }
  }
  return linked ? static_cast<const Abstract&>(None::get_none())
                : static_cast<const Abstract&>(Unknown::get_unknown());
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
