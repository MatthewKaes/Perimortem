// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/allocation/system_v.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "tetrodotoxin/abi/type.hpp"
#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Compiler;

Allocation::SystemV::SystemV(View::Vector<Ttx::Member> parameters) {
  constexpr Count integer_capacity = 6;
  constexpr Count real_capacity = 8;
  Count integer_index = 0;
  Count real_index = 0;
  for (Count i = 0; i < parameters.get_size(); i++) {
    const Ttx::Type& type = parameters[i].get_type();
    Count width = get_width(type);
    Abi::Lowering lowering =
        Abi::Lowering(type.resolve_attribute("abi"_view).get_unsigned());
    Bank bank = lowering == Abi::Lowering::Real ? Bank::Real : Bank::Integer;
    Count* index = bank == Bank::Real ? &real_index : &integer_index;
    Count capacity = bank == Bank::Real ? real_capacity : integer_capacity;
    if (*index + width > capacity) {
      bank = Bank::Stack;
      index = &stack_count;
    }

    starts.insert(locations.get_size());
    widths.insert(width);
    for (Count component = 0; component < width; component++) {
      locations.insert({bank, (*index)++});
    }
  }
}

auto Allocation::SystemV::get(Count parameter, Count component) const
    -> Location {
  if (parameter >= starts.get_size() || component >= widths[parameter]) {
    Diagnostics::Log::fatal(
        "System V allocation location is outside the planned layout."_view);
  }

  return locations[starts[parameter] + component];
}

auto Allocation::SystemV::get_width(const Ttx::Type& type) -> Count {
  switch (Abi::Lowering(type.resolve_attribute("abi"_view).get_unsigned())) {
  case Abi::Lowering::ViewBytes:
    return 2;

  case Abi::Lowering::Bool:
  case Abi::Lowering::Integer:
  case Abi::Lowering::Signed:
  case Abi::Lowering::Real:
    return 1;

  case Abi::Lowering::Invalid:
  case Abi::Lowering::Void:
    return 0;
  }

  return 0;
}
