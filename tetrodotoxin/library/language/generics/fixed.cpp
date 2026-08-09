// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/generics/fixed.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/library/language/types/fixed.hpp"

using namespace Tetrodotoxin::Library::Language;

auto Generics::Fixed::create(
    Perimortem::Core::View::Vector<Argument> arguments,
    Perimortem::Memory::Allocator::Arena& arena) const
    -> Perimortem::Core::Option<const Ttx::Model::Type&> {
  if (arguments.get_size() != 2) {
    return {};
  }

  const auto* argument_data = arguments.get_data();
  const Ttx::Model::Type* element =
      argument_data[0].find<const Ttx::Model::Type&>();
  const ::Signed_64* extent = argument_data[1].find<::Signed_64>();
  if (element == nullptr || extent == nullptr || *extent < 0) {
    return {};
  }

  Perimortem::Memory::Managed::Bytes name(arena, get_name());
  Perimortem::Serialization::Stream::Textual<Perimortem::Memory::Managed::Bytes>
      output(name);
  output << "["_view << element->get_name() << ","_view << *extent << "]"_view;
  return arena.construct<Types::Fixed>(name.get_view(), *element, *extent);
}
