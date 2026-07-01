// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/format/source.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "tetrodotoxin/syntax/ast/attribute.hpp"
#include "tetrodotoxin/syntax/ast/definition.hpp"
#include "tetrodotoxin/syntax/ast/function.hpp"
#include "tetrodotoxin/syntax/ast/import.hpp"
#include "tetrodotoxin/syntax/ast/layout.hpp"
#include "tetrodotoxin/syntax/ast/member.hpp"
#include "tetrodotoxin/syntax/ast/param.hpp"
#include "tetrodotoxin/syntax/ast/statement.hpp"
#include "tetrodotoxin/syntax/error.hpp"
#include "tetrodotoxin/syntax/expression/expression.hpp"
#include "tetrodotoxin/syntax/pack.hpp"
#include "tetrodotoxin/syntax/ttx.hpp"
#include "tetrodotoxin/syntax/type/declaration.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness TtxSyntax = {
  .name = "Tetrodotoxin::Syntax"_view,
};

static Count parse_error_count = 0;

static auto parse_package(
    Allocator::Arena& arena,
    View::Bytes source,
    View::Bytes source_path = "<inline ttx>"_view) -> Syntax::Ttx {
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(source, false);
  Syntax::Context ctx(tokenizer, source_path);
  Syntax::Ttx package = Syntax::Ttx::parse(ctx);
  parse_error_count = ctx.get_error_count();
  return package;
}

static auto format_package(const Syntax::Ttx& package)
    -> Perimortem::Memory::Dynamic::Bytes {
  return Tetrodotoxin::Format::format(package);
}

static auto find_member(
    View::Vector<Syntax::Ast::Member*> members,
    View::Bytes name) -> const Syntax::Ast::Member* {
  for (Count i = 0; i < members.get_size(); i++) {
    if (members[i] && members[i]->get_definition().get_name() == name) {
      return members[i];
    }
  }

  return nullptr;
}

static auto find_func(
    View::Vector<Syntax::Ast::Function*> funcs,
    View::Bytes name) -> const Syntax::Ast::Function* {
  for (Count i = 0; i < funcs.get_size(); i++) {
    if (funcs[i] && funcs[i]->get_definition().get_name() == name) {
      return funcs[i];
    }
  }

  return nullptr;
}

static auto find_type(
    View::Vector<Syntax::Type::Declaration*> types,
    View::Bytes name) -> const Syntax::Type::Declaration* {
  for (Count i = 0; i < types.get_size(); i++) {
    if (types[i] && types[i]->get_definition().get_name() == name) {
      return types[i];
    }
  }

  return nullptr;
}

static auto expression_is_class(
    const Syntax::Expression::Expression* expression,
    ::Ttx::Lexical::Class::Type expected_class) -> Bool {
  return expression && expression->get_class() == expected_class;
}

PERIMORTEM_UNIT_TEST(TtxSyntax, expression_subtree) {
  Allocator::Arena arena;
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse("value + 3 * other"_view, false);

  Syntax::Context ctx(tokenizer, "<inline expression>"_view);
  const Syntax::Expression::Expression* expression =
      Syntax::Expression::Expression::parse(ctx);

  ASSERT(expression);
  EXPECT_EQ(ctx.get_error_count(), 0);
  EXPECT(expression->get_class() == ::Ttx::Lexical::Class::Type::AddOp);
  ASSERT(expression->get_right());
  EXPECT(expression->get_right()->get_class() == ::Ttx::Lexical::Class::Type::MulOp);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, unary_minus_parse) {
  Allocator::Arena arena;
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse("value - -1"_view, false);

  Syntax::Context ctx(tokenizer, "<inline expression>"_view);
  const Syntax::Expression::Expression* expression =
      Syntax::Expression::Expression::parse(ctx);

  ASSERT(expression);
  EXPECT_EQ(ctx.get_error_count(), 0);
  EXPECT(expression->get_class() == ::Ttx::Lexical::Class::Type::SubOp);
  ASSERT(expression->get_left());
  EXPECT(expression->get_left()->get_class() == ::Ttx::Lexical::Class::Type::Addressable);
  ASSERT(expression->get_right());
  EXPECT(expression->get_right()->get_class() == ::Ttx::Lexical::Class::Type::SubOp);
  ASSERT(expression->get_right()->get_right());
  EXPECT(
      expression->get_right()->get_right()->get_class() ==
      ::Ttx::Lexical::Class::Type::Numeric);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, unary_minus_pack) {
  Allocator::Arena arena;
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse("(-1.0, (.x = -2, .y = value - -3))"_view, false);

  Syntax::Context ctx(tokenizer, "<inline pack>"_view);
  const auto* pack = Syntax::Pack::parse(ctx);

  ASSERT(pack);
  EXPECT_EQ(ctx.get_error_count(), 0);
  ASSERT_EQ(pack->get_fields().get_size(), 2);
  ASSERT(pack->get_fields()[0].value);
  EXPECT(
      pack->get_fields()[0].value->get_class() == ::Ttx::Lexical::Class::Type::SubOp);
  ASSERT(pack->get_fields()[1].value);
  EXPECT(
      pack->get_fields()[1].value->get_class() ==
      ::Ttx::Lexical::Class::Type::PackingStart);

  const auto* nested =
      static_cast<const Syntax::Pack*>(pack->get_fields()[1].value);
  ASSERT_EQ(nested->get_fields().get_size(), 2);
  ASSERT(nested->get_fields()[0].value);
  EXPECT(
      nested->get_fields()[0].value->get_class() ==
      ::Ttx::Lexical::Class::Type::SubOp);
  ASSERT(nested->get_fields()[1].value);
  EXPECT(
      nested->get_fields()[1].value->get_class() ==
      ::Ttx::Lexical::Class::Type::SubOp);
  ASSERT(nested->get_fields()[1].value->get_right());
  EXPECT(
      nested->get_fields()[1].value->get_right()->get_class() ==
      ::Ttx::Lexical::Class::Type::SubOp);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, statement_subtree) {
  Allocator::Arena arena;
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse("return value;"_view, false);

  Syntax::Context ctx(tokenizer, "<inline statement>"_view);
  const Syntax::Ast::Statement* statement = Syntax::Ast::Statement::parse(ctx);

  ASSERT(statement);
  EXPECT_EQ(ctx.get_error_count(), 0);
  EXPECT(statement->get_class() == ::Ttx::Lexical::Class::Type::Return);
  EXPECT_TEXT(statement->get_value()->get_text(), "value"_view);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, continue_statement) {
  Allocator::Arena arena;
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse("continue;"_view, false);

  Syntax::Context ctx(tokenizer, "<inline statement>"_view);
  const Syntax::Ast::Statement* statement = Syntax::Ast::Statement::parse(ctx);

  ASSERT(statement);
  EXPECT_EQ(ctx.get_error_count(), 0);
  EXPECT(statement->get_class() == ::Ttx::Lexical::Class::Type::Continue);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, assignment_statement) {
  Allocator::Arena arena;
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse("self.color.[r, g] = sample.[r, g];"_view, false);

  Syntax::Context ctx(tokenizer, "<inline statement>"_view);
  const Syntax::Ast::Statement* statement = Syntax::Ast::Statement::parse(ctx);

  ASSERT(statement);
  EXPECT_EQ(ctx.get_error_count(), 0);
  EXPECT(statement->get_class() == ::Ttx::Lexical::Class::Type::Assign);
  ASSERT(statement->get_cond());
  EXPECT(statement->get_cond()->get_class() == ::Ttx::Lexical::Class::Type::SwizzleOp);
  ASSERT(statement->get_value());
  EXPECT(statement->get_value()->get_class() == ::Ttx::Lexical::Class::Type::SwizzleOp);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, package_assignment) {
  Allocator::Arena arena;
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse("package.state = value;"_view, false);

  Syntax::Context ctx(tokenizer, "<inline statement>"_view);
  const Syntax::Ast::Statement* statement = Syntax::Ast::Statement::parse(ctx);

  ASSERT(statement);
  EXPECT_EQ(ctx.get_error_count(), 0);
  EXPECT(statement->get_class() == ::Ttx::Lexical::Class::Type::Assign);
  ASSERT(statement->get_cond());
  EXPECT(statement->get_cond()->get_class() == ::Ttx::Lexical::Class::Type::AddressOp);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, range_errors_plain) {
  Allocator::Arena arena;
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse("Vec[Bits_8, 1, 1]"_view, false);
  View::Vector<::Ttx::Lexical::Token> tokens = tokenizer.get_tokens();

  Perimortem::Memory::Dynamic::Bytes diagnostic = Syntax::Error::render(
      "<inline type>"_view, tokens[2], tokens[6], tokenizer.get_source(),
      "Invalid type arguments"_view, "Expected Vec[T, N]"_view, False);

  View::Bytes output = diagnostic.get_view();
  EXPECT(
      Algorithm::search(output, "error: Invalid type arguments"_view) !=
      Count(-1));
  EXPECT(Algorithm::search(output, "<inline type>:1:5:"_view) == Count(-1));
  EXPECT(Algorithm::search(output, "Vec[Bits_8, 1, 1]"_view) != Count(-1));
  EXPECT(Algorithm::search(output, "^~~~~~~~~~~~"_view) != Count(-1));
  EXPECT(
      Algorithm::search(output, "Note: Expected Vec[T, N]"_view) != Count(-1));
  EXPECT(Algorithm::search(output, "\x1b["_view) == Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxSyntax, call_pack) {
  Allocator::Arena arena;
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse("source -> copy_to(.target = _, .count = 4)"_view, false);

  Syntax::Context ctx(tokenizer, "<inline call>"_view);
  const Syntax::Expression::Expression* expression =
      Syntax::Expression::Expression::parse(ctx);

  ASSERT(expression);
  EXPECT_EQ(ctx.get_error_count(), 0);
  EXPECT(expression->get_class() == ::Ttx::Lexical::Class::Type::CallOp);
  ASSERT(expression->get_right());
  EXPECT(expression->get_right()->get_class() == ::Ttx::Lexical::Class::Type::PackingStart);
  const auto* pack = static_cast<const Syntax::Pack*>(expression->get_right());
  ASSERT_EQ(pack->get_fields().get_size(), 2);
  EXPECT_TEXT(pack->get_fields()[0].name, "target"_view);
  EXPECT_TEXT(pack->get_fields()[1].name, "count"_view);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, type_static_dispatch) {
  Allocator::Arena arena;
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse("Color -> from(value)"_view, false);

  Syntax::Context ctx(tokenizer, "<inline call>"_view);
  const Syntax::Expression::Expression* expression =
      Syntax::Expression::Expression::parse(ctx);

  ASSERT(expression);
  EXPECT_EQ(ctx.get_error_count(), 0);
  EXPECT(expression->get_class() == ::Ttx::Lexical::Class::Type::CallOp);
  ASSERT(expression->get_left());
  EXPECT(expression->get_left()->get_class() == ::Ttx::Lexical::Class::Type::Type);
  EXPECT_TEXT(expression->get_text(), "from"_view);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, package_dispatch) {
  Allocator::Arena arena;
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse("Graphics.Image -> decode(bytes)"_view, false);

  Syntax::Context ctx(tokenizer, "<inline call>"_view);
  const Syntax::Expression::Expression* expression =
      Syntax::Expression::Expression::parse(ctx);

  ASSERT(expression);
  EXPECT_EQ(ctx.get_error_count(), 0);
  EXPECT(expression->get_class() == ::Ttx::Lexical::Class::Type::CallOp);
  ASSERT(expression->get_left());
  EXPECT(expression->get_left()->get_class() == ::Ttx::Lexical::Class::Type::Type);
  EXPECT_TEXT(expression->get_text(), "decode"_view);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, attribute_subtree) {
  Allocator::Arena arena;
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse("@extern(.abi = \"c\", .enabled = true)"_view, false);

  Syntax::Context ctx(tokenizer, "<inline attribute>"_view);
  Syntax::Ast::Attribute attribute = Syntax::Ast::Attribute::parse(ctx);

  EXPECT_EQ(ctx.get_error_count(), 0);
  EXPECT_TEXT(attribute.get_name(), "extern"_view);
  ASSERT_EQ(attribute.get_fields().get_size(), 2);
  EXPECT_TEXT(attribute.get_fields()[0].name, "abi"_view);
  EXPECT(
      expression_is_class(
          attribute.get_fields()[0].value, ::Ttx::Lexical::Class::Type::String));
  EXPECT_TEXT(attribute.get_fields()[1].name, "enabled"_view);
  EXPECT(
      expression_is_class(
          attribute.get_fields()[1].value, ::Ttx::Lexical::Class::Type::True));
}

PERIMORTEM_UNIT_TEST(TtxSyntax, pack_subtree) {
  Allocator::Arena arena;
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse("(.width = 4, .name = \"main\")"_view, false);

  Syntax::Context ctx(tokenizer, "<inline pack>"_view);
  const Syntax::Pack* pack = Syntax::Pack::parse(ctx);

  EXPECT_EQ(ctx.get_error_count(), 0);
  ASSERT(pack);
  ASSERT_EQ(pack->get_fields().get_size(), 2);
  EXPECT_TEXT(pack->get_fields()[0].name, "width"_view);
  EXPECT(expression_is_class(
      pack->get_fields()[0].value, ::Ttx::Lexical::Class::Type::Numeric));
  EXPECT_TEXT(pack->get_fields()[1].name, "name"_view);
  EXPECT(
      expression_is_class(pack->get_fields()[1].value, ::Ttx::Lexical::Class::Type::String));
}

PERIMORTEM_UNIT_TEST(TtxSyntax, indexed_pack_subtree) {
  Allocator::Arena arena;
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse("(.10 = 50, .0x41 = 14)"_view, false);

  Syntax::Context ctx(tokenizer, "<inline indexed pack>"_view);
  const Syntax::Pack* pack = Syntax::Pack::parse(ctx);

  EXPECT_EQ(ctx.get_error_count(), 0);
  ASSERT(pack);
  ASSERT_EQ(pack->get_fields().get_size(), 2);
  EXPECT_TEXT(pack->get_fields()[0].index, "10"_view);
  EXPECT_TEXT(pack->get_fields()[1].index, "0x41"_view);
  EXPECT(Syntax::Pack::has_indexed_field(pack->get_fields()));
  EXPECT_NOT(Syntax::Pack::has_named_field(pack->get_fields()));
  EXPECT(expression_is_class(
      pack->get_fields()[0].value, ::Ttx::Lexical::Class::Type::Numeric));
}

PERIMORTEM_UNIT_TEST(TtxSyntax, layout_subtrees) {
  {
    Allocator::Arena arena;
    ::Ttx::Lexical::Tokenizer tokenizer(arena);
    tokenizer.parse("Vec4D"_view, false);

    Syntax::Context ctx(tokenizer, "<inline layout>"_view);
    Syntax::Ast::Layout layout = Syntax::Ast::Layout::parse(ctx);

    EXPECT_EQ(ctx.get_error_count(), 0);
    EXPECT(layout.is_value());
    EXPECT_TEXT(layout.get_value_type().get_name(), "Vec4D"_view);
  }

  {
    Allocator::Arena arena;
    ::Ttx::Lexical::Tokenizer tokenizer(arena);
    tokenizer.parse("[.ok : Bool, .value : Count]"_view, false);

    Syntax::Context ctx(tokenizer, "<inline layout>"_view);
    Syntax::Ast::Layout layout = Syntax::Ast::Layout::parse(ctx);

    EXPECT_EQ(ctx.get_error_count(), 0);
    EXPECT(layout.is_named());
    ASSERT_EQ(layout.get_params().get_size(), 2);
    EXPECT_TEXT(layout.get_params()[0].get_name(), "ok"_view);
    EXPECT_TEXT(layout.get_params()[1].get_type().get_name(), "Count"_view);
  }

  {
    Allocator::Arena arena;
    ::Ttx::Lexical::Tokenizer tokenizer(arena);
    tokenizer.parse("[Vec2D, Vec4D]"_view, false);

    Syntax::Context ctx(tokenizer, "<inline layout>"_view);
    Syntax::Ast::Layout layout = Syntax::Ast::Layout::parse(ctx);

    EXPECT_EQ(ctx.get_error_count(), 0);
    EXPECT(layout.is_unnamed());
    ASSERT_EQ(layout.get_types().get_size(), 2);
    EXPECT_TEXT(layout.get_types()[0].get_name(), "Vec2D"_view);
    EXPECT_TEXT(layout.get_types()[1].get_name(), "Vec4D"_view);
  }
}

PERIMORTEM_UNIT_TEST(TtxSyntax, import_subtrees) {
  {
    Allocator::Arena arena;
    ::Ttx::Lexical::Tokenizer tokenizer(arena);
    tokenizer.parse(
        "import Graphics : Library = ( .source = \"graphics/image.ttx\" );"_view,
        false);

    Syntax::Context ctx(tokenizer, "<inline import>"_view);
    Syntax::Ast::Import import = Syntax::Ast::Import::parse(ctx);

    EXPECT_EQ(ctx.get_error_count(), 0);
    EXPECT_TEXT(import.get_local_name(), "Graphics"_view);
    EXPECT(import.get_kind() == Syntax::PackageKind::Library);
    EXPECT(import.is_file_source());
    EXPECT_TEXT(import.get_file_path(), "\"graphics/image.ttx\""_view);
  }

  {
    Allocator::Arena arena;
    ::Ttx::Lexical::Tokenizer tokenizer(arena);
    tokenizer.parse("import Math : Library = TTX.Math;"_view, false);

    Syntax::Context ctx(tokenizer, "<inline import>"_view);
    Syntax::Ast::Import import = Syntax::Ast::Import::parse(ctx);

    EXPECT_EQ(ctx.get_error_count(), 0);
    EXPECT(import.is_package_source());
    ASSERT_EQ(import.get_package_path().get_size(), 2);
    EXPECT_TEXT(import.get_package_path()[0], "TTX"_view);
    EXPECT_TEXT(import.get_package_path()[1], "Math"_view);
  }
}

PERIMORTEM_UNIT_TEST(TtxSyntax, definition_subtrees) {
  {
    Allocator::Arena arena;
    ::Ttx::Lexical::Tokenizer tokenizer(arena);
    tokenizer.parse("@hidden count : Count = 4;"_view, false);

    Syntax::Context ctx(tokenizer, "<inline definition>"_view);
    Syntax::Ast::Definition definition =
        Syntax::Ast::Definition::parse(ctx);

    EXPECT_EQ(ctx.get_error_count(), 0);
    EXPECT(definition.get_sigil() == ::Ttx::Lexical::Class::Type::ConstHidden);
    EXPECT_TEXT(definition.get_name(), "count"_view);
    EXPECT(definition.is_value());
    EXPECT_TEXT(definition.get_type_ref().get_name(), "Count"_view);
    EXPECT(expression_is_class(
        definition.get_value().get_expression(), ::Ttx::Lexical::Class::Type::Numeric));
  }

  {
    Allocator::Arena arena;
    ::Ttx::Lexical::Tokenizer tokenizer(arena);
    tokenizer.parse("@hidden count : Count = Count -> from(1);"_view, false);

    Syntax::Context ctx(tokenizer, "<inline definition>"_view);
    Syntax::Ast::Definition definition =
        Syntax::Ast::Definition::parse(ctx);

    EXPECT_EQ(ctx.get_error_count(), 0);
    EXPECT(definition.is_value());
    EXPECT_TEXT(definition.get_name(), "count"_view);
    EXPECT(expression_is_class(
        definition.get_value().get_expression(), ::Ttx::Lexical::Class::Type::CallOp));
    EXPECT_TEXT(
        definition.get_value()
            .get_expression()
            ->get_left()
            ->get_type_ref()
            .get_name(),
        "Count"_view);
    ASSERT_EQ(
        definition.get_value()
            .get_expression()
            ->get_right()
            ->get_args()
            .get_size(),
        1);
    EXPECT(expression_is_class(
        definition.get_value().get_expression()->get_right()->get_args()[0],
        ::Ttx::Lexical::Class::Type::Numeric));
  }

  {
    Allocator::Arena arena;
    ::Ttx::Lexical::Tokenizer tokenizer(arena);
    tokenizer.parse(
        "@hidden Size : Struct { @hidden width : Count = 0; }"_view, false);

    Syntax::Context ctx(tokenizer, "<inline definition>"_view);
    Syntax::Ast::DeclarationPrefix prefix =
        Syntax::Ast::DeclarationPrefix::parse(ctx);
    Syntax::Ast::Definition definition =
        Syntax::Ast::Definition::parse_header(ctx);
    Syntax::Type::Declaration* type =
        Syntax::Type::Declaration::parse_declaration(
            ctx, ::Ttx::Lexical::Class::Type::EndOfStream, prefix, definition);

    EXPECT_EQ(ctx.get_error_count(), 0);
    ASSERT(type);
    EXPECT(type->is_scoped_body());
    EXPECT(type->get_kind() == Syntax::DeclarationKind::Struct);
    EXPECT(
        type->get_definition().get_type_ref().get_segments()[0].klass ==
        ::Ttx::Lexical::Class::Type::Struct);
    ASSERT_EQ(type->get_scope().get_size(), 1);
    EXPECT_TEXT(
        type->get_scope()[0]->get_definition().get_name(), "width"_view);
  }

  {
    Allocator::Arena arena;
    ::Ttx::Lexical::Tokenizer tokenizer(arena);
    tokenizer.parse(
        "@hidden Color : Enum[Bits_8](.red = 1, .green = 2);"_view, false);

    Syntax::Context ctx(tokenizer, "<inline definition>"_view);
    Syntax::Ast::DeclarationPrefix prefix =
        Syntax::Ast::DeclarationPrefix::parse(ctx);
    Syntax::Ast::Definition definition =
        Syntax::Ast::Definition::parse_header(ctx);
    Syntax::Type::Declaration* type =
        Syntax::Type::Declaration::parse_declaration(
            ctx, ::Ttx::Lexical::Class::Type::EndOfStream, prefix, definition);

    EXPECT_EQ(ctx.get_error_count(), 0);
    ASSERT(type);
    EXPECT(type->get_kind() == Syntax::DeclarationKind::Enum);
    ASSERT_EQ(type->get_scope().get_size(), 2);
    EXPECT_TEXT(type->get_scope()[0]->get_definition().get_name(), "red"_view);
    EXPECT(
        type->get_scope()[0]->get_definition().get_sigil() ==
        ::Ttx::Lexical::Class::Type::ConstDynamic);
    EXPECT(type->get_scope()[0]->get_definition().is_value());
    EXPECT_TEXT(
        type->get_scope()[0]->get_definition().get_type_ref().get_name(),
        "Bits_8"_view);
    EXPECT(expression_is_class(
        type->get_scope()[0]->get_definition().get_value().get_expression(),
        ::Ttx::Lexical::Class::Type::Numeric));
  }
}

PERIMORTEM_UNIT_TEST(TtxSyntax, function_subtree) {
  Allocator::Arena arena;
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(
      "[.source : View[Bytes], @builtin(.slot = 0) .count : Count] -> "
      "[.ok : Bool, .value : Count] { return (.ok = true, .value = count); }"_view,
      false);

  Syntax::Context ctx(tokenizer, "<inline func>"_view);
  Syntax::Ast::Function func = Syntax::Ast::Function::parse(ctx);

  EXPECT_EQ(ctx.get_error_count(), 0);
  ASSERT_EQ(func.get_params().get_params().get_size(), 2);
  EXPECT_TEXT(func.get_params().get_params()[0].get_name(), "source"_view);
  ASSERT_EQ(
      func.get_params().get_params()[1].get_attributes().get_size(), 1);
  EXPECT_TEXT(
      func.get_params().get_params()[1].get_attributes()[0].get_name(),
      "builtin"_view);
  EXPECT(func.get_returns().is_named());
  ASSERT_EQ(func.get_body().get_size(), 1);
  EXPECT(func.get_body()[0]->get_class() == ::Ttx::Lexical::Class::Type::Return);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, member_subtree) {
  Allocator::Arena arena;
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(
      "// Disabled member\n"
      "/> @extern(.abi = \"c\")\n"
      "@hidden count : Count = 4;"_view,
      false);

  Syntax::Context ctx(tokenizer, "<inline member>"_view);
  const Syntax::Ast::Member* member = Syntax::Ast::Member::parse(ctx);

  ASSERT(member);
  EXPECT_EQ(ctx.get_error_count(), 0);
  EXPECT(member->is_disabled());
  ASSERT_EQ(member->get_documentation().get_lines().get_size(), 1);
  ASSERT_EQ(member->get_attributes().get_size(), 1);
  EXPECT_TEXT(member->get_attributes()[0].get_name(), "extern"_view);
  EXPECT_TEXT(member->get_definition().get_name(), "count"_view);
  EXPECT(
      member->get_definition().get_sigil() ==
      ::Ttx::Lexical::Class::Type::ConstHidden);
  EXPECT(member->get_definition().is_value());
}

PERIMORTEM_UNIT_TEST(TtxSyntax, parse_package_kinds) {
  {
    Allocator::Arena arena;
    Syntax::Ttx package = parse_package(arena, "dialect : Library;\n"_view);
    ASSERT(package.is_valid());
    EXPECT_EQ(parse_error_count, 0);
    EXPECT(package.get_kind() == Syntax::PackageKind::Library);
  }

  {
    Allocator::Arena arena;
    Syntax::Ttx package = parse_package(arena, "dialect : Shader;\n"_view);
    ASSERT(package.is_valid());
    EXPECT_EQ(parse_error_count, 0);
    EXPECT(package.get_kind() == Syntax::PackageKind::Shader);
  }

  {
    Allocator::Arena arena;
    Syntax::Ttx package = parse_package(arena, "dialect : Render;\n"_view);
    ASSERT(package.is_valid());
    EXPECT_EQ(parse_error_count, 0);
    EXPECT(package.get_kind() == Syntax::PackageKind::Render);
  }

  {
    Allocator::Arena arena;
    Syntax::Ttx package = parse_package(arena, "dialect : Entity;\n"_view);
    ASSERT(package.is_valid());
    EXPECT_EQ(parse_error_count, 0);
    EXPECT(package.get_kind() == Syntax::PackageKind::Entity);
  }

  {
    Allocator::Arena arena;
    ::Ttx::Lexical::Tokenizer tokenizer(arena);
    tokenizer.parse("dialect : Render;\n"_view, false);
    EXPECT(
        Syntax::Ttx::detect_kind(tokenizer.get_tokens()) ==
        Syntax::PackageKind::Render);
  }

  {
    Allocator::Arena arena;
    ::Ttx::Lexical::Tokenizer tokenizer(arena);
    tokenizer.parse("dialect : Shader;\n"_view, false);
    EXPECT(
        Syntax::Ttx::detect_kind(tokenizer.get_tokens()) ==
        Syntax::PackageKind::Shader);
  }

  {
    Allocator::Arena arena;
    ::Ttx::Lexical::Tokenizer tokenizer(arena);
    tokenizer.parse("@public draft : Object {}"_view, false);
    EXPECT(
        Syntax::Ttx::detect_kind(tokenizer.get_tokens()) ==
        Syntax::PackageKind::Library);
  }
}

PERIMORTEM_UNIT_TEST(TtxSyntax, non_package_kinds) {
  {
    Allocator::Arena arena;
    Syntax::Ttx package = parse_package(arena, "dialect : Object;\n"_view);
    EXPECT_NOT(package.is_valid());
    EXPECT(parse_error_count != 0);
  }

  {
    Allocator::Arena arena;
    Syntax::Ttx package = parse_package(arena, "dialect : Struct;\n"_view);
    EXPECT_NOT(package.is_valid());
    EXPECT(parse_error_count != 0);
  }
}

PERIMORTEM_UNIT_TEST(TtxSyntax, imports_and_members) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Library;\n"
      "import Math : Library = TTX.Math;\n"
      "@packed()\n"
      "@hidden count : Count = 4;\n"_view);

  ASSERT(package.is_valid());
  EXPECT_EQ(parse_error_count, 0);
  ASSERT_EQ(package.get_imports().get_size(), 1);
  ASSERT_EQ(package.get_members().get_size(), 1);

  const Syntax::Ast::Import& import = package.get_imports()[0];
  EXPECT_TEXT(import.get_local_name(), "Math"_view);
  EXPECT(import.get_kind() == Syntax::PackageKind::Library);
  EXPECT(import.is_package_source());
  ASSERT_EQ(import.get_package_path().get_size(), 2);
  EXPECT_TEXT(import.get_package_path()[0], "TTX"_view);
  EXPECT_TEXT(import.get_package_path()[1], "Math"_view);

  const Syntax::Ast::Member* member = package.get_members()[0];
  ASSERT(member);
  ASSERT_EQ(member->get_attributes().get_size(), 1);
  EXPECT_TEXT(member->get_attributes()[0].get_name(), "packed"_view);
  EXPECT(member->get_attributes()[0].get_fields().is_empty());
  EXPECT_TEXT(member->get_definition().get_name(), "count"_view);
  EXPECT(
      member->get_definition().get_sigil() ==
      ::Ttx::Lexical::Class::Type::ConstHidden);
  EXPECT(member->get_definition().is_value());
}

PERIMORTEM_UNIT_TEST(TtxSyntax, package_scope_member) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Library;\n"
      "@public Size : Struct {\n"
      "  @hidden width : Bits_32 = 0;\n"
      "}\n"_view);

  ASSERT(package.is_valid());
  EXPECT_EQ(parse_error_count, 0);
  ASSERT_EQ(package.get_members().get_size(), 0);
  ASSERT_EQ(package.get_types().get_size(), 1);

  const Syntax::Type::Declaration* type = package.get_types()[0];
  ASSERT(type);
  const Syntax::Ast::Definition& definition = type->get_definition();
  EXPECT_TEXT(definition.get_name(), "Size"_view);
  EXPECT(type->is_scoped_body());
  EXPECT(type->get_kind() == Syntax::DeclarationKind::Struct);
  ASSERT_EQ(type->get_scope().get_size(), 1);
  EXPECT_TEXT(type->get_scope()[0]->get_definition().get_name(), "width"_view);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, missing_colon) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(arena, "package Library;\n"_view);

  EXPECT_NOT(package.is_valid());
  EXPECT(parse_error_count != 0);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, package_unknown_kind) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(arena, "dialect : Texture;\n"_view);

  EXPECT_NOT(package.is_valid());
  EXPECT(parse_error_count != 0);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, mixed_expression) {
  {
    Allocator::Arena arena;
    Syntax::Ttx package = parse_package(
        arena,
        "dialect : Library;\n"
        "@hidden bad : Pair = (1, .name = 2);\n"_view);

    EXPECT(package.is_valid());
    EXPECT(parse_error_count != 0);
  }

  {
    Allocator::Arena arena;
    Syntax::Ttx package = parse_package(
        arena,
        "dialect : Library;\n"
        "@hidden bad : Pair = (.name = 2, 1);\n"_view);

    EXPECT(package.is_valid());
    EXPECT(parse_error_count != 0);
  }
}

PERIMORTEM_UNIT_TEST(TtxSyntax, mixed_attribute_pack) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Library;\n"
      "@mixed(.name = 1, 2)\n"
      "@hidden bad : Count = 0;\n"_view);

  EXPECT(package.is_valid());
  EXPECT(parse_error_count != 0);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, mixed_index_pack) {
  {
    Allocator::Arena arena;
    Syntax::Ttx package = parse_package(
        arena,
        "dialect : Library;\n"
        "@hidden bad : Vec[Bits_8, 4] = (.0 = 1, .name = 2);\n"_view);

    EXPECT(package.is_valid());
    EXPECT(parse_error_count != 0);
  }

  {
    Allocator::Arena arena;
    Syntax::Ttx package = parse_package(
        arena,
        "dialect : Library;\n"
        "@hidden bad : Vec[Bits_8, 4] = (.0 = 1, 2);\n"_view);

    EXPECT(package.is_valid());
    EXPECT(parse_error_count != 0);
  }
}

PERIMORTEM_UNIT_TEST(TtxSyntax, non_integer_pack) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Library;\n"
      "@hidden bad : Vec[Bits_8, 4] = (.'A' = 1);\n"_view);

  EXPECT(package.is_valid());
  EXPECT(parse_error_count != 0);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, pascal_pack_fields) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Library;\n"
      "@hidden Color : Enum[Bits_8](.Red = 1);\n"_view);

  EXPECT(package.is_valid());
  EXPECT_EQ(parse_error_count, 1);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, enum_storage_type) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Library;\n"
      "@hidden Color : Enum(.red = 1);\n"_view);

  EXPECT(package.is_valid());
  EXPECT(parse_error_count != 0);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, inferred_assignment) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Library;\n"
      "@hidden count := 1;\n"_view);

  EXPECT(package.is_valid());
  EXPECT(parse_error_count != 0);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, type_call_recovery) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Library;\n"
      "@hidden bad : View[Bytes] = View[Bytes]();\n"
      "@hidden next : Count = 1;\n"_view);

  EXPECT(package.is_valid());
  EXPECT_EQ(parse_error_count, 1);
  EXPECT(find_member(package.get_members(), "bad"_view));
  EXPECT(find_member(package.get_members(), "next"_view));
}

PERIMORTEM_UNIT_TEST(TtxSyntax, native_func_decls) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Library;\n"
      "@hidden func native[.source : Bytes] -> Bytes;\n"_view);

  EXPECT(package.is_valid());
  EXPECT(parse_error_count != 0);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, external_foreign) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Library;\n"
      "@hidden Api : Struct {\n"
      "  external @hidden func call[] -> Void;\n"
      "}\n"_view);

  EXPECT(package.is_valid());
  EXPECT(parse_error_count != 0);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, foreign_after_member) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Library;\n"
      "@hidden value : Count = 1;\n"
      "@hidden C : Foreign {\n"
      "  external @hidden func call[] -> Void;\n"
      "}\n"_view);

  EXPECT(package.is_valid());
  EXPECT(parse_error_count != 0);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, input_scope) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Render;\n"
      "@input\n"
      "@hidden texture : Sampler2D;\n"_view);

  EXPECT(package.is_valid());
  EXPECT(parse_error_count != 0);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, input_layout_field) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Render;\n"
      "Pixel : Shader {\n"
      "  func main[@input .uv : Vec2D] -> Vec4D {\n"
      "    return Vec4D -> from(0.0, 0.0, 0.0, 1.0);\n"
      "  }\n"
      "}\n"_view);

  EXPECT(package.is_valid());
  EXPECT(parse_error_count != 0);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, render_stage_without_visibility_sigils) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Render;\n"
      "Vertex : Shader {\n"
      "  @input vertex_index : Bits_32;\n"
      "  quad_positions : Vec[Vec2D, 6];\n"
      "  func main[] -> [@output .position : Vec4D] {\n"
      "    return (.position = (0.0, 0.0, 0.0, 1.0));\n"
      "  }\n"
      "}\n"_view);

  EXPECT(package.is_valid());
  EXPECT_EQ(parse_error_count, 0);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, render_stage_visibility_sigil_rejected) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Render;\n"
      "@public Vertex : Shader {\n"
      "  @input\n"
      "  @hidden vertex_index : Bits_32;\n"
      "  @public func main[] -> [@output .position : Vec4D] {\n"
      "    return (.position = (0.0, 0.0, 0.0, 1.0));\n"
      "  }\n"
      "}\n"_view);

  EXPECT(package.is_valid());
  EXPECT(parse_error_count != 0);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, non_address_targets) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Library;\n"
      "@hidden func bad[] -> Void {\n"
      "  value + other = 1;\n"
      "}\n"_view);

  EXPECT(package.is_valid());
  EXPECT(parse_error_count != 0);
}

PERIMORTEM_UNIT_TEST(
    TtxSyntax,
    package_local_alias) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Library;\n"
      "@hidden Vec2D : Alias = Vec[Real_32, 2];\n"
      "@hidden position : Vec2D;\n"_view);

  ASSERT(package.is_valid());
  EXPECT_EQ(parse_error_count, 0);

  const Syntax::Ast::Member* position =
      find_member(package.get_members(), "position"_view);
  ASSERT(position);
  const Syntax::Type::Ref& type = position->get_definition().get_type_ref();
  EXPECT_TEXT(type.get_name(), "Vec2D"_view);

  const Syntax::Ast::Member* alias =
      find_member(package.get_members(), "Vec2D"_view);
  ASSERT(alias);
  const Syntax::Ast::Definition& definition = alias->get_definition();
  ASSERT(definition.is_alias());
  const Syntax::Type::Ref& target = definition.get_alias_target();
  EXPECT_TEXT(target.get_name(), "Vec"_view);
  ASSERT_EQ(target.get_params().get_size(), 2);
  EXPECT_TEXT(target.get_params()[0].get_name(), "Real_32"_view);
  EXPECT(target.get_params()[1].is_size_literal());
}

PERIMORTEM_UNIT_TEST(TtxSyntax, comprehensive_ast) {
  File source;
  ASSERT(source.read("validation/data/ttx/syntax_sample.ttx"_view));

  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena, source.get_view(), "validation/data/ttx/syntax_sample.ttx"_view);

  ASSERT(package.is_valid());
  ASSERT_EQ(parse_error_count, 0);

  ASSERT_EQ(package.get_documentation().get_lines().get_size(), 2);
  EXPECT(package.get_kind() == Syntax::PackageKind::Library);
  ASSERT_EQ(package.get_imports().get_size(), 2);
  ASSERT_EQ(package.get_members().get_size(), 10);
  ASSERT_EQ(package.get_functions().get_size(), 2);
  ASSERT_EQ(package.get_types().get_size(), 3);

  const Syntax::Ast::Import& file_import = package.get_imports()[0];
  EXPECT_TEXT(file_import.get_local_name(), "Graphics"_view);
  EXPECT(file_import.is_file_source());
  EXPECT_TEXT(file_import.get_file_path(), "\"graphics/image.ttx\""_view);

  const Syntax::Ast::Import& package_import = package.get_imports()[1];
  EXPECT_TEXT(package_import.get_local_name(), "Core"_view);
  EXPECT(package_import.is_package_source());
  ASSERT_EQ(package_import.get_package_path().get_size(), 2);
  EXPECT_TEXT(package_import.get_package_path()[0], "TTX"_view);
  EXPECT_TEXT(package_import.get_package_path()[1], "Core"_view);

  const Syntax::Type::Declaration* foreign_api =
      find_type(package.get_types(), "ForeignApi"_view);
  ASSERT(foreign_api);
  ASSERT_EQ(foreign_api->get_attributes().get_size(), 1);
  EXPECT_TEXT(foreign_api->get_attributes()[0].get_name(), "extern"_view);
  ASSERT_EQ(foreign_api->get_attributes()[0].get_fields().get_size(), 2);
  EXPECT(foreign_api->is_scoped_body());
  EXPECT(foreign_api->get_kind() == Syntax::DeclarationKind::Foreign);
  EXPECT(
      foreign_api->get_definition().get_type_ref().get_segments()[0].klass ==
      ::Ttx::Lexical::Class::Type::Foreign);
  ASSERT_EQ(foreign_api->get_scope().get_size(), 0);
  ASSERT_EQ(foreign_api->get_scope_functions().get_size(), 1);

  const Syntax::Ast::Function* inflate = foreign_api->get_scope_functions()[0];
  ASSERT(inflate);
  EXPECT_TEXT(inflate->get_definition().get_name(), "inflate"_view);
  EXPECT(inflate->is_external());
  EXPECT_NOT(inflate->has_body());
  ASSERT_EQ(inflate->get_params().get_params().get_size(), 2);
  EXPECT_TEXT(inflate->get_params().get_params()[0].get_name(), "source"_view);
  EXPECT_TEXT(inflate->get_returns().get_value_type().get_name(), "Bytes"_view);

  const Syntax::Type::Declaration* color =
      find_type(package.get_types(), "Color"_view);
  ASSERT(color);
  EXPECT(color->get_kind() == Syntax::DeclarationKind::Enum);
  ASSERT_EQ(color->get_scope().get_size(), 2);
  EXPECT_TEXT(color->get_scope()[0]->get_definition().get_name(), "red"_view);
  EXPECT_TEXT(color->get_scope()[1]->get_definition().get_name(), "green"_view);
  EXPECT(color->get_scope()[0]->get_definition().is_value());
  EXPECT_TEXT(
      color->get_scope()[0]->get_definition().get_type_ref().get_name(),
      "Bits_8"_view);
  EXPECT(
      color->get_scope()[0]->get_definition().get_sigil() ==
      ::Ttx::Lexical::Class::Type::ConstDynamic);

  const Syntax::Ast::Function* choose_func =
      find_func(package.get_functions(), "choose"_view);
  ASSERT(choose_func);
  ASSERT_EQ(choose_func->get_body().get_size(), 1);
  ASSERT(choose_func->get_body()[0]);
  EXPECT(
      choose_func->get_body()[0]->get_class() == ::Ttx::Lexical::Class::Type::Match);
  ASSERT_EQ(choose_func->get_body()[0]->get_cases().get_size(), 2);

  const Syntax::Ast::Member* empty =
      find_member(package.get_members(), "empty"_view);
  ASSERT(empty);
  EXPECT(empty->get_definition().is_empty());

  const Syntax::Ast::Member* signature =
      find_member(package.get_members(), "signature"_view);
  ASSERT(signature);
  EXPECT(signature->get_definition().get_type_ref().is_generic());
  ASSERT_EQ(
      signature->get_definition().get_type_ref().get_params().get_size(), 2);
  EXPECT(signature->get_definition()
             .get_type_ref()
             .get_params()[1]
             .is_size_literal());
  EXPECT(expression_is_class(
      signature->get_definition().get_value().get_expression(),
      ::Ttx::Lexical::Class::Type::Bytes));

  EXPECT(expression_is_class(
      find_member(package.get_members(), "greeting"_view)
          ->get_definition()
          .get_value()
          .get_expression(),
      ::Ttx::Lexical::Class::Type::String));
  EXPECT(expression_is_class(
      find_member(package.get_members(), "ratio"_view)
          ->get_definition()
          .get_value()
          .get_expression(),
      ::Ttx::Lexical::Class::Type::Float));
  EXPECT(expression_is_class(
      find_member(package.get_members(), "embedded"_view)
          ->get_definition()
          .get_value()
          .get_expression(),
      ::Ttx::Lexical::Class::Type::Embedded));
  EXPECT(expression_is_class(
      find_member(package.get_members(), "config"_view)
          ->get_definition()
          .get_value()
          .get_expression(),
      ::Ttx::Lexical::Class::Type::PackingStart));
  EXPECT(expression_is_class(
      find_member(package.get_members(), "values"_view)
          ->get_definition()
          .get_value()
          .get_expression(),
      ::Ttx::Lexical::Class::Type::ScopeStart));
  EXPECT(expression_is_class(
      find_member(package.get_members(), "span"_view)
          ->get_definition()
          .get_value()
          .get_expression(),
      ::Ttx::Lexical::Class::Type::RangeOp));
  const Syntax::Ast::Member* explicit_expression =
      find_member(package.get_members(), "explicit_expression"_view);
  ASSERT(explicit_expression);
  EXPECT(explicit_expression->get_definition().is_value());
  EXPECT_TEXT(
      explicit_expression->get_definition().get_type_ref().get_name(), "Count"_view);
  EXPECT(expression_is_class(
      explicit_expression->get_definition().get_value().get_expression(),
      ::Ttx::Lexical::Class::Type::Numeric));

  const Syntax::Ast::Member* generated =
      find_member(package.get_members(), "generated"_view);
  ASSERT(generated);
  EXPECT(generated->get_definition().is_value());
  EXPECT(expression_is_class(
      generated->get_definition().get_value().get_expression(),
      ::Ttx::Lexical::Class::Type::ScopeStart));

  const Syntax::Ast::Function* transform =
      find_func(package.get_functions(), "transform"_view);
  ASSERT(transform);
  const Syntax::Ast::Function& func = *transform;
  ASSERT_EQ(func.get_params().get_params().get_size(), 3);
  ASSERT_EQ(
      func.get_params().get_params()[0].get_attributes().get_size(), 1);
  EXPECT(func.get_returns().is_named());
  ASSERT_EQ(func.get_returns().get_params().get_size(), 2);
  ASSERT_EQ(func.get_body().get_size(), 20);

  EXPECT(func.get_body()[0]->is_aligned_declaration());
  EXPECT(func.get_body()[1]->is_aligned_declaration());
  EXPECT(func.get_body()[2]->get_class() == ::Ttx::Lexical::Class::Type::Assign);
  EXPECT(
      func.get_body()[3]->get_operator() ==
      ::Ttx::Lexical::Class::Type::AddAssign);
  EXPECT(
      func.get_body()[4]->get_operator() ==
      ::Ttx::Lexical::Class::Type::SubAssign);
  EXPECT(expression_is_class(
      func.get_body()[4]->get_value(), ::Ttx::Lexical::Class::Type::IndexStart));
  EXPECT(expression_is_class(
      func.get_body()[11]->get_value(), ::Ttx::Lexical::Class::Type::CallOp));
  EXPECT(expression_is_class(
      func.get_body()[12]->get_value(), ::Ttx::Lexical::Class::Type::CallOp));
  EXPECT(func.get_body()[13]->get_class() == ::Ttx::Lexical::Class::Type::If);
  EXPECT(func.get_body()[14]->get_class() == ::Ttx::Lexical::Class::Type::While);
  ASSERT_EQ(func.get_body()[14]->get_body().get_size(), 2);
  EXPECT(
      func.get_body()[14]->get_body()[1]->get_class() ==
      ::Ttx::Lexical::Class::Type::Continue);
  EXPECT(func.get_body()[15]->get_class() == ::Ttx::Lexical::Class::Type::For);
  EXPECT(func.get_body()[15]->get_layout().is_named());
  ASSERT_EQ(func.get_body()[15]->get_layout().get_params().get_size(), 1);
  EXPECT_TEXT(
      func.get_body()[15]->get_layout().get_params()[0].get_name(),
      "i"_view);
  EXPECT(func.get_body()[16]->get_class() == ::Ttx::Lexical::Class::Type::Match);
  ASSERT_EQ(func.get_body()[16]->get_cases().get_size(), 2);
  EXPECT(
      func.get_body()[17]->get_class() ==
      ::Ttx::Lexical::Class::Type::CompileIf);
  EXPECT(
      func.get_body()[18]->get_class() ==
      ::Ttx::Lexical::Class::Type::EndStatement);
  EXPECT(func.get_body()[19]->get_class() == ::Ttx::Lexical::Class::Type::Return);

  const Syntax::Type::Declaration* container =
      find_type(package.get_types(), "Container"_view);
  ASSERT(container);
  EXPECT(container->is_scoped_body());
  EXPECT(container->get_kind() == Syntax::DeclarationKind::Struct);
  ASSERT_EQ(container->get_scope().get_size(), 2);
  const Syntax::Ast::Member* width = container->get_scope()[0];
  ASSERT(width);
  EXPECT(width->is_disabled());
  ASSERT_EQ(width->get_documentation().get_lines().get_size(), 1);
  ASSERT_EQ(width->get_attributes().get_size(), 1);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, golden_data_fixture) {
  File source;
  File expected;
  ASSERT(source.read("validation/data/ttx/syntax_sample.ttx"_view));
  ASSERT(expected.read("validation/data/ttx/syntax_formatted.ttx"_view));

  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena, source.get_view(), "validation/data/ttx/syntax_sample.ttx"_view);

  ASSERT(package.is_valid());
  ASSERT_EQ(parse_error_count, 0);

  Perimortem::Memory::Dynamic::Bytes formatted = format_package(package);
  EXPECT_TEXT(formatted.get_view(), expected.get_view());
}

PERIMORTEM_UNIT_TEST(TtxSyntax, default_2d_shader_fixture) {
  File source;
  ASSERT(source.read("apps/shaders/default_2d.ttx"_view));

  Allocator::Arena arena;
  Syntax::Ttx package =
      parse_package(
          arena, source.get_view(), "apps/shaders/default_2d.ttx"_view);

  ASSERT(package.is_valid());
  EXPECT_EQ(parse_error_count, 0);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, package_format) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Library;\n"
      "import Math : Library = TTX.Math;\n"
      "@packed()\n"
      "@hidden count : Count = 4;\n"_view);

  ASSERT(package.is_valid());
  ASSERT_EQ(parse_error_count, 0);

  Perimortem::Memory::Dynamic::Bytes formatted = format_package(package);

  EXPECT_TEXT(
      formatted.get_view(),
      "//\n"
      "// Place holder TTX documentation\n"
      "//\n"
      "dialect : Library;\n"
      "\n"
      "import Math : Library = TTX.Math;\n"
      "\n"
      "@packed\n"
      "@hidden count : Count = 4;\n"_view);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, sorted_body_groups) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Library;\n"
      "@hidden Vertex : Shader {\n"
      "}\n"
      "@hidden count : Count = 1;\n"
      "@public func util[] -> Void {\n"
      "}\n"
      "@hidden Pixel : Shader {\n"
      "}\n"_view);

  ASSERT(package.is_valid());

  Perimortem::Memory::Dynamic::Bytes formatted = format_package(package);

  EXPECT_TEXT(
      formatted.get_view(),
      "//\n"
      "// Place holder TTX documentation\n"
      "//\n"
      "dialect : Library;\n"
      "\n"
      "@hidden count : Count = 1;\n"
      "\n"
      "@public func util[] -> Void {\n"
      "}\n"
      "\n"
      "@hidden Vertex : Shader {\n"
      "}\n"
      "\n"
      "@hidden Pixel : Shader {\n"
      "}\n"_view);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, scoped_enum_funcs) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Library;\n"
      "@hidden ColorType : Enum[Bits_8] {\n"
      "  rgb = 2;\n"
      "  rgba = 6;\n"
      "  @public func get_channel_count[] -> Count {\n"
      "    return 4;\n"
      "  }\n"
      "}\n"_view);

  ASSERT(package.is_valid());

  Perimortem::Memory::Dynamic::Bytes formatted = format_package(package);

  EXPECT_TEXT(
      formatted.get_view(),
      "//\n"
      "// Place holder TTX documentation\n"
      "//\n"
      "dialect : Library;\n"
      "\n"
      "@hidden ColorType : Enum[Bits_8] {\n"
      "  rgb  = 2;\n"
      "  rgba = 6;\n"
      "\n"
      "  @public func get_channel_count[] -> Count {\n"
      "    return 4;\n"
      "  }\n"
      "}\n"_view);
}

PERIMORTEM_UNIT_TEST(TtxSyntax, compact_nested_packs) {
  Allocator::Arena arena;
  Syntax::Ttx package = parse_package(
      arena,
      "dialect : Library;\n"
      "@hidden quad_positions : Vec[Vec2D, 6] = (\n"
      "  (\n"
      "    .x = -1.0,\n"
      "    .y = -1.0,\n"
      "  ),\n"
      "  (\n"
      "    .x = 1.0,\n"
      "    .y = -1.0,\n"
      "  ),\n"
      "  (\n"
      "    .x = 1.0,\n"
      "    .y = 1.0,\n"
      "  ),\n"
      "  (\n"
      "    .x = -1.0,\n"
      "    .y = -1.0,\n"
      "  ),\n"
      "  (\n"
      "    .x = 1.0,\n"
      "    .y = 1.0,\n"
      "  ),\n"
      "  (\n"
      "    .x = -1.0,\n"
      "    .y = 1.0,\n"
      "  ),\n"
      ");\n"_view);

  ASSERT(package.is_valid());
  ASSERT_EQ(parse_error_count, 0);

  Perimortem::Memory::Dynamic::Bytes formatted = format_package(package);

  EXPECT_TEXT(
      formatted.get_view(),
      "//\n"
      "// Place holder TTX documentation\n"
      "//\n"
      "dialect : Library;\n"
      "\n"
      "@hidden quad_positions : Vec[Vec2D, 6] = (\n"
      "  (.x = -1.0, .y = -1.0),\n"
      "  (.x = 1.0, .y = -1.0),\n"
      "  (.x = 1.0, .y = 1.0),\n"
      "  (.x = -1.0, .y = -1.0),\n"
      "  (.x = 1.0, .y = 1.0),\n"
      "  (.x = -1.0, .y = 1.0),\n"
      ");\n"_view);
}
