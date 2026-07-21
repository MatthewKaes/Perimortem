// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/model/dependencies/package.hpp"
#include "tetrodotoxin/model/dialect.hpp"
#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/packages/precompiled.hpp"
#include "tetrodotoxin/model/packages/sources.hpp"
#include "tetrodotoxin/model/source.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/documentations/block.hpp"
#include "ttx/model/exports.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Ttx::Model::Documentations;
using namespace Validation;

/// A named leaf keeps Source tests focused on general Abstract resolution.
class SourceLeaf final : public Abstract {
 public:
  explicit SourceLeaf(View::Bytes name) : name(name) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

 private:
  View::Bytes name;
};

/// A Dialect fixture proves Source and Dependency retain the real contract.
class SourceDialect final : public Tetrodotoxin::Model::Dialect {
 public:
  SourceDialect(View::Bytes name, View::Bytes result_name)
      : name(name), result_name(result_name) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto evaluate(Ttx::Lexical::Cursor& cursor, const Abstract&) const
      -> const Abstract& override {
    return cursor.get_arena().construct<SourceLeaf>(result_name);
  }

 private:
  View::Bytes name;
  View::Bytes result_name;
};

// A Source-owned Namespace fixture builds its root through the same
// arena-backed transaction used by production Dialects.
class SourceNamespaceDialect final : public Tetrodotoxin::Model::Dialect {
 public:
  SourceNamespaceDialect(
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

static Harness SourceTests = {
  .name = "Source"_view,
};

PERIMORTEM_UNIT_TEST(SourceTests, source_root) {
  Tetrodotoxin::Model::Environment empty_environment;
  SourceDialect dialect("Package"_view, "PublicValue"_view);
  Tetrodotoxin::Model::Source source(
      empty_environment, "public PublicValue : alias = Value;"_view,
      "validation/source.ttx"_view);
  Ttx::Lexical::Errors errors;

  const Abstract& evaluated = source.evaluate(dialect, errors);

  EXPECT_NOT(evaluated.is<Invalid>());
  ASSERT(source.is<Tetrodotoxin::Model::Source>());
  EXPECT_NOT(source.is<Ttx::Model::Type>());
  EXPECT(source.get_name().is_empty());
  EXPECT_TEXT(source.get_path(), "validation/source.ttx"_view);
  EXPECT_TEXT(source.get_text(), "public PublicValue : alias = Value;"_view);
  EXPECT_TEXT(source.get_tokenizer().get_source_path(), source.get_path());
  ASSERT_EQ(source.get_roots().get_size(), Count(1));
  EXPECT(&source.get_roots()[0].get_definition() == &evaluated);
  EXPECT(&source.get_roots()[0].get_dialect() == &dialect);
  EXPECT(source.get_documentation().is_empty());
  EXPECT(&source.resolve_context("PublicValue"_view) == &evaluated);
  EXPECT(&source.resolve_context("Missing"_view) == &Invalid::get_invalid());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(SourceTests, duplicate_root) {
  Tetrodotoxin::Model::Environment empty_environment;
  SourceDialect dialect("Package"_view, "Value"_view);
  Tetrodotoxin::Model::Source source(
      empty_environment, View::Bytes{}, View::Bytes{});
  Ttx::Lexical::Errors errors;

  const Abstract& first = source.evaluate(dialect, errors);
  const Abstract& second = source.evaluate(dialect, errors);

  EXPECT_NOT(first.is<Invalid>());
  EXPECT(second.is<Invalid>());
  EXPECT(source.get_path().is_empty());
  EXPECT(source.get_text().is_empty());
  EXPECT(&source.resolve_context("Value"_view) == &first);
  EXPECT_EQ(errors.get_size(), Count(1));
}

PERIMORTEM_UNIT_TEST(SourceTests, anonymous_roots) {
  Tetrodotoxin::Model::Environment empty_environment;
  SourceDialect first_dialect("Package"_view, View::Bytes{});
  SourceDialect second_dialect("Library"_view, View::Bytes{});
  Tetrodotoxin::Model::Source source(
      empty_environment, View::Bytes{}, View::Bytes{});
  Ttx::Lexical::Errors errors;

  const Abstract& first = source.evaluate(first_dialect, errors);
  const Abstract& second = source.evaluate(second_dialect, errors);

  EXPECT_NOT(first.is<Invalid>());
  EXPECT_NOT(second.is<Invalid>());
  ASSERT_EQ(source.get_roots().get_size(), Count(2));
  EXPECT(&source.get_roots()[0].get_definition() == &first);
  EXPECT(&source.get_roots()[0].get_dialect() == &first_dialect);
  EXPECT(&source.get_roots()[1].get_definition() == &second);
  EXPECT(&source.get_roots()[1].get_dialect() == &second_dialect);
  EXPECT(source.resolve_context(View::Bytes{}).is<Invalid>());
}

PERIMORTEM_UNIT_TEST(SourceTests, environment_bindings) {
  Tetrodotoxin::Model::Environment environment;
  SourceDialect package_dialect("Package"_view, View::Bytes{});
  Allocator::Arena package_arena;
  const Tetrodotoxin::Model::Namespace& exports =
      Tetrodotoxin::Model::Namespace::construct(
          package_arena, {}, View::Vector<Reference<Abstract>>())
          .assume<Tetrodotoxin::Model::Namespace>();
  Tetrodotoxin::Model::Packages::Precompiled package(package_arena, exports);
  ASSERT(environment.resolve(
      package_dialect, "Math"_view, "Perimortem.Math"_view,
      Perimortem::System::Version(1, 2), package, package.get_documentation()));

  SourceLeaf target("Value"_view);
  const Static::Vector<Reference<Abstract>, 1> exported = {{target}};
  SourceNamespaceDialect source_dialect("Library"_view, "Types"_view, exported);
  Tetrodotoxin::Model::Source source(environment, {}, "root.ttx"_view);
  Tetrodotoxin::Model::Source provider(environment, {}, "types.ttx"_view);
  Ttx::Lexical::Errors provider_errors;
  const Abstract& provider_root =
      provider.evaluate(source_dialect, provider_errors);
  ASSERT(provider_root.is<Tetrodotoxin::Model::Namespace>());
  const Tetrodotoxin::Model::Namespace& provider_exports =
      provider_root.assume<Tetrodotoxin::Model::Namespace>();
  const Static::Vector<View::Bytes, 1> lines = {{"local binding"_view}};
  Block documentation(lines);
  EXPECT(environment.bind("Types"_view, provider_exports, documentation));
  EXPECT_NOT(environment.bind("Types"_view, provider_exports, documentation));
  EXPECT(source.resolve_context("Types"_view).is<Alias>());
  EXPECT_TEXT(
      source.resolve_context("Types"_view).get_documentation().get_line(0),
      "local binding"_view);
  EXPECT(&source.resolve_context("Types"_view).resolve() == &provider_exports);
  EXPECT(source.resolve_context("Types"_view).resolve().is<Exports>());
  EXPECT(
      &source.resolve_context("Types"_view).resolve_context("Value"_view) ==
      &target);

  ASSERT_EQ(environment.get_resolutions().get_size(), Count(1));
  const auto& package_dependency = environment.get_resolutions()[0].get();
  const Alias& package_binding = package_dependency.get_binding();

  EXPECT(package_dependency.is<Tetrodotoxin::Model::Dependency>());
  EXPECT(package_dependency.is<Tetrodotoxin::Model::Dependencies::Package>());
  EXPECT(&package_dependency.get_root_dialect() == &package_dialect);
  EXPECT_TEXT(package_dependency.get_package_name(), "Perimortem.Math"_view);
  EXPECT(package_dependency.get_version() == Perimortem::System::Version(1, 2));
  EXPECT(&package_dependency.get_package() == &package);
  EXPECT(&source.resolve_context("Math"_view) == &package_binding);
  EXPECT(&provider.resolve_context("Math"_view) == &package_binding);
  EXPECT(source.resolve_context("Math"_view).is<Alias>());
  EXPECT(source.resolve_context("Math"_view)
             .resolve()
             .is<Tetrodotoxin::Model::Package>());
  ASSERT_EQ(environment.get_packages().get_size(), Count(1));
  EXPECT(&environment.get_packages()[0].get() == &package);

  EXPECT(environment.resolve(
      package_dialect, "Geometry"_view, "Perimortem.Math"_view,
      Perimortem::System::Version(1, 2), package, package.get_documentation()));
  EXPECT_EQ(environment.get_resolutions().get_size(), Count(2));
  EXPECT_EQ(environment.get_dependencies().get_size(), Count(1));
  EXPECT_EQ(environment.get_packages().get_size(), Count(1));
  EXPECT_NOT(environment.resolve(
      package_dialect, "Math"_view, "Perimortem.Math"_view,
      Perimortem::System::Version(1, 2), package, package.get_documentation()));
}

PERIMORTEM_UNIT_TEST(SourceTests, package_definitions_include_private_roots) {
  Tetrodotoxin::Model::Environment environment;
  Tetrodotoxin::Model::Source source(environment, {}, "package.ttx"_view);
  SourceLeaf private_definition("PrivateValue"_view);
  SourceLeaf public_definition("PublicValue"_view);
  Tetrodotoxin::Model::Namespace namespace_object(
      source.get_arena(), View::Bytes{});
  ASSERT(namespace_object.add_root(private_definition));
  ASSERT(namespace_object.add_export(public_definition));

  const Static::Vector<Reference<Tetrodotoxin::Model::Source>, 1> members = {
    {source},
  };
  const Abstract& constructed =
      Tetrodotoxin::Model::Packages::Sources::construct(
          source.get_arena(), members, namespace_object);
  ASSERT(constructed.is<Tetrodotoxin::Model::Packages::Interpreted>());
  const auto& package = constructed.assume<Tetrodotoxin::Model::Package>();
  EXPECT(package.resolve_context("PrivateValue"_view).is<Invalid>());
  Count private_id = package.get_definition_id(private_definition);
  ASSERT(private_id != Count(-1));
  EXPECT(&package.get_definition(private_id) == &private_definition);
}

PERIMORTEM_UNIT_TEST(SourceTests, explicit_package_membership) {
  Tetrodotoxin::Model::Environment environment;
  Tetrodotoxin::Model::Environment other_environment;
  Tetrodotoxin::Model::Source first(environment, {}, "first.ttx"_view);
  Tetrodotoxin::Model::Source second(environment, {}, "second.ttx"_view);
  Tetrodotoxin::Model::Source foreign(
      other_environment, {}, "foreign.ttx"_view);
  Tetrodotoxin::Model::Namespace exports(first.get_arena(), {});
  Allocator::Arena package_arena;

  const Static::Vector<Reference<Tetrodotoxin::Model::Source>, 2> members = {{
    first,
    second,
  }};
  const Abstract& constructed =
      Tetrodotoxin::Model::Packages::Sources::construct(
          package_arena, members, exports);
  ASSERT(constructed.is<Tetrodotoxin::Model::Packages::Interpreted>());
  const auto sources =
      constructed.assume<Tetrodotoxin::Model::Packages::Interpreted>()
          .get_sources();
  ASSERT_EQ(sources.get_size(), Count(2));
  EXPECT(&sources[0].get() == &first);
  EXPECT(&sources[1].get() == &second);

  const Static::Vector<Reference<Tetrodotoxin::Model::Source>, 2> duplicate = {{
    first,
    first,
  }};
  EXPECT(
      Tetrodotoxin::Model::Packages::Sources::construct(
          package_arena, duplicate, exports)
          .is<Invalid>());

  const Static::Vector<Reference<Tetrodotoxin::Model::Source>, 2> mixed = {{
    first,
    foreign,
  }};
  EXPECT(
      Tetrodotoxin::Model::Packages::Sources::construct(
          package_arena, mixed, exports)
          .is<Invalid>());
  EXPECT(
      Tetrodotoxin::Model::Packages::Sources::construct(
          package_arena, {}, exports)
          .is<Invalid>());
}

PERIMORTEM_UNIT_TEST(SourceTests, owns_input_and_tokenizer) {
  Tetrodotoxin::Model::Environment empty_environment;
  Dynamic::Bytes input("public Value : alias = Target;"_view);
  Dynamic::Bytes path("snippet.ttx"_view);
  Tetrodotoxin::Model::Source source(empty_environment, input, path);

  input.clear();
  path.clear();

  EXPECT_TEXT(source.get_text(), "public Value : alias = Target;"_view);
  EXPECT_TEXT(source.get_path(), "snippet.ttx"_view);
  EXPECT_TEXT(source.get_tokenizer().get_source_text(), source.get_text());
  EXPECT_TEXT(source.get_tokenizer().get_source_path(), source.get_path());
  EXPECT_NOT(source.get_tokenizer().is_empty());
}
