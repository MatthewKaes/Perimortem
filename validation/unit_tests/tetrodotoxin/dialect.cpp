// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/interpreter/definition.hpp"
#include "tetrodotoxin/interpreter/definitions.hpp"
#include "tetrodotoxin/interpreter/dialects/alias.hpp"
#include "tetrodotoxin/interpreter/dialects/group.hpp"
#include "tetrodotoxin/interpreter/dialects/package.hpp"
#include "tetrodotoxin/model/dependencies/package.hpp"
#include "tetrodotoxin/model/dependencies/source.hpp"
#include "tetrodotoxin/model/dependency.hpp"
#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/package.hpp"
#include "tetrodotoxin/model/packages/interpreted.hpp"
#include "tetrodotoxin/model/packages/precompiled.hpp"
#include "tetrodotoxin/model/source.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/addressables/writable.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/exports.hpp"
#include "ttx/model/types/unsigned_8.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Validation;

// A typed address fixture proves Alias routes retain data identity.
class PackageValue final : public Addressable {
 public:
  PackageValue(View::Bytes name, const Type& type) : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Abstract& override { return type; }

 private:
  View::Bytes name;
  const Type& type;
};

class ReadOnlyPackageValue final : public Addressable {
 public:
  ReadOnlyPackageValue(View::Bytes name, const Type& type)
      : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Abstract& override { return type; }

 private:
  View::Bytes name;
  const Type& type;
};

class WritablePackageValue final : public Ttx::Model::Addressables::Writable {
 public:
  WritablePackageValue(View::Bytes name, const Type& type)
      : name(name), type(type), read_only(name, type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Abstract& override { return type; }
  auto get_read_only() const -> const Addressable& override {
    return read_only;
  }

 private:
  View::Bytes name;
  const Type& type;
  ReadOnlyPackageValue read_only;
};

class StateDialect final {
 public:
  static constexpr View::Bytes name = "slot"_view;

  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      View::Bytes name,
      const Documentation&,
      View::Vector<Ttx::Lexical::Token> modifiers,
      View::Vector<Reference<Abstract>>) -> const Abstract& {
    if (modifiers.is_empty()) {
      return Invalid::get_invalid();
    }

    cursor.consume();
    const Ttx::Lexical::Token end_statement = cursor.require(
        Ttx::Lexical::Code::Type::EndStatement,
        "Expected `;` after test state definition."_view);
    if (!end_statement.is_valid()) {
      return Invalid::get_invalid();
    }

    static Types::Unsigned_8 type;
    return cursor.get_arena().construct<WritablePackageValue>(name, type);
  }
};

// Namespace fixtures are produced inside the Source transaction so dependency
// tests cannot bypass Source ownership merely to seed an imported context.
class NamespaceDialect final : public Tetrodotoxin::Model::Dialect {
 public:
  NamespaceDialect(
      View::Bytes name,
      View::Bytes namespace_name,
      View::Vector<Reference<Abstract>> definitions)
      : name(name), namespace_name(namespace_name), definitions(definitions) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto evaluate(Ttx::Lexical::Cursor& cursor, const Abstract&) const
      -> const Abstract& override {
    return Tetrodotoxin::Model::Namespace::construct(
        cursor.get_arena(), namespace_name, definitions);
  }

 private:
  View::Bytes name;
  View::Bytes namespace_name;
  View::Vector<Reference<Abstract>> definitions;
};

// Test contexts use the same total Namespace construction path as production.
// Every fixture supplies unique names so narrowing is a checked precondition.
static auto build_namespace(
    Allocator::Arena& arena,
    View::Bytes name,
    View::Vector<Reference<Abstract>> definitions)
    -> const Tetrodotoxin::Model::Namespace& {
  return Tetrodotoxin::Model::Namespace::construct(arena, name, definitions)
      .assume<Tetrodotoxin::Model::Namespace>();
}

static auto evaluate_dialect(
    const Tetrodotoxin::Model::Dialect& dialect,
    Ttx::Lexical::Errors& errors,
    Tetrodotoxin::Model::Source& source) -> const Abstract& {
  return source.evaluate(dialect, errors);
}

static auto add_dependency(
    Tetrodotoxin::Model::Source& source,
    const Tetrodotoxin::Model::Dialect& dialect,
    View::Bytes name,
    const Tetrodotoxin::Model::Source& provider,
    const Exports& target) -> Bool {
  const Tetrodotoxin::Model::Dependency& dependency =
      source.get_arena().construct<Tetrodotoxin::Model::Dependencies::Source>(
          dialect, provider.get_path(), provider, name, target,
          target.get_documentation());
  return source.depend(dependency);
}

static auto add_dependency(
    Tetrodotoxin::Model::Source& source,
    const Tetrodotoxin::Model::Dialect& dialect,
    View::Bytes name,
    View::Bytes package_name,
    const Tetrodotoxin::Model::Package& package) -> Bool {
  const Tetrodotoxin::Model::Dependency& dependency =
      source.get_arena().construct<Tetrodotoxin::Model::Dependencies::Package>(
          dialect, package_name, name, package, package.get_documentation());
  return source.depend(dependency);
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

  EXPECT(dialects.is<Exports>());
  EXPECT_EQ(dialects.get_export_count(), Count(1));
  EXPECT(&dialects.get_export(0) == &package);
  EXPECT(dialects.get_export(1).is<Invalid>());
  EXPECT(selected.is<Tetrodotoxin::Model::Dialect>());
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

PERIMORTEM_UNIT_TEST(DialectTests, export_boundary) {
  Allocator::Arena arena;
  Types::Unsigned_8 type;
  PackageValue hidden("Hidden"_view, type);
  PackageValue visible("Visible"_view, type);
  Tetrodotoxin::Model::Namespace exports(arena, "Public"_view);

  EXPECT(exports.add_root(hidden));
  EXPECT(exports.add_root(visible));
  EXPECT(exports.add_export(visible));

  EXPECT_EQ(exports.get_export_count(), Count(1));
  EXPECT(&exports.get_export(0) == &visible);
  EXPECT(&exports.resolve_context("Visible"_view) == &visible);
  EXPECT(exports.resolve_context("Hidden"_view).is<Invalid>());

  Alias imported("Provider"_view, exports);
  EXPECT(&imported.resolve_context("Visible"_view) == &visible);
  EXPECT(imported.resolve_context("Hidden"_view).is<Invalid>());

  Tetrodotoxin::Model::Packages::Precompiled package(arena, exports);
  EXPECT(&package.resolve_context("Visible"_view) == &visible);
  EXPECT(package.resolve_context("Hidden"_view).is<Invalid>());
}

PERIMORTEM_UNIT_TEST(DialectTests, dialect_parts) {
  using AliasDefinition = Tetrodotoxin::Interpreter::Definition<
      Tetrodotoxin::Interpreter::Dialects::Alias,
      Ttx::Lexical::Code::Type::Public>;
  using GroupDefinition = Tetrodotoxin::Interpreter::Definition<
      Tetrodotoxin::Interpreter::Dialects::Group,
      Ttx::Lexical::Code::Type::Private>;
  const Static::Vector<Ttx::Lexical::Token, 1> public_prefix = {{
    Ttx::Lexical::Token(0, 0, 0, 6, Ttx::Lexical::Code::Type::Public),
  }};
  const Static::Vector<Ttx::Lexical::Token, 1> private_prefix = {{
    Ttx::Lexical::Token(0, 0, 0, 7, Ttx::Lexical::Code::Type::Private),
  }};
  EXPECT_TEXT(AliasDefinition::get_name(), "alias"_view);
  EXPECT_TEXT(GroupDefinition::get_name(), "group"_view);
  EXPECT(AliasDefinition::accepts(public_prefix));
  EXPECT_NOT(AliasDefinition::accepts(private_prefix));
  EXPECT(GroupDefinition::accepts(private_prefix));
  EXPECT_NOT(GroupDefinition::accepts(public_prefix));
}

PERIMORTEM_UNIT_TEST(DialectTests, definition_publication) {
  using StateDefinition = Tetrodotoxin::Interpreter::Definition<
      StateDialect, Ttx::Lexical::Code::Type::Public,
      Ttx::Lexical::Code::Type::Private, Ttx::Lexical::Code::Type::Expose,
      Ttx::Lexical::Code::Type::State, Ttx::Lexical::Code::Type::Const>;
  using StateDefinitions =
      Tetrodotoxin::Interpreter::Definitions<StateDefinition>;

  Tetrodotoxin::Model::Source source(
      "private state hidden : slot;\n"
      "public state open : slot;\n"
      "expose state observed : slot;"_view,
      "state.ttx"_view);
  Tetrodotoxin::Model::Namespace roots(source.get_arena(), "Internal"_view);
  Tetrodotoxin::Model::Namespace exports(source.get_arena(), "Public"_view);
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Cursor cursor(source.get_tokenizer(), errors);

  const Abstract& hidden =
      StateDefinitions::evaluate(cursor, roots, exports, {});
  const Abstract& open = StateDefinitions::evaluate(cursor, roots, exports, {});
  const Abstract& exposed =
      StateDefinitions::evaluate(cursor, roots, exports, {});

  EXPECT_NOT(hidden.is<Invalid>());
  EXPECT_NOT(open.is<Invalid>());
  EXPECT_NOT(exposed.is<Invalid>());

  EXPECT(hidden.is<Ttx::Model::Addressables::Writable>());
  EXPECT(open.is<Ttx::Model::Addressables::Writable>());
  EXPECT(exposed.is<Ttx::Model::Addressables::Writable>());
  EXPECT(roots.resolve_context("hidden"_view).is<Invalid>());
  EXPECT(roots.resolve_context("open"_view).is<Invalid>());
  EXPECT(roots.resolve_context("observed"_view).is<Invalid>());
  EXPECT_EQ(roots.get_export_count(), Count(0));

  EXPECT(exports.resolve_context("hidden"_view).is<Invalid>());
  EXPECT(exports.resolve_context("open"_view)
             .is<Ttx::Model::Addressables::Writable>());
  const Abstract& observed = exports.resolve_context("observed"_view);
  EXPECT(observed.is<Addressable>());
  EXPECT_NOT(observed.is<Ttx::Model::Addressables::Writable>());
  EXPECT_EQ(exports.get_export_count(), Count(2));
  EXPECT(&exports.get_export(0) == &exports.resolve_context("open"_view));
  EXPECT(&exports.get_export(1) == &observed);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, expose_requires_state) {
  using StateDefinition = Tetrodotoxin::Interpreter::Definition<
      StateDialect, Ttx::Lexical::Code::Type::Expose,
      Ttx::Lexical::Code::Type::Const>;
  using StateDefinitions =
      Tetrodotoxin::Interpreter::Definitions<StateDefinition>;

  Allocator::Arena render_arena;
  Tetrodotoxin::Model::Source source(
      "expose const observed : slot;"_view, "state.ttx"_view);
  Tetrodotoxin::Model::Namespace roots(source.get_arena(), "Internal"_view);
  Tetrodotoxin::Model::Namespace exports(source.get_arena(), "Public"_view);
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Cursor cursor(source.get_tokenizer(), errors);

  const Abstract& evaluated =
      StateDefinitions::evaluate(cursor, roots, exports, {});

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  EXPECT(
      Algorithm::search(
          errors.render_message(render_arena, 0),
          "`expose` is legal only as the `expose state` prefix."_view) !=
      Count(-1));
  EXPECT(roots.resolve_context("observed"_view).is<Invalid>());
  EXPECT_EQ(exports.get_export_count(), Count(0));
}

PERIMORTEM_UNIT_TEST(DialectTests, package_alias) {
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
  NamespaceDialect types_dialect("Fixture"_view, "Types"_view, imported);
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source types_source({}, "fixture.ttx"_view);
  Ttx::Lexical::Errors types_errors;
  const Abstract& types_root =
      types_source.evaluate(types_dialect, types_errors);
  ASSERT(types_root.is<Tetrodotoxin::Model::Namespace>());
  const Tetrodotoxin::Model::Namespace& types =
      types_root.assume<Tetrodotoxin::Model::Namespace>();
  Allocator::Arena package_arena;
  const Tetrodotoxin::Model::Namespace& math_exports =
      build_namespace(package_arena, {}, {});
  Tetrodotoxin::Model::Packages::Precompiled math(package_arena, math_exports);
  Tetrodotoxin::Model::Source source(
      "// public package\n"
      "public Value : alias = Types::Value;\n"
      "public Copy : alias = Value;\n"
      "public item : alias = Types::item;\n"
      "// nested exports\n"
      "public Nested : group {\n"
      "  // nested child\n"
      "  public Child : alias = Types::Child;\n"
      "  public Parent : alias = Value;\n"
      "}"_view,
      "package.ttx"_view);
  EXPECT(add_dependency(source, package, "Types"_view, types_source, types));
  EXPECT(add_dependency(
      source, package, "Math"_view, "Perimortem.Math"_view, math));
  Ttx::Lexical::Errors errors;

  const Abstract& evaluated = evaluate_dialect(package, errors, source);

  ASSERT(evaluated.is<Tetrodotoxin::Model::Package>());
  ASSERT(evaluated.is<Tetrodotoxin::Model::Packages::Interpreted>());
  EXPECT_NOT(evaluated.is<Tetrodotoxin::Model::Source>());
  EXPECT_NOT(evaluated.is<Type>());
  EXPECT(evaluated.is<Exports>());
  EXPECT(evaluated.get_name().is_empty());
  const Tetrodotoxin::Model::Package& interpreted =
      evaluated.assume<Tetrodotoxin::Model::Package>();
  EXPECT_EQ(interpreted.get_export_count(), Count(4));
  ASSERT_EQ(interpreted.get_dependencies().get_size(), Count(1));
  EXPECT(&interpreted.get_dependencies()[0].get() == &math);
  const auto sources =
      evaluated.assume<Tetrodotoxin::Model::Packages::Interpreted>()
          .get_sources();
  ASSERT_EQ(sources.get_size(), Count(2));
  EXPECT(&sources[0].get() == &source);
  EXPECT(&sources[1].get() == &types_source);
  ASSERT_EQ(source.get_roots().get_size(), Count(1));
  EXPECT_EQ(source.get_dependencies().get_size(), Count(2));
  EXPECT(&source.get_roots()[0].get_definition() == &evaluated);
  EXPECT(source.get_roots()[0]
             .get_definition()
             .is<Tetrodotoxin::Model::Package>());
  EXPECT(&source.get_roots()[0].get_dialect() == &package);
  const Abstract& imported_types = source.resolve_context("Types"_view);
  ASSERT(imported_types.is<Alias>());
  EXPECT(&imported_types.resolve() == &types);
  EXPECT(imported_types.resolve().is<Exports>());
  const Abstract& exported = evaluated.resolve_context("Value"_view);
  ASSERT(exported.is<Alias>());
  EXPECT(&exported.resolve() == &value);
  EXPECT(&interpreted.get_export(0) == &exported);
  EXPECT(interpreted.get_export(4).is<Invalid>());
  EXPECT_EQ(
      exported.assume<Alias>().get_documentation().line_count(), Count(2));
  EXPECT_TEXT(
      exported.assume<Alias>().get_documentation().get_line(0),
      "public package"_view);
  EXPECT_TEXT(
      exported.assume<Alias>().get_documentation().get_line(1),
      value.get_documentation().get_line(0));
  const Abstract& copy = evaluated.resolve_context("Copy"_view);
  ASSERT(copy.is<Alias>());
  EXPECT(&copy.resolve() == &value);
  const Abstract& addressable = evaluated.resolve_context("item"_view);
  ASSERT(addressable.is<Alias>());
  ASSERT(addressable.resolve().is<Addressable>());
  EXPECT(&addressable.resolve() == &item);
  EXPECT(&addressable.resolve().assume<Addressable>().get_type() == &value);
  const Abstract& nested = evaluated.resolve_context("Nested"_view);
  EXPECT(nested.is<Tetrodotoxin::Model::Namespace>());
  EXPECT_NOT(nested.is<Type>());
  EXPECT_TEXT(nested.get_documentation().get_line(0), "nested exports"_view);
  const Abstract& nested_child = nested.resolve_context("Child"_view);
  ASSERT(nested_child.is<Alias>());
  EXPECT(&nested_child.resolve() == &child);
  EXPECT_TEXT(
      nested_child.assume<Alias>().get_documentation().get_line(0),
      "nested child"_view);
  const Abstract& parent = nested.resolve_context("Parent"_view);
  ASSERT(parent.is<Alias>());
  EXPECT(&parent.resolve() == &value);
  EXPECT(types_errors.is_empty());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, precompiled_package_surface) {
  Allocator::Arena arena;
  Types::Unsigned_8 value;
  Alias exported("Value"_view, value);
  const Static::Vector<Reference<Abstract>, 1> exported_definitions = {
    {exported},
  };
  const Tetrodotoxin::Model::Namespace& exports =
      build_namespace(arena, {}, exported_definitions);
  Tetrodotoxin::Model::Packages::Precompiled package(arena, exports);

  const Abstract& abstract = package;

  EXPECT(abstract.is<Tetrodotoxin::Model::Package>());
  EXPECT(abstract.is<Exports>());
  EXPECT_NOT(abstract.is<Tetrodotoxin::Model::Packages::Interpreted>());
  EXPECT(abstract.get_name().is_empty());
  EXPECT_EQ(package.get_export_count(), Count(1));
  EXPECT(&package.get_export(0) == &exported);
  EXPECT(package.get_export(1).is<Invalid>());
  EXPECT(&abstract.resolve_context("Value"_view).resolve() == &value);
}

PERIMORTEM_UNIT_TEST(DialectTests, local_collision) {
  Allocator::Arena render_arena;
  Types::Unsigned_8 value;
  Alias value_import("Value"_view, value);
  const Static::Vector<Reference<Abstract>, 1> imported = {{value_import}};
  NamespaceDialect types_dialect("Fixture"_view, "Types"_view, imported);
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source types_source({}, "fixture.ttx"_view);
  Ttx::Lexical::Errors types_errors;
  const Abstract& types_root =
      types_source.evaluate(types_dialect, types_errors);
  ASSERT(types_root.is<Tetrodotoxin::Model::Namespace>());
  const Tetrodotoxin::Model::Namespace& types =
      types_root.assume<Tetrodotoxin::Model::Namespace>();
  Tetrodotoxin::Model::Source source(
      "public Value : alias = Types::Value;\n"
      "public Value : alias = Types::Value;"_view,
      "package.ttx"_view);
  EXPECT(add_dependency(source, package, "Types"_view, types_source, types));
  Ttx::Lexical::Errors errors;

  const Abstract& evaluated = evaluate_dialect(package, errors, source);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered,
          "Definition name `Value` already resolves in this context."_view) !=
      Count(-1));
  EXPECT(Algorithm::search(rendered, "Value"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, import_collision) {
  Allocator::Arena render_arena;
  Types::Unsigned_8 value;
  Alias imported_value("Value"_view, value);
  const Static::Vector<Reference<Abstract>, 1> provider_definitions = {
    {imported_value},
  };
  NamespaceDialect provider_dialect(
      "Fixture"_view, View::Bytes{}, provider_definitions);
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source provider({}, "fixture.ttx"_view);
  Ttx::Lexical::Errors provider_errors;
  const Abstract& provider_root =
      provider.evaluate(provider_dialect, provider_errors);
  ASSERT(provider_root.is<Tetrodotoxin::Model::Namespace>());
  const Tetrodotoxin::Model::Namespace& provider_exports =
      provider_root.assume<Tetrodotoxin::Model::Namespace>();
  Tetrodotoxin::Model::Source source(
      "public Value : alias = Value;"_view, "package.ttx"_view);
  EXPECT(add_dependency(
      source, package, "Value"_view, provider, provider_exports));
  Ttx::Lexical::Errors errors;

  const Abstract& evaluated = evaluate_dialect(package, errors, source);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered,
          "Definition name `Value` already resolves in this context."_view) !=
      Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, nested_shadowing) {
  Allocator::Arena render_arena;
  Types::Unsigned_8 value;
  Alias value_import("Value"_view, value);
  const Static::Vector<Reference<Abstract>, 1> imported = {{value_import}};
  NamespaceDialect types_dialect("Fixture"_view, "Types"_view, imported);
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source types_source({}, "fixture.ttx"_view);
  Ttx::Lexical::Errors types_errors;
  const Abstract& types_root =
      types_source.evaluate(types_dialect, types_errors);
  ASSERT(types_root.is<Tetrodotoxin::Model::Namespace>());
  const Tetrodotoxin::Model::Namespace& types =
      types_root.assume<Tetrodotoxin::Model::Namespace>();
  Tetrodotoxin::Model::Source source(
      "public Value : alias = Types::Value;\n"
      "public Nested : group {\n"
      "  public Value : alias = Types::Value;\n"
      "}"_view,
      "package.ttx"_view);
  EXPECT(add_dependency(source, package, "Types"_view, types_source, types));
  Ttx::Lexical::Errors errors;

  const Abstract& evaluated = evaluate_dialect(package, errors, source);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered,
          "Definition name `Value` already resolves in this context."_view) !=
      Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, wrong_modifier) {
  Allocator::Arena render_arena;
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "private Value : alias = Missing;"_view, "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  const Abstract& evaluated = evaluate_dialect(package, errors, source);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered,
          "Definition Dialect `alias` does not accept modifier prefix "
          "`private`."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "private"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, package_rejects_evaluation_modifier) {
  Allocator::Arena render_arena;
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "public const Value : alias = Missing;"_view, "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  const Abstract& evaluated = evaluate_dialect(package, errors, source);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered,
          "Definition Dialect `alias` does not accept modifier prefix "
          "`public const`."_view) != Count(-1));
  EXPECT(source.get_roots().is_empty());
}

PERIMORTEM_UNIT_TEST(DialectTests, wrong_dialect) {
  Allocator::Arena render_arena;
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "public Value : missing;"_view, "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  const Abstract& evaluated = evaluate_dialect(package, errors, source);

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

PERIMORTEM_UNIT_TEST(DialectTests, modifier_order) {
  Allocator::Arena render_arena;
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "public private : alias = Missing;"_view, "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  const Abstract& evaluated = evaluate_dialect(package, errors, source);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered,
          "Definition modifier `private` is duplicated or out of order."_view) !=
      Count(-1));
  EXPECT(Algorithm::search(rendered, "private"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, invalid_target) {
  Allocator::Arena render_arena;
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "public Value : alias = Missing;"_view, "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  const Abstract& evaluated = evaluate_dialect(package, errors, source);

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
  Allocator::Arena render_arena;
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "public Nested : group public"_view, "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  const Abstract& evaluated = evaluate_dialect(package, errors, source);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered, "Expected `{` after Group definition."_view) != Count(-1));
  EXPECT(Algorithm::search(rendered, "public"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, missing_close) {
  Allocator::Arena render_arena;
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "public Nested : group {"_view, "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  const Abstract& evaluated = evaluate_dialect(package, errors, source);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(
          rendered, "Expected `}` after Group definition."_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, missing_assign) {
  Allocator::Arena render_arena;
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(
      "public Value : alias Missing;"_view, "package.ttx"_view);
  Ttx::Lexical::Errors errors;
  const Abstract& evaluated = evaluate_dialect(package, errors, source);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(rendered, "Expected `=` before Alias target."_view) !=
      Count(-1));
  EXPECT(Algorithm::search(rendered, "Missing"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, missing_end) {
  Allocator::Arena render_arena;
  Types::Unsigned_8 value;
  Alias imported_value("Value"_view, value);
  const Static::Vector<Reference<Abstract>, 1> provider_definitions = {
    {imported_value},
  };
  NamespaceDialect provider_dialect(
      "Fixture"_view, View::Bytes{}, provider_definitions);
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source provider({}, "fixture.ttx"_view);
  Ttx::Lexical::Errors provider_errors;
  const Abstract& provider_root =
      provider.evaluate(provider_dialect, provider_errors);
  ASSERT(provider_root.is<Tetrodotoxin::Model::Namespace>());
  const Tetrodotoxin::Model::Namespace& provider_exports =
      provider_root.assume<Tetrodotoxin::Model::Namespace>();
  Tetrodotoxin::Model::Source source(
      "public Copy : alias = Value"_view, "package.ttx"_view);
  EXPECT(add_dependency(
      source, package, "Value"_view, provider, provider_exports));
  Ttx::Lexical::Errors errors;

  const Abstract& evaluated = evaluate_dialect(package, errors, source);

  EXPECT(evaluated.is<Invalid>());
  ASSERT_EQ(errors.get_size(), Count(1));
  View::Bytes rendered = errors.render_message(render_arena, 0);
  EXPECT(
      Algorithm::search(rendered, "Expected `;` after Alias."_view) !=
      Count(-1));
}

PERIMORTEM_UNIT_TEST(DialectTests, anonymous_source) {
  Tetrodotoxin::Interpreter::Dialects::Package package;
  Tetrodotoxin::Model::Source source(View::Bytes{}, View::Bytes{});
  Ttx::Lexical::Errors errors;
  const Abstract& evaluated = evaluate_dialect(package, errors, source);

  EXPECT(evaluated.is<Tetrodotoxin::Model::Package>());
  EXPECT(evaluated.is<Tetrodotoxin::Model::Packages::Interpreted>());
  EXPECT(evaluated.get_name().is_empty());
  EXPECT(errors.is_empty());
}
