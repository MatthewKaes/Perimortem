// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/types/members.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;

auto Tetrodotoxin::Model::Types::Members::add(
    const Abstract& member,
    Bool publish) -> Bool {
  View::Bytes name = member.get_name();
  if (name.is_empty() || roots_by_name.find(name) != nullptr) {
    return False;
  }

  roots_by_name.insert(name, Reference<Abstract>(member));
  roots.insert(Reference<Abstract>(member));
  if (publish) {
    exports_by_name.insert(name, Reference<Abstract>(member));
    exports.insert(Reference<Abstract>(member));
  }
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

auto Tetrodotoxin::Model::Types::Members::resolve_context(
    View::Bytes route) const -> const Abstract& {
  const Index::Entry* selected = exports_by_name.find(route);
  return selected == nullptr ? Invalid::get_invalid() : selected->value.get();
}
