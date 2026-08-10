// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/generics/range.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/library/language/types/range.hpp"
#include "ttx/model/types/signed.hpp"
#include "ttx/model/types/unsigned.hpp"

using namespace Tetrodotoxin::Library::Language;

auto Generics::Range::create(
    Perimortem::Core::View::Vector<Argument> arguments,
    Perimortem::Memory::Allocator::Arena& arena) const
    -> Perimortem::Core::Option<const Ttx::Model::Type&> {
  if (arguments.get_size() != 1) {
    return {};
  }

  const Ttx::Model::Type* element =
      arguments.get_data()[0].find<const Ttx::Model::Type&>();
  if (element == nullptr || (!element->is<Ttx::Model::Types::Signed>() &&
                             !element->is<Ttx::Model::Types::Unsigned>())) {
    return {};
  }

  Perimortem::Memory::Managed::Bytes name(arena, get_name());
  name.concat("["_view);
  name.concat(element->get_name());
  name.concat("]"_view);
  return arena.construct<Types::Range>(name.get_view(), *element);
}
