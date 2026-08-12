// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/materializations.hpp"

#include "perimortem/core/hash.hpp"

#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"

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

static auto normalize_argument(
    Language::Generic::Parameters parameter,
    const Ttx::Concept::Abstract& argument)
    -> Core::Option<Language::Generic::Argument> {
  switch (parameter) {
  case Language::Generic::Parameters::Type: {
    // Alias is opaque to materialization. Resolution is the only operation
    // that may reveal its target, while a direct staged Type identity remains
    // valid before that Type can expose its complete Layout.
    const Ttx::Concept::Abstract& selected = argument.visit<Ttx::Model::Alias>(
        [](const Ttx::Model::Alias& alias) -> const Ttx::Concept::Abstract& {
          return alias.resolve();
        },
        [](const Ttx::Concept::Abstract& direct)
            -> const Ttx::Concept::Abstract& { return direct; });
    auto type = selected.select<Ttx::Model::Type>();
    BAIL_IF(!type);
    return Language::Generic::Argument(*type);
  }
  case Language::Generic::Parameters::Unsigned_64: {
    auto constant = argument.select<Language::Constants::Unsigned>();
    BAIL_IF(
        !constant || &constant->get_type() !=
                         &Tetrodotoxin::Library::Dialect::get_unsigned_64());
    return Language::Generic::Argument(constant->get_value());
  }
  case Language::Generic::Parameters::Signed_64: {
    auto constant = argument.select<Language::Constants::Signed>();
    BAIL_IF(
        !constant || &constant->get_type() !=
                         &Tetrodotoxin::Library::Dialect::get_signed_64());
    return Language::Generic::Argument(constant->get_value());
  }
  case Language::Generic::Parameters::Bool: {
    auto constant = argument.select<Language::Constants::Flag>();
    BAIL_IF(
        !constant ||
        &constant->get_type() != &Tetrodotoxin::Library::Dialect::get_bool());
    return Language::Generic::Argument(constant->get_value());
  }
  }

  return {};
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
    const Ttx::Concept::Layout& argument_layout)
    -> Core::Option<const Ttx::Model::Type&> {
  auto parameters = generic.get_parameterization();
  BAIL_IF(parameters.get_size() != argument_layout.get_size());

  // Fitting is a query. Only a newly published key enters the Arena below;
  // retries and cache hits must not accumulate transient normalization state.
  Memory::Dynamic::Vector<Generic::Argument> arguments(parameters.get_size());
  const auto* parameter_data = parameters.get_data();
  for (Count i = 0; i < parameters.get_size(); i++) {
    auto semantic = argument_layout.get_abstract(i);
    BAIL_IF(!semantic);
    auto argument = normalize_argument(parameter_data[i], *semantic);
    BAIL_IF(!argument);
    arguments.insert(*argument);
  }

  return materialize(generic, arguments.get_view());
}

auto Language::Materializations::materialize(
    const Generic& generic,
    Core::View::Vector<Generic::Argument> arguments)
    -> Core::Option<const Ttx::Model::Type&> {
  auto parameters = generic.get_parameterization();
  BAIL_IF(
      &generic.resolve() != &generic ||
      parameters.get_size() != arguments.get_size());

  // A key contains exact semantic facts. A directly selected Type may still
  // be completing its owner-defined Layout; formulas retain that stable
  // identity and must not demand facts that linking has not reached yet.
  const auto* parameter_data = parameters.get_data();
  const auto* argument_data = arguments.get_data();
  for (Count i = 0; i < arguments.get_size(); i++) {
    BAIL_IF(!matches_parameter(parameter_data[i], argument_data[i]));

    const Ttx::Model::Type* type =
        argument_data[i].find<const Ttx::Model::Type&>();
    if (type != nullptr) {
      const Ttx::Concept::Abstract& resolved = type->resolve();
      BAIL_IF(!resolved.is<Ttx::Concept::Invalid>() && &resolved != type);
    }
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
