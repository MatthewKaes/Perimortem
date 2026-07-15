// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/base/implementation.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;

auto Isa::Base::Implementation::define(
    const Ttx::Function& function,
    Definition definition) -> Bool {
  return define(function, FunctionBody(definition));
}

auto Isa::Base::Implementation::define(
    const Ttx::Function& function,
    FunctionBody body) -> Bool {
  if (functions.find(&function) != nullptr) {
    return False;
  }

  functions.insert(&function, body);
  return True;
}

auto Isa::Base::Implementation::define(
    const Ttx::Member& member,
    Definition definition) -> Bool {
  if (members.find(&member) != nullptr) {
    return False;
  }

  members.insert(&member, definition);
  return True;
}

auto Isa::Base::Implementation::define(
    const Ttx::Type& type,
    Definition definition) -> Bool {
  if (types.find(&type) != nullptr) {
    return False;
  }

  types.insert(&type, definition);
  return True;
}

auto Isa::Base::Implementation::define(Abi::Linkage linkage) -> Bool {
  const Ttx::Function* function = &linkage.get_function();
  if (linkage_indices.find(function) != nullptr) {
    return False;
  }

  linkage_indices.insert(function, linkages.get_size());
  linkages.insert(linkage);
  return True;
}

auto Isa::Base::Implementation::find_linkage(
    const Ttx::Function& function) const -> const Abi::Linkage* {
  const auto* entry = linkage_indices.find(&function);
  return entry == nullptr ? nullptr : &linkages[entry->value];
}

auto Isa::Base::Implementation::find_body(const Ttx::Function& function) const
    -> const FunctionBody* {
  const auto* entry = functions.find(&function);
  return entry == nullptr ? nullptr : &entry->value;
}

auto Isa::Base::Implementation::find_definition(
    const Ttx::Function& function) const -> const Definition* {
  const FunctionBody* body = find_body(function);
  return body == nullptr ? nullptr : &body->get_definition();
}

auto Isa::Base::Implementation::find(const Ttx::Member& member) const
    -> const Definition* {
  const auto* entry = members.find(&member);
  return entry == nullptr ? nullptr : &entry->value;
}

auto Isa::Base::Implementation::find(const Ttx::Type& type) const
    -> const Definition* {
  const auto* entry = types.find(&type);
  return entry == nullptr ? nullptr : &entry->value;
}

auto Isa::Base::Implementation::has(const Ttx::Function& function) const
    -> Bool {
  return find_body(function) != nullptr;
}

auto Isa::Base::Implementation::has(const Ttx::Member& member) const -> Bool {
  return find(member) != nullptr;
}

auto Isa::Base::Implementation::has(const Ttx::Type& type) const -> Bool {
  return find(type) != nullptr;
}
