// // Perimortem Engine
// // Copyright © Matt Kaes

// #include "tetrodotoxin/isa/registry.hpp"

// #include "validation/unit_test.hpp"

// using namespace Perimortem::Core;
// using namespace Tetrodotoxin::Isa;
// using namespace Validation;

// static Harness TetrodotoxinRegistry = {
//   .name = "Tetrodotoxin::Registry"_view,
// };

// static auto evaluate_test(Ttx::Lexical::Cursor&, Base::Context&) ->
// Ttx::Type* {
//   return nullptr;
// }

// static auto evaluate_other(Ttx::Lexical::Cursor&, Base::Context&)
//     -> Ttx::Type* {
//   return nullptr;
// }

// static auto lower_test(
//     Tetrodotoxin::Isa::Lowering::Context&,
//     const Tetrodotoxin::Isa::Lowering::Input&) -> Bool {
//   return True;
// }

// PERIMORTEM_UNIT_TEST(TetrodotoxinRegistry, install_find) {
//   Registry registry;

//   EXPECT(registry.install("Library"_view, evaluate_test, lower_test));
//   EXPECT_EQ(registry.get_size(), Count(1));

//   const Dialect* entry = registry.find("Library"_view);
//   ASSERT(entry != nullptr);
//   EXPECT_TEXT(entry->get_name(), "Library"_view);
//   EXPECT(entry->get_evaluator() == evaluate_test);
//   EXPECT(entry->can_lower());
//   EXPECT_NOT(entry->can_lower_package());
//   EXPECT(entry->get_lowerer() == lower_test);
//   EXPECT(registry.find("Missing"_view) == nullptr);
// }

// PERIMORTEM_UNIT_TEST(TetrodotoxinRegistry, package_lower) {
//   Registry registry;

//   EXPECT(registry.install("Library"_view, evaluate_test, lower_test, True));
//   const Dialect* entry = registry.find("Library"_view);
//   ASSERT(entry != nullptr);
//   EXPECT(entry->can_lower());
//   EXPECT(entry->can_lower_package());
// }

// PERIMORTEM_UNIT_TEST(TetrodotoxinRegistry, replace) {
//   Registry registry;

//   EXPECT(registry.install("Library"_view, evaluate_test));
//   EXPECT(registry.install("Library"_view, evaluate_other));
//   EXPECT_EQ(registry.get_size(), Count(1));

//   const Dialect* entry = registry.find("Library"_view);
//   ASSERT(entry != nullptr);
//   EXPECT(entry->get_evaluator() == evaluate_other);
//   EXPECT_NOT(entry->can_lower());
// }

// PERIMORTEM_UNIT_TEST(TetrodotoxinRegistry, replace_lower) {
//   Registry registry;

//   EXPECT(registry.install("Library"_view, evaluate_test));
//   EXPECT(registry.install("Library"_view, evaluate_other, lower_test));
//   EXPECT_EQ(registry.get_size(), Count(1));

//   const Dialect* entry = registry.find("Library"_view);
//   ASSERT(entry != nullptr);
//   EXPECT(entry->get_evaluator() == evaluate_other);
//   EXPECT(entry->get_lowerer() == lower_test);
// }

// PERIMORTEM_UNIT_TEST(TetrodotoxinRegistry, invalid) {
//   Registry registry;

//   EXPECT_NOT(registry.install(View::Bytes(), evaluate_test));
//   EXPECT_NOT(registry.install("Library"_view, nullptr));
//   EXPECT_EQ(registry.get_size(), Count(0));
// }
