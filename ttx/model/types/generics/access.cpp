// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/types/generics/access.hpp"

#include "perimortem/memory/managed/bytes.hpp"

namespace Ttx::Model::Types::Generics {

auto Access::find(Perimortem::Core::View::Vector<Argument> arguments) const
    -> Perimortem::Utility::Option<Ttx::Model::Type&> {
  if (arguments.get_size() != 1) {
    return Perimortem::Utility::none;
  }

  const Ttx::Model::Type* element =
      arguments[0].find<const Ttx::Model::Type&>();
  if (element == nullptr) {
    return Perimortem::Utility::none;
  }

  // Access has one formula contract, so its environment-owned cache is keyed
  // by the real element identity. Cache entries are never null.
  for (Count i = 0; i < cache.get_size(); i++) {
    Type& cached = *cache[i];
    if (&cached.get_element_type() == element) {
      return cached;
    }
  }

  Perimortem::Memory::Allocator::Arena& arena = cache.get_arena();
  Perimortem::Memory::Managed::Bytes name(arena, get_name());
  name.concat("["_view);
  name.concat(element->get_name());
  name.concat("]"_view);
  Type& materialized = arena.construct<Type>(name.get_view(), *element);
  cache.insert(&materialized);
  return materialized;
}

}  // namespace Ttx::Model::Types::Generics
