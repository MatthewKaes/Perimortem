// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/generics/view.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/library/language/types/view.hpp"

using namespace Tetrodotoxin::Library::Language;

auto Generics::View::create(
    Perimortem::Core::View::Vector<Argument> arguments,
    Perimortem::Memory::Allocator::Arena& arena) const
    -> Perimortem::Core::Option<const Ttx::Model::Type&> {
  if (arguments.get_size() != 1) {
    return {};
  }

  const Ttx::Model::Type* element =
      arguments.get_data()[0].find<const Ttx::Model::Type&>();
  if (element == nullptr) {
    return {};
  }

  Perimortem::Memory::Managed::Bytes name(arena, get_name());
  name.concat("["_view);
  name.concat(element->get_name());
  name.concat("]"_view);
  return arena.construct<Types::View>(name.get_view(), *element);
}
