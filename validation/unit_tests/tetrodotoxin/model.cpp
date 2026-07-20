// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/model/dependencies/package.hpp"
#include "tetrodotoxin/model/dependencies/source.hpp"
#include "tetrodotoxin/model/dialect.hpp"
#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/packages/precompiled.hpp"
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
  SourceDialect dialect("Package"_view, "PublicValue"_view);
  Tetrodotoxin::Model::Source source(
      "public PublicValue : alias = Value;"_view, "validation/source.ttx"_view);
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
  SourceDialect dialect("Package"_view, "Value"_view);
  Tetrodotoxin::Model::Source source(View::Bytes{}, View::Bytes{});
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
  SourceDialect first_dialect("Package"_view, View::Bytes{});
  SourceDialect second_dialect("Library"_view, View::Bytes{});
  Tetrodotoxin::Model::Source source(View::Bytes{}, View::Bytes{});
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

PERIMORTEM_UNIT_TEST(SourceTests, dependency_contracts) {
  SourceLeaf target("Value"_view);
  const Static::Vector<Reference<Abstract>, 1> exported = {{target}};
  SourceNamespaceDialect source_dialect("Library"_view, "Types"_view, exported);
  SourceDialect package_dialect("Package"_view, View::Bytes{});
  Tetrodotoxin::Model::Source source({}, "root.ttx"_view);
  Tetrodotoxin::Model::Source provider({}, "types.ttx"_view);
  Ttx::Lexical::Errors provider_errors;
  const Abstract& provider_root =
      provider.evaluate(source_dialect, provider_errors);
  ASSERT(provider_root.is<Tetrodotoxin::Model::Namespace>());
  const Tetrodotoxin::Model::Namespace& provider_exports =
      provider_root.assume<Tetrodotoxin::Model::Namespace>();
  const Static::Vector<View::Bytes, 1> lines = {{"local import"_view}};
  Block documentation(lines);
  const auto& source_dependency =
      source.get_arena().construct<Tetrodotoxin::Model::Dependencies::Source>(
          source_dialect, "types.ttx"_view, provider, "Types"_view,
          provider_exports, documentation);
  const Alias& source_binding = source_dependency.get_binding();

  Bool depended_on_source = source.depend(source_dependency);

  EXPECT(depended_on_source);
  EXPECT(source_dependency.is<Tetrodotoxin::Model::Dependency>());
  EXPECT(source_dependency.is<Tetrodotoxin::Model::Dependencies::Source>());
  EXPECT_NOT(
      source_dependency.is<Tetrodotoxin::Model::Dependencies::Package>());
  EXPECT(&source_dependency.get_root_dialect() == &source_dialect);
  EXPECT_TEXT(source_dependency.get_path(), "types.ttx"_view);
  EXPECT(&source_dependency.get_source() == &provider);
  EXPECT(source_dependency.get_documentation().is_empty());
  EXPECT(&source.resolve_context("Types"_view) == &source_binding);
  EXPECT_TEXT(
      source.resolve_context("Types"_view).get_documentation().get_line(0),
      "local import"_view);
  EXPECT(&source.resolve_context("Types"_view).resolve() == &provider_exports);
  EXPECT(source.resolve_context("Types"_view).resolve().is<Exports>());
  EXPECT(
      &source.resolve_context("Types"_view).resolve_context("Value"_view) ==
      &target);

  Allocator::Arena package_arena;
  const Tetrodotoxin::Model::Namespace& exports =
      Tetrodotoxin::Model::Namespace::construct(
          package_arena, {}, View::Vector<Reference<Abstract>>())
          .assume<Tetrodotoxin::Model::Namespace>();
  Tetrodotoxin::Model::Packages::Precompiled package(package_arena, exports);
  const auto& package_dependency =
      source.get_arena().construct<Tetrodotoxin::Model::Dependencies::Package>(
          package_dialect, "Perimortem.Math"_view, "Math"_view, package,
          package.get_documentation());
  const Alias& package_binding = package_dependency.get_binding();

  Bool depended_on_package = source.depend(package_dependency);

  EXPECT(depended_on_package);
  EXPECT(package_dependency.is<Tetrodotoxin::Model::Dependency>());
  EXPECT(package_dependency.is<Tetrodotoxin::Model::Dependencies::Package>());
  EXPECT_NOT(
      package_dependency.is<Tetrodotoxin::Model::Dependencies::Source>());
  EXPECT(&package_dependency.get_root_dialect() == &package_dialect);
  EXPECT_TEXT(package_dependency.get_package_name(), "Perimortem.Math"_view);
  EXPECT(&package_dependency.get_package() == &package);
  EXPECT(&source.resolve_context("Math"_view) == &package_binding);
  EXPECT(source.resolve_context("Math"_view).is<Alias>());
  EXPECT(source.resolve_context("Math"_view)
             .resolve()
             .is<Tetrodotoxin::Model::Package>());
  EXPECT_EQ(source.get_dependencies().get_size(), Count(2));
}

PERIMORTEM_UNIT_TEST(SourceTests, owns_input_and_tokenizer) {
  Dynamic::Bytes input("public Value : alias = Target;"_view);
  Dynamic::Bytes path("snippet.ttx"_view);
  Tetrodotoxin::Model::Source source(input, path);

  input.clear();
  path.clear();

  EXPECT_TEXT(source.get_text(), "public Value : alias = Target;"_view);
  EXPECT_TEXT(source.get_path(), "snippet.ttx"_view);
  EXPECT_TEXT(source.get_tokenizer().get_source_text(), source.get_text());
  EXPECT_TEXT(source.get_tokenizer().get_source_path(), source.get_path());
  EXPECT_NOT(source.get_tokenizer().is_empty());
}
