// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/materializations.hpp"

#include "perimortem/core/hash.hpp"

#include "perimortem/memory/managed/vector.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

static auto combine_hash(
    Unsigned_64 current,
    Unsigned_8 alternative,
    Unsigned_64 value) -> Unsigned_64 {
  current = Core::Hash(alternative).Rehash(current);
  return Core::Hash(value).Rehash(current);
}

static auto matches_parameter(
    Language::Generic::Parameters parameter,
    const Language::Generic::Argument& argument) -> Bool {
  switch (parameter) {
  case Language::Generic::Parameters::Type:
    return argument.is<const Ttx::Model::Type&>();
  case Language::Generic::Parameters::Unsigned_64:
    return argument.is<::Unsigned_64>();
  case Language::Generic::Parameters::Signed_64:
    return argument.is<::Signed_64>();
  case Language::Generic::Parameters::Bool:
    return argument.is<::Bool>();
  }

  return False;
}

auto Language::Materializations::Key::hash() const -> Unsigned_64 {
  Unsigned_64 value = Core::Hash(&formula).get_value();
  value = Core::Hash(arguments.get_size()).Rehash(value);
  const auto* argument_data = arguments.get_data();
  for (Count i = 0; i < arguments.get_size(); i++) {
    value = argument_data[i].visit(
        [&value]() { return combine_hash(value, 0, 0); },
        [&value](const Ttx::Model::Type& type) {
          return combine_hash(value, 1, Core::Hash(&type).get_value());
        },
        [&value](::Unsigned_64 scalar) {
          return combine_hash(value, 2, Core::Hash(scalar).get_value());
        },
        [&value](::Signed_64 scalar) {
          return combine_hash(value, 3, Core::Hash(scalar).get_value());
        },
        [&value](::Bool scalar) {
          return combine_hash(value, 4, Core::Hash(scalar.value).get_value());
        });
  }

  return value;
}

auto Language::Materializations::materialize(
    const Generic& generic,
    Core::View::Vector<Generic::Argument> arguments)
    -> Core::Option<const Ttx::Model::Type&> {
  auto parameters = generic.get_parameterization();
  BAIL_IF(
      &generic.resolve() != &generic ||
      parameters.get_size() != arguments.get_size());

  // A published key contains only complete semantic facts. Formula code never
  // sees a mismatched value and an incomplete Type cannot become cache state.
  const auto* parameter_data = parameters.get_data();
  const auto* argument_data = arguments.get_data();
  for (Count i = 0; i < arguments.get_size(); i++) {
    BAIL_IF(!matches_parameter(parameter_data[i], argument_data[i]));

    const Ttx::Model::Type* type =
        argument_data[i].find<const Ttx::Model::Type&>();
    BAIL_IF(type != nullptr && &type->resolve() != type);
  }

  Key key(generic, arguments);
  auto existing = entries.find(key);
  if (existing) {
    return existing->value.get();
  }

  // Every active frame belongs to the same nested construction transaction.
  // Tainting the complete stack keeps an outer formula from hiding a cycle.
  for (Active* candidate = active; candidate != nullptr;
       candidate = candidate->previous) {
    if (candidate->key == key) {
      for (Active* participant = active; participant != nullptr;
           participant = participant->previous) {
        participant->reentered = True;
      }

      return {};
    }
  }

  Active transaction(key, active);
  active = &transaction;
  auto created = generic.create(arguments, arena);
  active = transaction.previous;

  // The stack is restored before a formula result can publish anything. A
  // nested rejection therefore cannot leak transient state into later retries.
  BAIL_IF(transaction.reentered || !created);

  const Ttx::Model::Type& result = *created;
  BAIL_IF(&result.resolve() != &result);

  // Managed Vector begins every Argument lifetime while Arena keeps the
  // resulting bytes alive after this local handle leaves the transaction.
  Memory::Managed::Vector<Generic::Argument> retained(arena);
  for (Count i = 0; i < arguments.get_size(); i++) {
    retained.insert(argument_data[i]);
  }

  Key retained_key(generic, retained.get_view());
  entries.insert(
      retained_key, Ttx::Concept::Reference<const Ttx::Model::Type>(result));
  return result;
}
