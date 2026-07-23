// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/types/generics.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/hash.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;

auto Ttx::Model::Types::Generic::Materializations::Key::hash() const
    -> Unsigned_64 {
  Unsigned_64 value = Hash(&formula.get()).get_value();
  for (Count i = 0; i < arguments.get_size(); i++) {
    Unsigned_64 argument = arguments[i].visit(
        []() -> Unsigned_64 { return 0; },
        [](const Ttx::Model::Type& type) { return Hash(&type).Rehash(1); },
        [](::Unsigned_64 scalar) { return Hash(scalar).Rehash(2); },
        [](::Signed_64 scalar) { return Hash(scalar).Rehash(3); },
        [](::Bool scalar) {
          Unsigned_64 value = scalar == True ? 1 : 0;
          return Hash(value).Rehash(4);
        });
    value = Hash(value).Rehash(argument);
  }

  return value;
}

auto Ttx::Model::Types::Generic::Materializations::materialize(
    const Generic& generic,
    View::Vector<Argument> arguments) -> Option<const Ttx::Model::Type&> {
  Key requested(generic, arguments);
  const auto* existing = entries.find(requested);
  if (existing != nullptr) {
    return existing->value.get();
  }

  Option<const Ttx::Model::Type&> created = generic.create(arguments, arena);
  return created.visit(
      [](const None&) -> Option<const Ttx::Model::Type&> { return none; },
      [&](const Ttx::Model::Type& type) -> Option<const Ttx::Model::Type&> {
        Argument* retained = Data::cast<Argument>(
            arena.allocate(sizeof(Argument) * arguments.get_size()));
        for (Count i = 0; i < arguments.get_size(); i++) {
          new (retained + i) Argument(arguments[i]);
        }

        View::Vector<Argument> retained_arguments(
            retained, arguments.get_size());
        Key key(generic, retained_arguments);
        entries.insert(key, Ttx::Concept::Reference<Ttx::Model::Type>(type));
        return type;
      });
}
