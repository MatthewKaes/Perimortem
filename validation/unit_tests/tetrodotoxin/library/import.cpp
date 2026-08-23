// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/import.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;
using namespace Validation;

class ImportContext : public Abstract {
 public:
  TTX_CONTRACT(ImportContext, Abstract);
  TTX_NAME("ImportContext"_view);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

static auto interpret_library(
    Allocator::Arena& arena,
    Library::Dialect& dialect,
    Abstract& context,
    View::Bytes source,
    Errors& errors) -> Option<Library::Language::Monograph&> {
  Tokenizer tokenizer(arena, source, "library-import.ttx"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  auto monograph = dialect.interpret(
      cursor, Documentation::get_empty(), Anchor::create(Span()), context);
  BAIL_IF(
      !monograph || !monograph->is<Library::Language::Monograph>() ||
      !cursor.matches(Code::Type::Terminal));
  return static_cast<Library::Language::Monograph&>(*monograph);
}

static auto complete_library(
    Allocator::Arena& arena,
    Library::Language::Monograph& monograph,
    View::Bytes source,
    Errors& errors) -> Bool {
  Tokenizer tokenizer(arena, source, "library-import.ttx"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  return monograph.link(cursor) && monograph.finalize(cursor);
}

static auto create_package(Allocator::Arena& arena, Package::Dialect& dialect)
    -> Package::Language::Monograph& {
  return Package::Language::Monograph::create_synthetic(
      arena, dialect, dialect, {});
}

static auto create_package(
    Allocator::Arena& arena,
    Package::Dialect& dialect,
    View::Bytes dependency_name) -> Package::Language::Monograph& {
  Managed::Vector<Package::Language::Dependency> dependencies(arena);
  dependencies.insert(
      Package::Language::Dependency(
          dependency_name, "Test.Dependency"_view, Version(1, 0)));
  return Package::Language::Monograph::create_synthetic(
      arena, dialect, dialect, dependencies);
}

static auto bind_dependency(
    Package::Language::Monograph& source,
    const Package::Language::Monograph& target) -> Bool {
  auto dependencies = source.get_dependencies();
  return dependencies.get_size() == 1 &&
         source.bind_dependency(dependencies.get_data()[0], target);
}

static auto diagnostic_contains(const Errors& errors, View::Bytes text)
    -> Bool {
  Allocator::Arena rendered;
  for (Count index = 0; index < errors.get_size(); index++) {
    if (Algorithm::search(errors.render_message(rendered, index), text) !=
        Count(-1)) {
      return True;
    }
  }
  return False;
}

static Harness LibraryImports = {
  .name = "Tetrodotoxin::Library::Language::Import"_view,
};

PERIMORTEM_UNIT_TEST(LibraryImports, statement_grammar) {
  static constexpr Static::Vector<View::Bytes, 2> accepted = {{
    "using Core;"_view,
    "using Runtime::Core::Api;"_view,
  }};
  for (Count index = 0; index < accepted.get_size(); index++) {
    View::Bytes source = accepted[index];
    Allocator::Arena arena;
    Errors errors;
    Tokenizer tokenizer(arena, source, "import.ttx"_view);
    Ttx::Lexical::Associations associations(tokenizer.get_arena());
    Cursor cursor(tokenizer, errors, associations);
    auto import =
        Library::Language::Import::parse(cursor, Documentation::get_empty());
    EXPECT(import && cursor.matches(Code::Type::Terminal));
    EXPECT(errors.is_empty());
  }

  static constexpr Static::Vector<View::Bytes, 8> rejected = {{
    "Using Core;"_view,
    "using core;"_view,
    "using Runtime ::Core;"_view,
    "using Runtime:: Core;"_view,
    "using;"_view,
    "using Core Other;"_view,
    "using Core"_view,
    "using Core[];"_view,
  }};
  for (Count index = 0; index < rejected.get_size(); index++) {
    View::Bytes source = rejected[index];
    Allocator::Arena arena;
    Errors errors;
    Tokenizer tokenizer(arena, source, "import.ttx"_view);
    Ttx::Lexical::Associations associations(tokenizer.get_arena());
    Cursor cursor(tokenizer, errors, associations);
    EXPECT_NOT(
        Library::Language::Import::parse(cursor, Documentation::get_empty()));
    EXPECT_NOT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(LibraryImports, selected_fallback) {
  static constexpr View::Bytes provider_source =
      "// Provider.\n"
      "public Provided : struct { public state ready : Bool; }\n"
      "private Hidden : struct { private state ready : Bool; }"_view;
  static constexpr View::Bytes importer_source =
      "// Importer.\n"
      "using Runtime::Core::Provider;\n"
      "public Local : struct { public state value : Provided; }"_view;

  Allocator::Arena arena;
  Library::Dialect library;
  Package::Dialect package;
  ImportContext context;
  Errors errors;
  auto provider =
      interpret_library(arena, library, context, provider_source, errors);
  ASSERT(provider);
  ASSERT(complete_library(arena, *provider, provider_source, errors));

  auto& target = create_package(arena, package);
  ASSERT(target.bind_member("Provider"_view, *provider));
  auto& runtime = create_package(arena, package);
  ASSERT(runtime.bind_member("Core"_view, target));
  auto& source = create_package(arena, package, "Runtime"_view);
  ASSERT(bind_dependency(source, runtime));

  auto importer =
      interpret_library(arena, library, source, importer_source, errors);
  ASSERT(importer);
  ASSERT(complete_library(arena, *importer, importer_source, errors));

  const Abstract& provided = provider->resolve_context("Provided"_view);
  EXPECT(
      &importer->resolve_context("Provided"_view).resolve() ==
      &provided.resolve());
  EXPECT(importer->resolve_context("Hidden"_view).is<Invalid>());
  auto local_types = importer->get_source().get_types();
  ASSERT(local_types != local_types.end());
  EXPECT_TEXT((*local_types).get().get_name(), "Local"_view);
  ++local_types;
  EXPECT(local_types == importer->get_source().get_types().end());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryImports, fallback_composition) {
  static constexpr View::Bytes upstream_source =
      "// Upstream.\n"
      "public Upstream : struct { public state ready : Bool; }"_view;
  static constexpr View::Bytes provider_source =
      "// Provider.\n"
      "using Up::Api;\n"
      "public Provider : struct { public state ready : Bool; }"_view;
  static constexpr View::Bytes importer_source =
      "// Importer.\n"
      "using Provider;\n"
      "public Local : struct { public state value : Upstream; }"_view;

  Allocator::Arena arena;
  Library::Dialect library;
  Package::Dialect package;
  ImportContext context;
  Errors errors;
  auto upstream =
      interpret_library(arena, library, context, upstream_source, errors);
  ASSERT(upstream);
  ASSERT(complete_library(arena, *upstream, upstream_source, errors));
  auto& upstream_package = create_package(arena, package);
  ASSERT(upstream_package.bind_member("Api"_view, *upstream));

  auto& provider_context = create_package(arena, package, "Up"_view);
  ASSERT(bind_dependency(provider_context, upstream_package));
  auto provider = interpret_library(
      arena, library, provider_context, provider_source, errors);
  ASSERT(provider);
  ASSERT(complete_library(arena, *provider, provider_source, errors));

  auto& importer_context = create_package(arena, package);
  ASSERT(importer_context.bind_member("Provider"_view, *provider));
  auto importer = interpret_library(
      arena, library, importer_context, importer_source, errors);
  ASSERT(importer);
  ASSERT(complete_library(arena, *importer, importer_source, errors));

  const Abstract& upstream_type = upstream->resolve_context("Upstream"_view);
  EXPECT(
      &importer->resolve_context("Upstream"_view).resolve() ==
      &upstream_type.resolve());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryImports, local_collision) {
  static constexpr View::Bytes provider_source =
      "// Provider.\n"
      "public Shared : struct { public state ready : Bool; }"_view;
  static constexpr View::Bytes importer_source =
      "// Importer.\n"
      "using Provider;\n"
      "public Shared : struct { public state local : Bool; }"_view;

  Allocator::Arena arena;
  Library::Dialect library;
  Package::Dialect package;
  ImportContext context;
  Errors errors;
  auto provider =
      interpret_library(arena, library, context, provider_source, errors);
  ASSERT(provider);
  ASSERT(complete_library(arena, *provider, provider_source, errors));
  auto& package_context = create_package(arena, package);
  ASSERT(package_context.bind_member("Provider"_view, *provider));
  auto importer = interpret_library(
      arena, library, package_context, importer_source, errors);
  ASSERT(importer);
  EXPECT_NOT(complete_library(arena, *importer, importer_source, errors));
  EXPECT(
      diagnostic_contains(errors, "conflicts with this source context"_view));
}

PERIMORTEM_UNIT_TEST(LibraryImports, route_diagnostics) {
  static constexpr View::Bytes provider_source =
      "// Provider.\n"
      "public Provided : struct { public state ready : Bool; }"_view;
  static constexpr Static::Vector<View::Bytes, 2> rejected = {{
    "// Missing.\n"
    "using Missing;"_view,
    "// Duplicate.\n"
    "using Provider;\n"
    "using Provider;"_view,
  }};

  for (Count index = 0; index < rejected.get_size(); index++) {
    Allocator::Arena arena;
    Library::Dialect library;
    Package::Dialect package;
    ImportContext context;
    Errors errors;
    auto provider =
        interpret_library(arena, library, context, provider_source, errors);
    ASSERT(provider);
    ASSERT(complete_library(arena, *provider, provider_source, errors));
    auto& package_context = create_package(arena, package);
    ASSERT(package_context.bind_member("Provider"_view, *provider));

    auto importer = interpret_library(
        arena, library, package_context, rejected[index], errors);
    ASSERT(importer);
    EXPECT_NOT(complete_library(arena, *importer, rejected[index], errors));
    EXPECT_NOT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(LibraryImports, ambiguous_fallback) {
  static constexpr View::Bytes provider_source =
      "// Provider.\n"
      "public Shared : struct { public state ready : Bool; }"_view;
  static constexpr View::Bytes importer_source =
      "// Importer.\n"
      "using First;\n"
      "using Second;"_view;

  Allocator::Arena arena;
  Library::Dialect library;
  Package::Dialect package;
  ImportContext context;
  Errors errors;
  auto first =
      interpret_library(arena, library, context, provider_source, errors);
  auto second =
      interpret_library(arena, library, context, provider_source, errors);
  ASSERT(first && second);
  ASSERT(complete_library(arena, *first, provider_source, errors));
  ASSERT(complete_library(arena, *second, provider_source, errors));

  auto& package_context = create_package(arena, package);
  ASSERT(package_context.bind_member("First"_view, *first));
  ASSERT(package_context.bind_member("Second"_view, *second));
  auto importer = interpret_library(
      arena, library, package_context, importer_source, errors);
  ASSERT(importer);
  ASSERT(complete_library(arena, *importer, importer_source, errors));
  EXPECT(importer->resolve_context("Shared"_view).is<Invalid>());
  EXPECT(errors.is_empty());
}
