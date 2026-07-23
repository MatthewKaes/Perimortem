// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/types/members.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

auto Tetrodotoxin::Model::Types::Members::outer_contains(
    const Abstract& outer_context,
    View::Bytes name) const -> Bool {
  return !outer_context.resolve_context(name).is<Invalid>();
}

auto Tetrodotoxin::Model::Types::Members::add_root(
    const Abstract& member,
    const Abstract& outer_context) -> Bool {
  View::Bytes name = member.get_name();
  if (sealed || name.is_empty() || member.is<Ttx::Model::Callables::Static>() ||
      member.is<Ttx::Model::Callables::Self>() ||
      roots_by_name.find(name) != nullptr ||
      outer_contains(outer_context, name)) {
    return False;
  }

  roots_by_name.insert(name, Reference<Abstract>(member));
  roots.insert(Reference<Abstract>(member));
  return True;
}

auto Tetrodotoxin::Model::Types::Members::add_export(
    const Abstract& member,
    const Abstract& outer_context) -> Bool {
  View::Bytes name = member.get_name();
  if (sealed || name.is_empty() || member.is<Ttx::Model::Callables::Static>() ||
      member.is<Ttx::Model::Callables::Self>() ||
      exports_by_name.find(name) != nullptr ||
      outer_contains(outer_context, name)) {
    return False;
  }

  const Index::Entry* rooted = roots_by_name.find(name);
  if (rooted != nullptr && &rooted->value.get() != &member) {
    return False;
  }

  if (rooted == nullptr) {
    roots_by_name.insert(name, Reference<Abstract>(member));
    roots.insert(Reference<Abstract>(member));
  }

  exports_by_name.insert(name, Reference<Abstract>(member));
  exports.insert(Reference<Abstract>(member));
  return True;
}

auto Tetrodotoxin::Model::Types::Members::add_exposed(
    const Ttx::Model::Addressables::Writable& member,
    const Abstract& outer_context) -> Bool {
  const Ttx::Model::Addressable& read_only = member.get_read_only();
  const Abstract& member_type = member.get_type().resolve();
  const Abstract& read_only_type = read_only.get_type().resolve();
  View::Bytes name = member.get_name();
  if (sealed || name.is_empty() || read_only.get_name() != name ||
      &read_only == static_cast<const Ttx::Model::Addressable*>(&member) ||
      read_only.is<Ttx::Model::Addressables::Writable>() ||
      member_type.is<Invalid>() || &read_only_type != &member_type ||
      exports_by_name.find(name) != nullptr ||
      outer_contains(outer_context, name)) {
    return False;
  }

  const Index::Entry* rooted = roots_by_name.find(name);
  if (rooted != nullptr && &rooted->value.get() != &member) {
    return False;
  }

  if (rooted == nullptr) {
    roots_by_name.insert(name, Reference<Abstract>(member));
    roots.insert(Reference<Abstract>(member));
  }

  exports_by_name.insert(name, Reference<Abstract>(read_only));
  exports.insert(Reference<Abstract>(read_only));
  return True;
}

auto Tetrodotoxin::Model::Types::Members::add_static(
    const Ttx::Model::Callables::Static& callable,
    const Abstract& outer_context) -> Bool {
  View::Bytes name = callable.get_name();
  if (sealed || name.is_empty() || statics_by_name.find(name) != nullptr ||
      outer_contains(outer_context, name)) {
    return False;
  }

  statics_by_name.insert(name, Reference<Abstract>(callable));
  roots.insert(Reference<Abstract>(callable));
  return True;
}

auto Tetrodotoxin::Model::Types::Members::add_exported_static(
    const Ttx::Model::Callables::Static& callable,
    const Abstract& outer_context) -> Bool {
  View::Bytes name = callable.get_name();
  if (sealed || name.is_empty() ||
      exported_statics_by_name.find(name) != nullptr ||
      outer_contains(outer_context, name)) {
    return False;
  }

  const Index::Entry* rooted = statics_by_name.find(name);
  if (rooted != nullptr && &rooted->value.get() != &callable) {
    return False;
  }

  if (rooted == nullptr) {
    statics_by_name.insert(name, Reference<Abstract>(callable));
    roots.insert(Reference<Abstract>(callable));
  }

  exported_statics_by_name.insert(name, Reference<Abstract>(callable));
  exports.insert(Reference<Abstract>(callable));
  return True;
}

auto Tetrodotoxin::Model::Types::Members::add_self(
    const Ttx::Model::Callables::Self& callable,
    const Abstract& outer_context) -> Bool {
  View::Bytes name = callable.get_name();
  if (sealed || name.is_empty() || selves_by_name.find(name) != nullptr ||
      outer_contains(outer_context, name)) {
    return False;
  }

  selves_by_name.insert(name, Reference<Abstract>(callable));
  roots.insert(Reference<Abstract>(callable));
  return True;
}

auto Tetrodotoxin::Model::Types::Members::add_exported_self(
    const Ttx::Model::Callables::Self& callable,
    const Abstract& outer_context) -> Bool {
  View::Bytes name = callable.get_name();
  if (sealed || name.is_empty() ||
      exported_selves_by_name.find(name) != nullptr ||
      outer_contains(outer_context, name)) {
    return False;
  }

  const Index::Entry* rooted = selves_by_name.find(name);
  if (rooted != nullptr && &rooted->value.get() != &callable) {
    return False;
  }

  if (rooted == nullptr) {
    selves_by_name.insert(name, Reference<Abstract>(callable));
    roots.insert(Reference<Abstract>(callable));
  }

  exported_selves_by_name.insert(name, Reference<Abstract>(callable));
  exports.insert(Reference<Abstract>(callable));
  return True;
}

auto Tetrodotoxin::Model::Types::Members::get_root(Count index) const
    -> const Abstract& {
  return index < roots.get_size() ? roots.get_view()[index].get()
                                  : Invalid::get_invalid();
}

auto Tetrodotoxin::Model::Types::Members::get_export(Count index) const
    -> const Abstract& {
  return index < exports.get_size() ? exports.get_view()[index].get()
                                    : Invalid::get_invalid();
}

auto Tetrodotoxin::Model::Types::Members::resolve_root(View::Bytes route) const
    -> const Abstract& {
  const Index::Entry* selected = roots_by_name.find(route);
  return selected == nullptr ? Invalid::get_invalid() : selected->value.get();
}

auto Tetrodotoxin::Model::Types::Members::resolve_context(
    View::Bytes route) const -> const Abstract& {
  const Index::Entry* selected = exports_by_name.find(route);
  return selected == nullptr ? Invalid::get_invalid() : selected->value.get();
}

auto Tetrodotoxin::Model::Types::Members::resolve_static(
    View::Bytes route) const -> const Abstract& {
  const Index::Entry* selected = statics_by_name.find(route);
  return selected == nullptr ? Invalid::get_invalid() : selected->value.get();
}

auto Tetrodotoxin::Model::Types::Members::resolve_exported_static(
    View::Bytes route) const -> const Abstract& {
  const Index::Entry* selected = exported_statics_by_name.find(route);
  return selected == nullptr ? Invalid::get_invalid() : selected->value.get();
}

auto Tetrodotoxin::Model::Types::Members::resolve_self(View::Bytes route) const
    -> const Abstract& {
  const Index::Entry* selected = selves_by_name.find(route);
  return selected == nullptr ? Invalid::get_invalid() : selected->value.get();
}

auto Tetrodotoxin::Model::Types::Members::resolve_exported_self(
    View::Bytes route) const -> const Abstract& {
  const Index::Entry* selected = exported_selves_by_name.find(route);
  return selected == nullptr ? Invalid::get_invalid() : selected->value.get();
}

auto Tetrodotoxin::Model::Types::Members::seal() -> Bool {
  if (sealed) {
    return False;
  }

  sealed = True;
  return True;
}
