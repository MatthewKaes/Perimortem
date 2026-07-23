// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/types/generics/view.hpp"

#include "perimortem/memory/managed/bytes.hpp"

namespace Ttx::Model::Types::Generics {

auto View::create(
    Perimortem::Core::View::Vector<Argument> arguments,
    Perimortem::Memory::Allocator::Arena& arena) const
    -> Perimortem::Utility::Option<const Ttx::Model::Type&> {
  if (arguments.get_size() != 1) {
    return Perimortem::Utility::none;
  }

  const Ttx::Model::Type* element =
      arguments[0].find<const Ttx::Model::Type&>();
  if (element == nullptr) {
    return Perimortem::Utility::none;
  }

  Perimortem::Memory::Managed::Bytes name(arena, get_name());
  name.concat("["_view);
  name.concat(element->get_name());
  name.concat("]"_view);
  return arena.construct<Type>(name.get_view(), *element);
}

}  // namespace Ttx::Model::Types::Generics
