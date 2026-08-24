// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/language/structure.hpp"

#include "tetrodotoxin/render/language/alias.hpp"
#include "tetrodotoxin/render/language/binding.hpp"
#include "tetrodotoxin/render/language/stage.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Render;

auto Language::Structure::create(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition) -> Structure& {
  return domain.construct_from<Structure>(
      [&]() { return Structure(domain, definition); });
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

auto Language::Structure::retain_addressable(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  BAIL_IF(linked);
  return retain_declaration(
      addressables, published_addressables, declaration, visibility);
}

auto Language::Structure::retain_callable(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  BAIL_IF(linked);
  return retain_declaration(
      callables, published_callables, declaration, visibility);
}

auto Language::Structure::retain_type(
    Abstract& declaration,
    Tetrodotoxin::Language::Visibility visibility) -> Bool {
  BAIL_IF(linked);
  return retain_declaration(types, published_types, declaration, visibility);
}

auto Language::Structure::retain_instance(Ttx::Model::Addressable& value)
    -> void {
  instances.insert(value);
}

auto Language::Structure::link(Cursor& cursor) -> Bool {
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

auto Language::Structure::link_restored() -> Bool {
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

auto Language::Structure::resolve() const -> const Abstract& {
  return linked ? static_cast<const Abstract&>(*this)
                : static_cast<const Abstract&>(Invalid::get_invalid());
}

auto Language::Structure::resolve_context(View::Bytes name) const
    -> const Abstract& {
  const Abstract& local = resolve_named(published_types, name);
  return local.is<Invalid>() ? definition.get_host().resolve_context(name)
                             : local;
}

auto Language::Structure::resolve_local_context(View::Bytes name) const
    -> const Abstract& {
  return resolve_named(types, name);
}

auto Language::Structure::resolve_access(const Abstract&, View::Bytes name)
    const -> const Abstract& {
  return resolve_named(published_addressables, name);
}

auto Language::Structure::resolve_call(const Abstract&, View::Bytes name) const
    -> const Abstract& {
  return resolve_named(published_callables, name);
}

auto Language::Structure::get_layout() const -> const Ttx::Concept::Layout& {
  return layout;
}

auto Language::Structure::InstanceLayout::get_size() const -> Count {
  return owner.instances.get_size();
}

auto Language::Structure::InstanceLayout::get_abstract(Count index) const
    -> Option<const Abstract&> {
  BAIL_IF(index >= owner.instances.get_size());
  return owner.instances.at(index).get();
}

auto Language::Structure::InstanceLayout::get_name(Count index) const
    -> Option<View::Bytes> {
  BAIL_IF(index >= owner.instances.get_size());
  return owner.instances.at(index).get().get_name();
}

auto Language::Structure::InstanceLayout::fits_entry(
    const Ttx::Concept::Layout& target,
    Count source_index,
    Count target_index) const -> Bool {
  auto source = get_abstract(source_index);
  auto destination = target.get_abstract(target_index);
  BAIL_IF(!source || !destination);
  auto source_value = source->select<Ttx::Model::Addressable>();
  auto target_value = destination->select<Ttx::Model::Addressable>();
  const Abstract& source_type =
      source_value ? static_cast<const Abstract&>(source_value->get_type())
                   : source->resolve();
  const Abstract& target_type =
      target_value ? static_cast<const Abstract&>(target_value->get_type())
                   : destination->resolve();
  return &source_type == &target_type;
}

auto Language::Structure::InstanceLayout::fits_at(
    const Ttx::Concept::Layout& target,
    Count target_offset) const -> Bool {
  BAIL_IF(!has_target_segment(target, target_offset));
  for (Count index = 0; index < get_size(); index++) {
    BAIL_IF(!fits_entry(target, index, target_offset + index));
  }
  return True;
}

auto Language::Structure::InstanceLayout::get_fitted_at(
    const Ttx::Concept::Layout& target,
    Count target_offset,
    Count target_index) const -> Result<const Abstract&, Errors> {
  if (target_index >= get_size()) {
    return Errors::IndexOutOfBounds;
  }
  if (!has_target_segment(target, target_offset)) {
    return Errors::SizeMismatch;
  }
  if (!fits_entry(target, target_index, target_offset + target_index)) {
    return Errors::IncompatibleFit;
  }
  return *get_abstract(target_index);
}
