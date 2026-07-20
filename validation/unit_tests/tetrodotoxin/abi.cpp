// // Perimortem Engine
// // Copyright © Matt Kaes

// #include "validation/unit_test.hpp"

// #include "perimortem/core/static/vector.hpp"
// #include "perimortem/core/algorithm/search.hpp"

// #include "perimortem/memory/allocator/arena.hpp"
// #include "perimortem/memory/managed/vector.hpp"

// #include "tetrodotoxin/abi/symbol.hpp"
// #include "tetrodotoxin/compiler/program.hpp"
// #include "tetrodotoxin/compiler/target/cpp.hpp"
// #include "tetrodotoxin/standard/types.hpp"
// #include "ttx/type.hpp"

// using namespace Perimortem::Core;
// using namespace Perimortem::Memory;
// using namespace Tetrodotoxin;
// using namespace Validation;

// static Harness TetrodotoxinAbi = {
//   .name = "Tetrodotoxin::ABI"_view,
// };

// PERIMORTEM_UNIT_TEST(TetrodotoxinAbi, function_tables_define_abi_paths) {
//   Ttx::Type scalar("Scalar"_view);
//   Static::Vector<Ttx::Member, 1> parameters = {{
//     Ttx::Member("self"_view, scalar),
//   }};
//   Static::Vector<Ttx::Function, 1> type_functions = {{
//     Ttx::Function(
//         "identity"_view, Ttx::Layout(parameters), Ttx::Layout(parameters)),
//   }};
//   Static::Vector<Ttx::Function, 1> addressable_functions = {{
//     Ttx::Function(
//         "identity"_view, Ttx::Layout(parameters), Ttx::Layout(parameters)),
//   }};
//   Ttx::Type owner(
//       "Owner"_view, View::Vector<Ttx::Member>(),
//       View::Vector<Ttx::Type::Reference>(), type_functions,
//       addressable_functions);

//   const Ttx::Function* type = owner.find_type_function("identity"_view);
//   const Ttx::Function* addressable =
//       owner.find_addressable_function("identity"_view);
//   ASSERT(type != nullptr);
//   ASSERT(addressable != nullptr);
//   EXPECT(&owner.get_type_functions()[0] == type);
//   EXPECT(&owner.get_addressable_functions()[0] == addressable);
//   EXPECT_EQ(type->get_parameters().get_member_count(), Count(1));
//   EXPECT_EQ(addressable->get_parameters().get_member_count(), Count(1));

//   Allocator::Arena arena;
//   Managed::Vector<Abi::Type> identities(arena);
//   identities.insert(Abi::Type::create(arena, "Validation.Scalar"_view,
//   scalar)); View::Bytes type_symbol = Abi::Symbol::exported(
//       arena, "Validation.Owner.Type"_view, *type, identities);
//   View::Bytes addressable_symbol = Abi::Symbol::exported(
//       arena, "Validation.Owner.Addressable"_view, *addressable, identities);
//   EXPECT(type_symbol != addressable_symbol);
// }

// PERIMORTEM_UNIT_TEST(TetrodotoxinAbi, export_ignores_source_placement) {
//   Static::Vector<Ttx::Function, 1> functions = {{
//     Ttx::Function("entry"_view, Ttx::Layout(), Ttx::Layout()),
//   }};
//   Ttx::Type owner(
//       "Owner"_view, View::Vector<Ttx::Member>(),
//       View::Vector<Ttx::Type::Reference>(), functions);
//   const Ttx::Function* function = owner.find_type_function("entry"_view);
//   ASSERT(function != nullptr);

//   Allocator::Arena arena;
//   View::Bytes source_a = Abi::Symbol::type(
//       arena, "Validation.Unit"_view, "source_a"_view, "Owner"_view,
//       *function);
//   View::Bytes source_b = Abi::Symbol::type(
//       arena, "Validation.Unit"_view, "source_b"_view, "Owner"_view,
//       *function);
//   View::Bytes other_unit = Abi::Symbol::type(
//       arena, "Validation.Other"_view, "source_a"_view, "Owner"_view,
//       *function);
//   View::Bytes nested_a = Abi::Symbol::type(
//       arena, "Validation.Unit"_view, "source_a"_view, "A.Owner"_view,
//       *function);
//   View::Bytes nested_b = Abi::Symbol::type(
//       arena, "Validation.Unit"_view, "source_a"_view, "B.Owner"_view,
//       *function);
//   View::Bytes exported_a = Abi::Symbol::exported(
//       arena, "Validation.Unit.Owner.Type"_view, *function, {});
//   View::Bytes exported_b = Abi::Symbol::exported(
//       arena, "Validation.Unit.Owner.Type"_view, *function, {});

//   EXPECT(source_a != source_b);
//   EXPECT(source_a != other_unit);
//   EXPECT(nested_a != nested_b);
//   EXPECT_TEXT(exported_a, exported_b);
// }

// PERIMORTEM_UNIT_TEST(TetrodotoxinAbi, qualified_parameter_identity) {
//   Ttx::Type left("Thing"_view);
//   Ttx::Type right("Thing"_view);
//   Static::Vector<Ttx::Member, 1> left_parameters = {{
//     Ttx::Member("value"_view, left),
//   }};
//   Static::Vector<Ttx::Member, 1> right_parameters = {{
//     Ttx::Member("value"_view, right),
//   }};
//   Ttx::Function left_function(
//       "use"_view, Ttx::Layout(left_parameters), Ttx::Layout());
//   Ttx::Function right_function(
//       "use"_view, Ttx::Layout(right_parameters), Ttx::Layout());

//   Allocator::Arena arena;
//   Managed::Vector<Abi::Type> identities(arena);
//   identities.insert(Abi::Type::create(arena, "PackageA.Thing"_view, left));
//   identities.insert(Abi::Type::create(arena, "PackageB.Thing"_view, right));
//   View::Bytes left_symbol = Abi::Symbol::exported(
//       arena, "Validation.Api.Type"_view, left_function, identities);
//   View::Bytes right_symbol = Abi::Symbol::exported(
//       arena, "Validation.Api.Type"_view, right_function, identities);

//   EXPECT_NOT(left_symbol.is_empty());
//   EXPECT_NOT(right_symbol.is_empty());
//   EXPECT(left_symbol != right_symbol);
// }

// PERIMORTEM_UNIT_TEST(TetrodotoxinAbi, unpublished_parameter_identity) {
//   Ttx::Type private_type("Private"_view);
//   Static::Vector<Ttx::Member, 1> parameters = {{
//     Ttx::Member("value"_view, private_type),
//   }};
//   Ttx::Function function("use"_view, Ttx::Layout(parameters), Ttx::Layout());

//   Allocator::Arena arena;
//   EXPECT(
//       Abi::Symbol::exported(arena, "Validation.Api.Type"_view, function, {})
//           .is_empty());
// }

// PERIMORTEM_UNIT_TEST(TetrodotoxinAbi, standard_alias_uses_canonical_identity)
// {
//   const Ttx::Type* count = Standard::Types::find_type("Count"_view);
//   ASSERT(count != nullptr);

//   Ttx::Type alias = Ttx::Type::alias("LocalCount"_view, *count);
//   Static::Vector<Ttx::Member, 1> direct_parameters = {{
//     Ttx::Member("value"_view, *count),
//   }};
//   Static::Vector<Ttx::Member, 1> alias_parameters = {{
//     Ttx::Member("value"_view, alias),
//   }};
//   Ttx::Function direct(
//       "use"_view, Ttx::Layout(direct_parameters), Ttx::Layout());
//   Ttx::Function projected(
//       "use"_view, Ttx::Layout(alias_parameters), Ttx::Layout());

//   Allocator::Arena arena;
//   Managed::Vector<Abi::Type> identities(arena);
//   identities.insert(
//       Abi::Type::create(
//           arena, "Tetrodotoxin.Standard.Count"_view, count->canonical()));
//   View::Bytes direct_symbol = Abi::Symbol::exported(
//       arena, "Validation.Api.Type"_view, direct, identities);
//   View::Bytes projected_symbol = Abi::Symbol::exported(
//       arena, "Validation.Api.Type"_view, projected, identities);
//   EXPECT_NOT(direct_symbol.is_empty());
//   EXPECT_TEXT(direct_symbol, projected_symbol);
// }

// PERIMORTEM_UNIT_TEST(TetrodotoxinAbi, unmapped_addressable_cpp_boundary) {
//   Allocator::Arena arena;
//   Ttx::Type* object = arena.reserve<Ttx::Type>();
//   Static::Vector<Ttx::Member, 1> parameters = {{
//     Ttx::Member::reserved_type("self"_view, object),
//   }};
//   Static::Vector<Ttx::Function, 1> functions = {{
//     Ttx::Function("touch"_view, Ttx::Layout(parameters), Ttx::Layout()),
//   }};
//   new (object) Ttx::Type(
//       "Object"_view, View::Vector<Ttx::Member>(),
//       View::Vector<Ttx::Type::Reference>(), View::Vector<Ttx::Function>(),
//       functions);

//   Managed::Vector<Abi::Type> identities(arena);
//   identities.insert(
//       Abi::Type::create(arena, "Validation.Object"_view, *object));
//   const Abi::Export* export_ = Abi::Export::create(
//       arena, "Validation.Object.Addressable"_view, functions[0],
//       "internal_object_touch"_view, identities);
//   Compiler::Program program;
//   ASSERT(export_ != nullptr);
//   ASSERT(program.expose(identities[0]));
//   ASSERT(program.expose(*export_));

//   auto header = Compiler::Target::Cpp::build_header(program);
//   EXPECT(
//       Algorithm::search(header, "at least one parameter or result type"_view)
//       != Count(-1));
//   EXPECT(Algorithm::search(header, "extern \"C\""_view) == Count(-1));
// }
