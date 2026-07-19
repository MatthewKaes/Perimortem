// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/interpreter/definition.hpp"
#include "tetrodotoxin/interpreter/definitions.hpp"
#include "tetrodotoxin/interpreter/dialects/alias.hpp"
#include "tetrodotoxin/interpreter/dialects/group.hpp"
#include "tetrodotoxin/interpreter/dialects/package.hpp"
#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/source.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/types/unsigned_8.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Validation;

/// An address-bearing fixture proves Alias routes are not restricted to Types.
class PackageValue final : public Addressable {
 public:
  PackageValue(View::Bytes name, const Abstract& value)
      : name(name), value(value) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve() const -> const Abstract& override { return value.resolve(); }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    return value.resolve_context(route);
  }

 private:
  View::Bytes name;
  const Abstract& value;
};

// Test contexts use the same total construction path as the interpreter. Each
// fixture below supplies unique names, so narrowing is a checked precondition.
static auto build_namespace(
    Allocator::Arena& arena,
    View::Bytes name,
    View::Vector<Reference<Abstract>> definitions)
    -> const Tetrodotoxin::Model::Namespace& {
  return Tetrodotoxin::Model::Namespace::construct(arena, name, definitions)
      .as<Tetrodotoxin::Model::Namespace>();
}

template <typename dialect_type>
static auto evaluate_dialect(
    const dialect_type& dialect,
    Ttx::Lexical::Cursor& cursor,
    Tetrodotoxin::Model::Source& source,
    const Ttx::Concept::Abstract& imports)
    -> const Abstract& {
  return dialect.evaluate(cursor, source, imports);
}

static Harness DialectTests = {
  .name = "Dialect"_view,
};

PERIMORTEM_UNIT_TEST(DialectTests, dialect_lookup) {
  Allocator::Arena arena;
  Tetrodotoxin::Interpreter::Dialects::Package package;
  const Static::Vector<Reference<Abstract>, 1> installed = {{package}};
  const Tetrodotoxin::Model::Namespace& dialects =
      build_namespace(arena, "Dialects"_view, installed);

  const Abstract& selected = dialects.resolve_context("Package"_view);

  EXPECT(selected.is<Tetrodotoxin::Interpreter::Dialect>());
  EXPECT(selected.is<Tetrodotoxin::Interpreter::Dialects::Package>());
  EXPECT(&selected == &package);
  EXPECT(dialects.resolve_context("Missing"_view).is<Invalid>());
}

PERIMORTEM_UNIT_TEST(DialectTests, duplicate_dialect) {
  Allocator::Arena arena;
  Tetrodotoxin::Interpreter::Dialects::Package first;
  Tetrodotoxin::Interpreter::Dialects::Package second;
  const Static::Vector<Reference<Abstract>, 2> installed = {{first, second}};

  const Abstract& dialects = Tetrodotoxin::Model::Namespace::construct(
      arena, "Dialects"_view, installed);

  EXPECT(dialects.is<Invalid>());
}

PERIMORTEM_UNIT_TEST(DialectTests, dialect_parts) {
  using AliasDefinition = Tetrodotoxin::Interpreter::Definition<
      Tetrodotoxin::Interpreter::Dialects::Alias,
      Ttx::Lexical::Class::Type::Expose>;
  using GroupDefinition = Tetrodotoxin::Interpreter::Definition<
      Tetrodotoxin::Interpreter::Dialects::Group,
      Ttx::Lexical::Class::Type::Private>;
  EXPECT_TEXT(AliasDefinition::get_name(), "alias"_view);
  EXPECT_TEXT(GroupDefinition::get_name(), "group"_view);
  EXPECT(AliasDefinition::accepts(Ttx::Lexical::Class::Type::Expose));
  EXPECT_NOT(AliasDefinition::accepts(Ttx::Lexical::Class::Type::Private));
  EXPECT(GroupDefinition::accepts(Ttx::Lexical::Class::Type::Private));
  EXPECT_NOT(GroupDefinition::accepts(Ttx::Lexical::Class::Type::Expose));
}

PERIMORTEM_UNIT_TEST(DialectTests, package_alias) {
  Allocator::Arena arena;
  Types::Unsigned_8 value;
  Types::Unsigned_8 child;
  Alias value_import("Value"_view, value);
  Alias child_import("Child"_view, child);
  PackageValue item("item"_view, value);
  const Static::Vector<Reference<Abstract>, 3> imported = {{
    value_import,
    child_import,
    item,
  }};
  const Tetrodotoxin::Model::Namespace& types =
      build_namespace(arena, "Types"_view, imported);
  const Static::Vector<Reference<Abstract>, 1> imports = {{types}};
  const Tetrodotoxin::Model::Namespace& outer =
      build_namespace(arena, "Validation.Package"_view, imports);
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "// public package\n"
      "expose Value : alias = Types::Value;\n"
      "expose Copy : alias = Value;\n"
      "expose item : alias = Types::item;\n"
      "// nested exports\n"
      "expose Nested : group {\n"
      "  // nested child\n"
      "  expose Child : alias = Types::Child;\n"
      "  expose Parent : alias = Value;\n"
      "}"_view,
      "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Cursor cursor(source.get_tokenizer(), errors);

  const Abstract& evaluated = evaluate_dialect(package, cursor, source, outer);

  ASSERT(evaluated.is<Tetrodotoxin::Model::Source>());
  EXPECT_NOT(evaluated.is<Type>());
  EXPECT(evaluated.get_name().is_empty());
  EXPECT_EQ(
      evaluated.as<Tetrodotoxin::Model::Source>().get_definitions().get_size(),
      Count(4));
  const Abstract& exported = evaluated.resolve_context("Value"_view);
  ASSERT(exported.is<Alias>());
  EXPECT(&exported.resolve() == &value);
  EXPECT_EQ(exported.as<Alias>().get_documentation().line_count(), Count(2));
  EXPECT_TEXT(
      exported.as<Alias>().get_documentation().get_line(0),
      "public package"_view);
  EXPECT_TEXT(
      exported.as<Alias>().get_documentation().get_line(1),
      value.get_documentation().get_line(0));
  const Abstract& copy = evaluated.resolve_context("Copy"_view);
  ASSERT(copy.is<Alias>());
  EXPECT(&copy.resolve() == &value);
  const Abstract& addressable = evaluated.resolve_context("item"_view);
  ASSERT(addressable.is<Alias>());
  EXPECT(&addressable.resolve() == &value);
  const Abstract& nested = evaluated.resolve_context("Nested"_view);
  EXPECT(nested.is<Tetrodotoxin::Model::Namespace>());
  EXPECT_NOT(nested.is<Type>());
  EXPECT_TEXT(nested.get_documentation().get_line(0), "nested exports"_view);
  const Abstract& nested_child = nested.resolve_context("Child"_view);
  ASSERT(nested_child.is<Alias>());
  EXPECT(&nested_child.resolve() == &child);
  EXPECT_TEXT(
      nested_child.as<Alias>().get_documentation().get_line(0),
      "nested child"_view);
  const Abstract& parent = nested.resolve_context("Parent"_view);
  ASSERT(parent.is<Alias>());
  EXPECT(&parent.resolve() == &value);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, local_collision) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Types::Unsigned_8 value;
  Alias value_import("Value"_view, value);
  const Static::Vector<Reference<Abstract>, 1> imported = {{value_import}};
  const Tetrodotoxin::Model::Namespace& types =
      build_namespace(arena, "Types"_view, imported);
  const Static::Vector<Reference<Abstract>, 1> imports = {{types}};
  const Tetrodotoxin::Model::Namespace& outer =
      build_namespace(arena, "Validation.Package"_view, imports);
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "expose Value : alias = Types::Value;\n"
      "expose Value : alias = Types::Value;"_view,
      "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Cursor cursor(source.get_tokenizer(), errors);

  const Abstract& evaluated = evaluate_dialect(package, cursor, source, outer);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered,
          "Definition name `Value` is already visible in this scope."_view) !=
      Count(-1));
  EXPECT(Algorithm::search(rendered, "Value"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, import_collision) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Types::Unsigned_8 value;
  Alias imported_value("Value"_view, value);
  const Static::Vector<Reference<Abstract>, 1> imports = {{imported_value}};
  const Tetrodotoxin::Model::Namespace& outer =
      build_namespace(arena, "Validation.Package"_view, imports);
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "expose Value : alias = Value;"_view, "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Cursor cursor(source.get_tokenizer(), errors);

  const Abstract& evaluated = evaluate_dialect(package, cursor, source, outer);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered,
          "Definition name `Value` is already visible in this scope."_view) !=
      Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, outer_collision) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Types::Unsigned_8 value;
  Alias value_import("Value"_view, value);
  const Static::Vector<Reference<Abstract>, 1> imported = {{value_import}};
  const Tetrodotoxin::Model::Namespace& types =
      build_namespace(arena, "Types"_view, imported);
  const Static::Vector<Reference<Abstract>, 1> imports = {{types}};
  const Tetrodotoxin::Model::Namespace& outer =
      build_namespace(arena, "Validation.Package"_view, imports);
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "expose Value : alias = Types::Value;\n"
      "expose Nested : group {\n"
      "  expose Value : alias = Types::Value;\n"
      "}"_view,
      "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Cursor cursor(source.get_tokenizer(), errors);

  const Abstract& evaluated = evaluate_dialect(package, cursor, source, outer);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered,
          "Definition name `Value` is already visible in this scope."_view) !=
      Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, wrong_sigil) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  const Tetrodotoxin::Model::Namespace& outer =
      build_namespace(arena, "Validation.Package"_view, {});
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "private Value : alias = Missing;"_view, "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Cursor cursor(source.get_tokenizer(), errors);

  const Abstract& evaluated = evaluate_dialect(package, cursor, source, outer);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered,
          "Definition Dialect `alias` does not accept sigil `private`."_view) !=
      Count(-1));
  EXPECT(Algorithm::search(rendered, "private"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, wrong_dialect) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  const Tetrodotoxin::Model::Namespace& outer =
      build_namespace(arena, "Validation.Package"_view, {});
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "expose Value : missing;"_view, "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Cursor cursor(source.get_tokenizer(), errors);

  const Abstract& evaluated = evaluate_dialect(package, cursor, source, outer);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered,
          "Definition Dialect `missing` is not registered. Expected one of "
          "{`alias`, `group`}."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "missing"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, bad_name_class) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  const Tetrodotoxin::Model::Namespace& outer =
      build_namespace(arena, "Validation.Package"_view, {});
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "expose private : alias = Missing;"_view, "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Cursor cursor(source.get_tokenizer(), errors);

  const Abstract& evaluated = evaluate_dialect(package, cursor, source, outer);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered,
          "Expected a Type or Addressable definition name after sigil "
          "`expose`, but found `private`."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "private"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, invalid_target) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  const Tetrodotoxin::Model::Namespace& outer =
      build_namespace(arena, "Validation.Package"_view, {});
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "expose Value : alias = Missing;"_view, "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Cursor cursor(source.get_tokenizer(), errors);

  const Abstract& evaluated = evaluate_dialect(package, cursor, source, outer);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered, "Alias target `Missing` could not be resolved."_view) !=
      Count(-1));
  EXPECT(Algorithm::search(rendered, "Missing"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, missing_open) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  const Tetrodotoxin::Model::Namespace& outer =
      build_namespace(arena, "Validation.Package"_view, {});
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "expose Nested : group expose"_view, "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Cursor cursor(source.get_tokenizer(), errors);

  const Abstract& evaluated = evaluate_dialect(package, cursor, source, outer);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered, "Expected `{` after Group definition."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "expose"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, missing_close) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  const Tetrodotoxin::Model::Namespace& outer =
      build_namespace(arena, "Validation.Package"_view, {});
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "expose Nested : group {"_view, "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Cursor cursor(source.get_tokenizer(), errors);

  const Abstract& evaluated = evaluate_dialect(package, cursor, source, outer);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered, "Expected `}` after Group definition."_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, missing_assign) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  const Tetrodotoxin::Model::Namespace& outer =
      build_namespace(arena, "Validation.Package"_view, {});
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "expose Value : alias Missing;"_view, "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Cursor cursor(source.get_tokenizer(), errors);

  const Abstract& evaluated = evaluate_dialect(package, cursor, source, outer);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered, "Expected `=` before Alias target."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "Missing"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, missing_end) {
  Allocator::Arena arena;
  Allocator::Arena render_arena;
  Types::Unsigned_8 value;
  Alias imported_value("Value"_view, value);
  const Static::Vector<Reference<Abstract>, 1> imports = {{imported_value}};
  const Tetrodotoxin::Model::Namespace& outer =
      build_namespace(arena, "Validation.Package"_view, imports);
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "expose Copy : alias = Value"_view, "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Cursor cursor(source.get_tokenizer(), errors);

  const Abstract& evaluated = evaluate_dialect(package, cursor, source, outer);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(rendered, "Expected `;` after Alias."_view) !=
      Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, anonymous_source) {
  Allocator::Arena arena;
  const Tetrodotoxin::Model::Namespace& outer = build_namespace(arena, {}, {});
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(View::Bytes{}, View::Bytes{});
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Cursor cursor(source.get_tokenizer(), errors);

  const Abstract& evaluated = evaluate_dialect(package, cursor, source, outer);

  EXPECT(evaluated.is<Tetrodotoxin::Model::Source>());
  EXPECT(evaluated.get_name().is_empty());
  EXPECT(errors.is_empty());
}
