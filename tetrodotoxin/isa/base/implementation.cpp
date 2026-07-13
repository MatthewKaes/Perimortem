// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/base/implementation.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;

auto Isa::Base::Implementation::define(
    const Ttx::Function& function,
    Definition definition) -> Bool {
  return define(function, nullptr, nullptr, definition);
}

auto Isa::Base::Implementation::define(
    const Ttx::Function& function,
    const void* body,
    const void* type,
    Definition definition) -> Bool {
  if (functions.find(&function) != nullptr) {
    return False;
  }

  functions.insert(&function, FunctionBody(body, type, definition));
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

auto Isa::Base::Implementation::define(Compiler::Linkage linkage) -> Bool {
  if (!linkage.is_valid() || find_linkage(linkage.get_function()) != nullptr) {
    return False;
  }

  linkages.insert(linkage);
  return True;
}

auto Isa::Base::Implementation::find_linkage(
    const Ttx::Function& function) const -> const Compiler::Linkage* {
  for (Count i = 0; i < linkages.get_size(); i++) {
    if (&linkages[i].get_function() == &function) {
      return &linkages[i];
    }
  }

  return nullptr;
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

auto Isa::Base::Implementation::has(const Ttx::Function& function) const
    -> Bool {
  return find_body(function) != nullptr;
}

auto Isa::Base::Implementation::has(const Ttx::Member& member) const -> Bool {
  return find(member) != nullptr;
}
