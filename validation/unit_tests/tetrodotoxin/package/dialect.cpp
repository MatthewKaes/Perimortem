// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/package/dialect.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/package/language/dependency.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/package/language/source.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/span.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

static auto contains(View::Bytes text, View::Bytes fragment) -> Bool {
  if (fragment.is_empty()) {
    return True;
  }

  if (fragment.get_size() > text.get_size()) {
    return False;
  }

  for (Count start = 0; start + fragment.get_size() <= text.get_size();
       start++) {
    Bool matches = True;
    for (Count i = 0; i < fragment.get_size(); i++) {
      if (text[start + i] != fragment[i]) {
        matches = False;
        break;
      }
    }

    if (matches) {
      return True;
    }
  }

  return False;
}

static auto has_diagnostic(const Errors& errors, View::Bytes fragment) -> Bool {
  Allocator::Arena arena;
  for (Count i = 0; i < errors.get_size(); i++) {
    if (contains(errors.render_message(arena, i), fragment)) {
      return True;
    }
  }

  return False;
}

static auto rejects_package(
    View::Bytes path,
    View::Bytes source,
    View::Bytes diagnostic) -> Bool {
  Environment::Workspace workspace;
  Errors errors;
  const Bool installed =
      workspace.install_dialect<Package::Dialect>("Package"_view);
  const auto imported =
      workspace.import_source("Rejected"_view, path, source, errors);
  const Bool unpublished =
      &workspace.resolve_context("Rejected"_view) == &Invalid::get_invalid();

  return installed && !imported && unpublished &&
         has_diagnostic(errors, diagnostic);
}

static auto rejects_package_file(View::Bytes path, View::Bytes diagnostic)
    -> Bool {
  auto source = File::read(path);
  if (!source) {
    return False;
  }

  return rejects_package(path, *source, diagnostic);
}

static Harness PackageDialect = {
  .name = "Tetrodotoxin::Package::Dialect"_view,
};

PERIMORTEM_UNIT_TEST(PackageDialect, dependency_statement) {
  static constexpr View::Bytes source =
      "resolve Runtime::Math : Perimortem.Graphics.Math = \"12.34\";\n"
      "source Main from \"main.ttx\";"_view;
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, source, "dependency.ttx"_view);
  Cursor cursor(tokenizer, errors);

  Span span;
  auto parsed = Package::Language::Dependency::parse(cursor, span);

  ASSERT(parsed);
  EXPECT_TEXT((*parsed).get_local_name(), "Runtime::Math"_view);
  EXPECT_TEXT((*parsed).get_package_name(), "Perimortem.Graphics.Math"_view);
  EXPECT((*parsed).get_version() == Version(12, 34));
  EXPECT(span.get_start().get_code() == Code::Type::Resolve);
  EXPECT_EQ(span.get_start().get_offset(), Unsigned_16(0));
  EXPECT_TEXT(span.get_start().caculate_text(source), "resolve"_view);
  EXPECT(span.get_end().get_code() == Code::Type::EndStatement);
  EXPECT_EQ(span.get_end().get_offset(), Unsigned_16(58));
  EXPECT_TEXT(span.get_end().caculate_text(source), ";"_view);
  EXPECT_TEXT(
      span.caculate_text(source),
      "resolve Runtime::Math : Perimortem.Graphics.Math = \"12.34\";"_view);
  EXPECT(cursor.matches(Code::Type::Source));
  EXPECT(errors.is_empty());

  static constexpr View::Bytes invalid_source =
      "resolve Runtime : Example.Runtime = \"1.0\""_view;
  Allocator::Arena invalid_arena;
  Errors invalid_errors;
  Tokenizer invalid_tokenizer(
      invalid_arena, invalid_source, "invalid-dependency.ttx"_view);
  Cursor invalid_cursor(invalid_tokenizer, invalid_errors);
  Span invalid_span(Token(0, 1, 1, 7, Code::Type::Resolve));

  auto rejected =
      Package::Language::Dependency::parse(invalid_cursor, invalid_span);
  EXPECT_NOT(rejected);
  EXPECT_NOT(invalid_span);
  EXPECT_NOT(invalid_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(PackageDialect, source_statement) {
  static constexpr View::Bytes source =
      "source Scenes::Splash from \"scenes/./splash.ttx\";\n"
      "resolve Runtime : Example.Runtime = \"1.0\";"_view;
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, source, "source.ttx"_view);
  Cursor cursor(tokenizer, errors);

  auto parsed = Package::Language::Source::parse(arena, cursor);

  ASSERT(parsed);
  EXPECT_TEXT((*parsed).get_local_name(), "Scenes::Splash"_view);
  EXPECT_TEXT((*parsed).get_source_path(), "scenes/splash.ttx"_view);
  EXPECT(cursor.matches(Code::Type::Resolve));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(PackageDialect, ordered_monograph) {
  Dynamic::Bytes source(
      "// Synthetic Package\n"
      "dialect : Package;\n"
      "\n"
      "resolve Runtime::Math : Perimortem.Graphics.Math = \"12.34\";\n"
      "resolve Assets : Example.Assets = \"2.7\";\n"
      "source Scenes::Splash from \"scenes/./splash.ttx\";\n"
      "source Main from \"main.ttx\";\n"_view);
  Environment::Workspace workspace;
  Errors errors;

  ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
  ASSERT(workspace.import_source(
      "Synthetic"_view, "synthetic/package.ttx"_view, source, errors));
  source.set('x');

  const Abstract& imported = workspace.resolve_context("Synthetic"_view);
  ASSERT(imported.is<Package::Language::Monograph>());
  const auto& monograph =
      static_cast<const Package::Language::Monograph&>(imported);
  View::Vector<Package::Language::Dependency> dependencies =
      monograph.get_dependencies();
  View::Vector<Span> dependency_spans = monograph.get_dependency_spans();
  View::Vector<Package::Language::Source> sources = monograph.get_sources();

  ASSERT_EQ(dependencies.get_size(), 2);
  ASSERT_EQ(dependency_spans.get_size(), dependencies.get_size());
  EXPECT_TEXT(dependencies[0].get_local_name(), "Runtime::Math"_view);
  EXPECT_TEXT(
      dependencies[0].get_package_name(), "Perimortem.Graphics.Math"_view);
  EXPECT(dependencies[0].get_version() == Version(12, 34));
  EXPECT_TEXT(dependencies[1].get_local_name(), "Assets"_view);
  EXPECT_TEXT(dependencies[1].get_package_name(), "Example.Assets"_view);
  EXPECT(dependencies[1].get_version() == Version(2, 7));
  EXPECT_EQ(dependency_spans[0].get_start().get_line(), Unsigned_16(4));
  EXPECT_EQ(dependency_spans[1].get_start().get_line(), Unsigned_16(5));
  EXPECT(dependency_spans[0].get_start().get_code() == Code::Type::Resolve);
  EXPECT(dependency_spans[0].get_end().get_code() == Code::Type::EndStatement);
  EXPECT(dependency_spans[1].get_start().get_code() == Code::Type::Resolve);
  EXPECT(dependency_spans[1].get_end().get_code() == Code::Type::EndStatement);

  ASSERT_EQ(sources.get_size(), 2);
  EXPECT_TEXT(sources[0].get_local_name(), "Scenes::Splash"_view);
  EXPECT_TEXT(sources[0].get_source_path(), "scenes/splash.ttx"_view);
  EXPECT_TEXT(sources[1].get_local_name(), "Main"_view);
  EXPECT_TEXT(sources[1].get_source_path(), "main.ttx"_view);

  ASSERT_EQ(monograph.get_documentation().line_count(), 1);
  EXPECT_TEXT(
      monograph.get_documentation().get_line(0), "Synthetic Package"_view);
  EXPECT(monograph.is<Package::Language::Monograph>());
  EXPECT(monograph.is<Abstract>());
  EXPECT(&monograph.resolve_context("Missing"_view) == &Invalid::get_invalid());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(PackageDialect, canonical_inventory) {
  static constexpr View::Bytes path =
      "apps/ttx/scene_lifetime/package.ttx"_view;
  auto source = File::read(path);
  ASSERT(source);

  Environment::Workspace workspace;
  Errors errors;
  ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
  ASSERT(workspace.import_source("Root"_view, path, *source, errors));

  const Abstract& imported = workspace.resolve_context("Root"_view);
  ASSERT(imported.is<Package::Language::Monograph>());
  const auto& monograph =
      static_cast<const Package::Language::Monograph&>(imported);
  View::Vector<Package::Language::Dependency> dependencies =
      monograph.get_dependencies();
  View::Vector<Package::Language::Source> sources = monograph.get_sources();

  ASSERT_EQ(dependencies.get_size(), 3);
  EXPECT_TEXT(dependencies[0].get_local_name(), "Math"_view);
  EXPECT_TEXT(dependencies[0].get_package_name(), "Perimortem.Math"_view);
  EXPECT(dependencies[0].get_version() == Version(1, 0));
  EXPECT_TEXT(dependencies[1].get_local_name(), "Graphics"_view);
  EXPECT_TEXT(dependencies[1].get_package_name(), "Perimortem.Graphics"_view);
  EXPECT(dependencies[1].get_version() == Version(1, 0));
  EXPECT_TEXT(dependencies[2].get_local_name(), "System"_view);
  EXPECT_TEXT(dependencies[2].get_package_name(), "Perimortem.System"_view);
  EXPECT(dependencies[2].get_version() == Version(1, 0));

  ASSERT_EQ(sources.get_size(), 3);
  EXPECT_TEXT(sources[0].get_local_name(), "Scenes::Splash"_view);
  EXPECT_TEXT(sources[0].get_source_path(), "scenes/splash.ttx"_view);
  EXPECT_TEXT(sources[1].get_local_name(), "Scenes::Title"_view);
  EXPECT_TEXT(sources[1].get_source_path(), "scenes/title.ttx"_view);
  EXPECT_TEXT(sources[2].get_local_name(), "Main"_view);
  EXPECT_TEXT(sources[2].get_source_path(), "main.ttx"_view);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(PackageDialect, prior_diagnostics) {
  Environment::Workspace workspace;
  Errors errors;
  {
    Errors::Report report(
        errors, "prior-package.ttx"_view, View::Bytes(), Span());
    report << "Earlier independent diagnostic."_view;
  }

  ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
  ASSERT(workspace.import_source(
      "Minimal"_view, "minimal.ttx"_view,
      "// Minimal\ndialect : Package;\nsource Main from \"./main.ttx\";\n"_view,
      errors));

  const Abstract& imported = workspace.resolve_context("Minimal"_view);
  ASSERT(imported.is<Package::Language::Monograph>());
  const auto& monograph =
      static_cast<const Package::Language::Monograph&>(imported);
  EXPECT(monograph.get_dependencies().is_empty());
  EXPECT(monograph.get_dependency_spans().is_empty());
  ASSERT_EQ(monograph.get_sources().get_size(), 1);
  EXPECT_TEXT(monograph.get_sources()[0].get_local_name(), "Main"_view);
  EXPECT_TEXT(monograph.get_sources()[0].get_source_path(), "main.ttx"_view);
  EXPECT_EQ(errors.get_size(), 1);
}

PERIMORTEM_UNIT_TEST(PackageDialect, construction_provenance) {
  Package::Language::Dependency dependencies[] = {
    Package::Language::Dependency(
        "Core"_view, "Perimortem.Core"_view, Version(1, 0)),
    Package::Language::Dependency(
        "Memory"_view, "Perimortem.Memory"_view, Version(1, 0)),
  };
  Span partial_spans[] = {
    Span(
        Token(0, 1, 1, 7, Code::Type::Resolve),
        Token(40, 1, 41, 1, Code::Type::EndStatement)),
  };
  Package::Language::Source sources[] = {
    Package::Language::Source("Main"_view, "main.ttx"_view),
  };
  Environment::Workspace registry;
  Package::Dialect host(registry);
  Allocator::Arena arena;

  // The authored factory owns the only invalid inventory shape. The separate
  // source free operation has no span input that a caller could misclassify.
  auto partial = Package::Language::Monograph::create_authored(
      arena, Documentation::get_empty(), host, dependencies, partial_spans,
      sources);
  EXPECT_NOT(partial);

  auto& source_free = Package::Language::Monograph::create_source_free(
      arena, Documentation::get_empty(), host, dependencies);
  ASSERT_EQ(source_free.get_dependencies().get_size(), Count(2));
  EXPECT(source_free.get_dependency_spans().is_empty());
  EXPECT(source_free.get_sources().is_empty());
}

PERIMORTEM_UNIT_TEST(PackageDialect, frozen_negative_fixtures) {
  EXPECT(rejects_package_file(
      "validation/data/ttx/package/float_version.ttx"_view,
      "Package dependency versions must be quoted."_view));
  EXPECT(rejects_package_file(
      "validation/data/ttx/package/noncanonical_version.ttx"_view,
      "Package dependency version is not canonical `Major.Minor` text."_view));
  EXPECT(rejects_package_file(
      "validation/data/ttx/package/duplicate_semantic_name.ttx"_view,
      "Duplicate Source semantic name in this Package."_view));
  EXPECT(rejects_package_file(
      "validation/data/ttx/package/duplicate_normalized_path.ttx"_view,
      "Duplicate normalized Source path in this Package."_view));
}

PERIMORTEM_UNIT_TEST(PackageDialect, duplicate_dimensions) {
  static constexpr View::Bytes shared_source =
      "// Duplicate dimensions\n"
      "dialect : Package;\n"
      "source First from \"./member.ttx\";\n"
      "source First from \"member.ttx\";\n"_view;
  Environment::Workspace workspace;
  Errors errors;

  ASSERT(workspace.install_dialect<Package::Dialect>("Package"_view));
  EXPECT_NOT(workspace.import_source(
      "Duplicate"_view, "duplicate.ttx"_view, shared_source, errors));
  EXPECT(
      &workspace.resolve_context("Duplicate"_view) == &Invalid::get_invalid());
  EXPECT(has_diagnostic(
      errors, "Duplicate Source semantic name in this Package."_view));
  EXPECT(has_diagnostic(
      errors, "Duplicate normalized Source path in this Package."_view));
  EXPECT_NOT(has_diagnostic(
      errors, "Package requires at least one complete Source statement."_view));

  EXPECT(rejects_package(
      "dependency.ttx"_view,
      "// Duplicate alias\n"
      "dialect : Package;\n"
      "resolve Runtime : First.Runtime = \"1.0\";\n"
      "resolve Runtime : Second.Runtime = \"2.0\";\n"
      "source Main from \"main.ttx\";\n"_view,
      "Duplicate Dependency local alias in this Package."_view));
}

PERIMORTEM_UNIT_TEST(PackageDialect, body_constraints) {
  EXPECT(rejects_package(
      "dependencies_only.ttx"_view,
      "// Dependencies only\n"
      "dialect : Package;\n"
      "resolve Runtime : Example.Runtime = \"1.0\";\n"_view,
      "Package requires at least one complete Source statement."_view));

  EXPECT(rejects_package(
      "late_resolve.ttx"_view,
      "// Late Resolve\n"
      "dialect : Package;\n"
      "source Main from \"main.ttx\";\n"
      "resolve Runtime : Example.Runtime = \"1.0\";\n"_view,
      "Resolve statements must precede every Source statement."_view));
}

PERIMORTEM_UNIT_TEST(PackageDialect, version_failures) {
  EXPECT(rejects_package(
      "missing_version.ttx"_view,
      "// Missing version\n"
      "dialect : Package;\n"
      "resolve Runtime : Example.Runtime = ;\n"
      "source Main from \"main.ttx\";\n"_view,
      "Package dependency versions must be quoted."_view));

  EXPECT(rejects_package(
      "repeated_assignment.ttx"_view,
      "// Repeated assignment\n"
      "dialect : Package;\n"
      "resolve Runtime : Example.Runtime = = \"1.0\";\n"
      "source Main from \"main.ttx\";\n"_view,
      "Package dependency versions must be quoted."_view));

  EXPECT(rejects_package(
      "unterminated_version.ttx"_view,
      "// Unterminated version\n"
      "dialect : Package;\n"
      "resolve Runtime : Example.Runtime = \"1.0"_view,
      "Package dependency version String is unterminated."_view));

  EXPECT(rejects_package(
      "empty_version.ttx"_view,
      "// Empty version\n"
      "dialect : Package;\n"
      "resolve Runtime : Example.Runtime = \"\";\n"
      "source Main from \"main.ttx\";\n"_view,
      "Package dependency version is not canonical `Major.Minor` text."_view));

  EXPECT(rejects_package(
      "null_version.ttx"_view,
      "// Null version\n"
      "dialect : Package;\n"
      "resolve Runtime : Example.Runtime = \"0.0\";\n"
      "source Main from \"main.ttx\";\n"_view,
      "Package dependency version is not canonical `Major.Minor` text."_view));

  EXPECT(rejects_package(
      "overflow_version.ttx"_view,
      "// Overflow version\n"
      "dialect : Package;\n"
      "resolve Runtime : Example.Runtime = \"65536.0\";\n"
      "source Main from \"main.ttx\";\n"_view,
      "Package dependency version is not canonical `Major.Minor` text."_view));
}

PERIMORTEM_UNIT_TEST(PackageDialect, contiguous_qualification) {
  EXPECT(rejects_package(
      "local_spacing.ttx"_view,
      "// Local spacing\n"
      "dialect : Package;\n"
      "source Scenes :: Splash from \"splash.ttx\";\n"_view,
      "Semantic names cannot contain whitespace around `::`."_view));

  EXPECT(rejects_package(
      "external_spacing.ttx"_view,
      "// External spacing\n"
      "dialect : Package;\n"
      "resolve Math : Perimortem . Math = \"1.0\";\n"
      "source Main from \"main.ttx\";\n"_view,
      "External Package names cannot contain whitespace around `.`."_view));

  EXPECT(rejects_package(
      "malformed_qualification.ttx"_view,
      "// Malformed qualification\n"
      "dialect : Package;\n"
      "source Scenes:: from \"splash.ttx\";\n"_view,
      "Semantic name qualification requires a Type segment after `::`."_view));
}

PERIMORTEM_UNIT_TEST(PackageDialect, source_statement_shape) {
  EXPECT(rejects_package(
      "anonymous.ttx"_view,
      "// Anonymous Source\n"
      "dialect : Package;\n"
      "source \"main.ttx\";\n"_view,
      "Expected an authored Type shaped semantic name."_view));

  EXPECT(rejects_package(
      "missing_from.ttx"_view,
      "// Missing from\n"
      "dialect : Package;\n"
      "source Main \"main.ttx\";\n"_view,
      "Source statements require exact `from` spelling."_view));

  EXPECT(rejects_package(
      "wrong_relation.ttx"_view,
      "// Wrong relation\n"
      "dialect : Package;\n"
      "source Main via \"main.ttx\";\n"_view,
      "Source statements require exact `from` spelling."_view));
}

PERIMORTEM_UNIT_TEST(PackageDialect, quoted_payload_failures) {
  EXPECT(rejects_package(
      "missing_path.ttx"_view,
      "// Missing path\n"
      "dialect : Package;\n"
      "source Main from;\n"_view,
      "Source paths must be closed quoted String values."_view));

  EXPECT(rejects_package(
      "unquoted_path.ttx"_view,
      "// Unquoted path\n"
      "dialect : Package;\n"
      "source Main from main;\n"_view,
      "Source paths must be closed quoted String values."_view));

  EXPECT(rejects_package(
      "unterminated_path.ttx"_view,
      "// Unterminated path\n"
      "dialect : Package;\n"
      "source Main from \"main.ttx"_view,
      "Source path String is unterminated."_view));
}

PERIMORTEM_UNIT_TEST(PackageDialect, statement_terminator) {
  EXPECT(rejects_package(
      "missing_terminator.ttx"_view,
      "// Missing terminator\n"
      "dialect : Package;\n"
      "source Main from \"main.ttx\""_view,
      "Source statements require a terminating `;`."_view));

  EXPECT(rejects_package(
      "resolve_terminator.ttx"_view,
      "// Missing Resolve terminator\n"
      "dialect : Package;\n"
      "resolve Runtime : Example.Runtime = \"1.0\"\n"
      "source Main from \"main.ttx\";"_view,
      "Resolve statements require a terminating `;`."_view));
}
