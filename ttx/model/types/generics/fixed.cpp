// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/types/generics/fixed.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/stream/textual.hpp"

namespace Ttx::Model::Types::Generics {

auto Fixed::find(Perimortem::Core::View::Vector<Argument> arguments) const
    -> Perimortem::Utility::Option<Ttx::Model::Type&> {
  if (arguments.get_size() != 2) {
    return Perimortem::Utility::none;
  }

  const Ttx::Model::Type* element =
      arguments[0].find<const Ttx::Model::Type&>();
  const ::Signed_64* extent = arguments[1].find<::Signed_64>();
  if (element == nullptr || extent == nullptr || *extent < 0) {
    return Perimortem::Utility::none;
  }

  // Fixed has one formula contract, so resolved Type identity and scalar value
  // form its complete cache key. Cache entries are never null.
  for (Count i = 0; i < cache.get_size(); i++) {
    Type& cached = *cache[i];
    if (&cached.get_element_type() == element &&
        cached.get_extent() == *extent) {
      return cached;
    }
  }

  Perimortem::Memory::Allocator::Arena& arena = cache.get_arena();
  Perimortem::Memory::Managed::Bytes name(arena, get_name());
  Perimortem::Serialization::Stream::Textual<Perimortem::Memory::Managed::Bytes>
      output(name);
  output << "["_view << element->get_name() << ","_view << *extent << "]"_view;
  Type& materialized =
      arena.construct<Type>(name.get_view(), *element, *extent);
  cache.insert(&materialized);
  return materialized;
}

}  // namespace Ttx::Model::Types::Generics
