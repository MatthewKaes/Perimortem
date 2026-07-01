// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "ttx/lexical/tokenizer.hpp"
#include "tetrodotoxin/resolution/resolver.hpp"
#include "tetrodotoxin/syntax/ast/function.hpp"
#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/ttx.hpp"
#include "tetrodotoxin/validation/abi.hpp"
#include "tetrodotoxin/validation/call.hpp"
#include "tetrodotoxin/validation/render.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;

static Validation::Harness TtxValidation = {
  .name = "Tetrodotoxin::Validation"_view,
};

static auto parse_validation_package(
    Allocator::Arena& arena,
    View::Bytes source) -> Tetrodotoxin::Syntax::Ttx {
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(source, false);
  Tetrodotoxin::Syntax::Context ctx(tokenizer, "<validation>"_view);
  return Tetrodotoxin::Syntax::Ttx::parse(ctx);
}

static auto first_package_func(
    const Tetrodotoxin::Syntax::Ttx& package)
    -> const Tetrodotoxin::Syntax::Ast::Function* {
  View::Vector<Tetrodotoxin::Syntax::Ast::Function*> package_funcs =
      package.get_functions();
  return package_funcs.is_empty() ? nullptr : package_funcs[0];
}

PERIMORTEM_UNIT_TEST(TtxValidation, view_bytes_calls) {
  Allocator::Arena arena;
  Tetrodotoxin::Syntax::Ttx package = parse_validation_package(
      arena,
      "dialect : Library;\n"
      "@hidden Console : Foreign {\n"
      "  external @hidden func print[.data : View[Bytes]] -> [];\n"
      "}\n"
      "@public func run[.data : View[Bytes]] -> [] {\n"
      "  Console -> print(\"Compiler Test\");\n"
      "  Console -> print(data);\n"
      "}\n"_view);

  ASSERT(package.is_valid());
  const Tetrodotoxin::Syntax::Ast::Function* package_func =
      first_package_func(package);
  ASSERT(package_func);

  Dynamic::Vector<Tetrodotoxin::Validation::RuntimeValue> runtime_values;
  runtime_values.insert(
      Tetrodotoxin::Validation::RuntimeValue::view_bytes("data"_view));

  View::Vector<Tetrodotoxin::Syntax::Ast::Statement*> function_body =
      package_func->get_body();
  ASSERT_EQ(function_body.get_size(), 2);
  EXPECT(Tetrodotoxin::Validation::accepts_view_bytes_foreign_call_body(
      runtime_values.get_view(), function_body));

  Tetrodotoxin::Abi::ForeignCall literal_call =
      Tetrodotoxin::Validation::select_foreign_view_bytes_call(
          runtime_values.get_view(), function_body[0]);
  ASSERT(literal_call.is_valid());
  EXPECT_TEXT(literal_call.get_callee_name(), "print"_view);
  EXPECT(literal_call.get_view_bytes_argument().is_view_bytes_literal());
  EXPECT_TEXT(
      literal_call.get_view_bytes_argument().get_literal_payload(),
      "Compiler Test"_view);

  Tetrodotoxin::Abi::ForeignCall runtime_value_call =
      Tetrodotoxin::Validation::select_foreign_view_bytes_call(
          runtime_values.get_view(), function_body[1]);
  ASSERT(runtime_value_call.is_valid());
  EXPECT(runtime_value_call.get_view_bytes_argument().is_view_bytes_runtime());
  EXPECT_EQ(
      runtime_value_call.get_view_bytes_argument().get_runtime_value_index(),
      0);
}

PERIMORTEM_UNIT_TEST(TtxValidation, named_bytes_pack) {
  Allocator::Arena arena;
  Tetrodotoxin::Syntax::Ttx package = parse_validation_package(
      arena,
      "dialect : Library;\n"
      "@hidden Console : Foreign {\n"
      "  external @hidden func print[.data : View[Bytes]] -> [];\n"
      "}\n"
      "@public func run[.data : View[Bytes]] -> [] {\n"
      "  Console -> print(.data = data);\n"
      "}\n"_view);

  ASSERT(package.is_valid());
  const Tetrodotoxin::Syntax::Ast::Function* package_func =
      first_package_func(package);
  ASSERT(package_func);

  Dynamic::Vector<Tetrodotoxin::Validation::RuntimeValue> runtime_values;
  runtime_values.insert(
      Tetrodotoxin::Validation::RuntimeValue::view_bytes("data"_view));

  View::Vector<Tetrodotoxin::Syntax::Ast::Statement*> function_body =
      package_func->get_body();
  ASSERT_EQ(function_body.get_size(), 1);
  EXPECT_NOT(Tetrodotoxin::Validation::select_foreign_view_bytes_call(
                 runtime_values.get_view(), function_body[0])
                 .is_valid());
  EXPECT_NOT(Tetrodotoxin::Validation::accepts_view_bytes_foreign_call_body(
      runtime_values.get_view(), function_body));
}

PERIMORTEM_UNIT_TEST(TtxValidation, host_abi_signature) {
  Allocator::Arena arena;
  Tetrodotoxin::Syntax::Ttx package = parse_validation_package(
      arena,
      "dialect : Library;\n"
      "@public func run[.data : View[Bytes]] -> [] {\n"
      "}\n"_view);

  ASSERT(package.is_valid());
  const Tetrodotoxin::Syntax::Ast::Function* package_func =
      first_package_func(package);
  ASSERT(package_func);

  Dynamic::Vector<Tetrodotoxin::Abi::Argument> abi_arguments;
  Tetrodotoxin::Abi::Type abi_return_type;
  EXPECT(Tetrodotoxin::Validation::select_host_abi_signature(
      *package_func, abi_arguments, abi_return_type));
  ASSERT_EQ(abi_arguments.get_size(), 1);
  EXPECT_TEXT(abi_arguments[0].name, "data"_view);
  EXPECT(
      abi_arguments[0].type.carrier ==
      Tetrodotoxin::Abi::Type::Carrier::ViewBytes);
  EXPECT(abi_return_type.carrier == Tetrodotoxin::Abi::Type::Carrier::Void);
}

PERIMORTEM_UNIT_TEST(TtxValidation, unsupported_host_abi) {
  Allocator::Arena arena;
  Tetrodotoxin::Syntax::Ttx package = parse_validation_package(
      arena,
      "dialect : Library;\n"
      "@public func run[.count : Count] -> Count {\n"
      "}\n"_view);

  ASSERT(package.is_valid());
  const Tetrodotoxin::Syntax::Ast::Function* package_func =
      first_package_func(package);
  ASSERT(package_func);

  Dynamic::Vector<Tetrodotoxin::Abi::Argument> abi_arguments;
  Tetrodotoxin::Abi::Type abi_return_type;
  EXPECT_NOT(Tetrodotoxin::Validation::select_host_abi_signature(
      *package_func, abi_arguments, abi_return_type));
}

PERIMORTEM_UNIT_TEST(TtxValidation, sprite_render_contract) {
  Tetrodotoxin::Resolution::Resolver resolver;
  ASSERT(resolver.resolve("apps/splash_screen.ttx"_view));

  Tetrodotoxin::Resolution::Record* record =
      resolver.find("apps/splash_screen.ttx"_view);
  ASSERT(record);

  EXPECT(Tetrodotoxin::Validation::accepts_render_import_contract(
      resolver, *record, "Default2D"_view, "Graphics.Sprite.renderer"_view));
}

PERIMORTEM_UNIT_TEST(TtxValidation, reject_bad_sprite_render) {
  Tetrodotoxin::Resolution::Resolver resolver;
  ASSERT(resolver.resolve("validation/data/ttx/sprite_shader_bad.ttx"_view));

  Tetrodotoxin::Resolution::Record* record =
      resolver.find("validation/data/ttx/sprite_shader_bad.ttx"_view);
  ASSERT(record);

  EXPECT_NOT(Tetrodotoxin::Validation::accepts_render_import_contract(
      resolver, *record, "Broken"_view, "Graphics.Sprite.renderer"_view));
}
