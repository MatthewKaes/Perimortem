// // Perimortem Engine
// // Copyright © Matt Kaes

// #include "tetrodotoxin/library/language/generic.hpp"

// #include "perimortem/core/data.hpp"
// #include "perimortem/core/hash.hpp"

// using namespace Perimortem::Core;
// using namespace Perimortem::Memory;
// using namespace Perimortem::Utility;
// using namespace Ttx::Concept;
// using namespace Ttx::Model;

// auto Types::Generic::Materializations::Key::hash() const -> Unsigned_64 {
//   Unsigned_64 value = Hash(&formula.get()).get_value();
//   for (Count i = 0; i < arguments.get_size(); i++) {
//     Unsigned_64 argument = arguments[i].visit(
//         []() -> Unsigned_64 { return 0; },
//         [](const Type& type) { return Hash(&type).Rehash(1); },
//         [](Unsigned_64 scalar) { return Hash(scalar).Rehash(2); },
//         [](Signed_64 scalar) { return Hash(scalar).Rehash(3); },
//         [](Bool scalar) {
//           Unsigned_64 value = scalar == True ? 1 : 0;
//           return Hash(value).Rehash(4);
//         });
//     value = Hash(value).Rehash(argument);
//   }

//   return value;
// }

// auto Types::Generic::Materializations::materialize(
//     const Generic& generic,
//     View::Vector<Argument> arguments) -> Option<const Type&> {
//   View::Vector<Parameters> parameters = generic.get_parameterization();
//   if (&generic.resolve() != &generic ||
//       parameters.get_size() != arguments.get_size()) {
//     return none;
//   }

//   // Validate each parameter in order against the argument list.
//   // The values themselves are not actually validated, on the kind.
//   for (Count i = 0; i < parameters.get_size(); i++) {
//     Bool valid = arguments[i].visit(
//         [&]() { return false; },
//         [&](const Type& type) {
//           return &type.resolve() == &type && parameters[i] ==
//           Parameters::Type;
//         },
//         [&](Unsigned_64) { return parameters[i] == Parameters::Unsigned_64;
//         },
//         [&](Signed_64) { return parameters[i] == Parameters::Signed_64; },
//         [&](Bool) { return parameters[i] == Parameters::Bool; });

//     if (!valid) {
//       return none;
//     }
//   }

//   Key requested(generic, arguments);
//   const auto* existing = entries.find(requested);
//   if (existing != nullptr) {
//     return existing->value.get();
//   }

//   Active* repeated = nullptr;
//   for (Active* invocation = active; invocation != nullptr;
//        invocation = invocation->previous) {
//     if (invocation->key == requested) {
//       repeated = invocation;
//       break;
//     }
//   }

//   if (repeated != nullptr) {
//     for (Active* invocation = active;; invocation = invocation->previous) {
//       invocation->reentered = True;
//       if (invocation == repeated) {
//         break;
//       }
//     }

//     return none;
//   }

//   Active invocation(requested, active);
//   active = &invocation;
//   Option<const Type&> created = generic.create(arguments, arena);
//   active = invocation.previous;
//   if (invocation.reentered) {
//     return none;
//   }

//   return created.visit(
//       [](const None&) -> Option<const Type&> { return none; },
//       [&](const Type& type) -> Option<const Type&> {
//         if (&type.resolve() != &type) {
//           return none;
//         }

//         Argument* retained = Data::cast<Argument>(
//             arena.allocate(sizeof(Argument) * arguments.get_size()));
//         for (Count i = 0; i < arguments.get_size(); i++) {
//           new (retained + i) Argument(arguments[i]);
//         }

//         View::Vector<Argument> retained_arguments(
//             retained, arguments.get_size());
//         Key key(generic, retained_arguments);
//         entries.insert(key, Reference<Type>(type));
//         return type;
//       });
// }
