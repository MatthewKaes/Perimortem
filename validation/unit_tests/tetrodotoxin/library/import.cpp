// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/import.hpp"

#include "validation/unit_test.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/package/repository/repository.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

class ImportRegistry : public Abstract {
 public:
  auto get_name() const -> View::Bytes override {
    return "ImportRegistry"_view;
  }

  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

static auto interpret_library(
    Allocator::Arena& arena,
    Library::Dialect& dialect,
    Abstract& context,
    View::Bytes source) -> Option<Library::Language::Monograph&> {
  Errors errors;
  Tokenizer tokenizer(arena, source, "library-import.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto interpreted = dialect.interpret(
      arena, cursor, Documentation::get_empty(), Anchor::create(Span()),
      context);
  if (!interpreted || !errors.is_empty() ||
      !cursor.matches(Code::Type::Terminal) ||
      !interpreted->is<Library::Language::Monograph>()) {
    return {};
  }

  return static_cast<Library::Language::Monograph&>(*interpreted);
}

static auto create_package(Allocator::Arena& arena, Package::Dialect&)
    -> Package::Language::Monograph& {
  Managed::Vector<Package::Language::Dependency> dependencies(arena);
  return Package::Language::Monograph::create_synthetic(
      arena, Documentation::get_empty(), dependencies);
}

static auto create_package_with_dependency(
    Allocator::Arena& arena,
    Package::Dialect&,
    View::Bytes local_name) -> Package::Language::Monograph& {
  Managed::Vector<Package::Language::Dependency> dependencies(arena);
  dependencies.insert(
      Package::Language::Dependency(
          local_name, "Pkg.Target"_view, Version(1, 0)));
  return Package::Language::Monograph::create_synthetic(
      arena, Documentation::get_empty(), dependencies);
}

static auto bind_only_dependency(
    Package::Language::Monograph& source,
    const Package::Language::Monograph& target) -> Bool {
  auto dependencies = source.get_dependencies();
  if (dependencies.get_size() != 1) {
    return False;
  }

  Bool bound = source.bind_dependency(dependencies.get_data()[0], target);
  return bound;
}

static auto diagnostic_matches(
    const Library::Language::Monograph& monograph,
    Count index,
    View::Bytes source,
    View::Bytes anchor_text,
    View::Bytes message) -> Bool {
  auto diagnostics = monograph.get_diagnostics();
  if (index >= diagnostics.get_size()) {
    return False;
  }

  const auto& diagnostic = diagnostics.get_data()[index];
  if (!diagnostic.get_anchor()) {
    return False;
  }

  return diagnostic.get_anchor()->get_span().caculate_text(source) ==
             anchor_text &&
         diagnostic.get_message() == message;
}

static Harness LibraryImports = {
  .name = "Tetrodotoxin::Library::Language::Import"_view,
};

PERIMORTEM_UNIT_TEST(LibraryImports, exact_statement_grammar) {
  static constexpr Static::Vector<View::Bytes, 2> accepted = {{
    "using Core;"_view,
    "using Runtime::Core;"_view,
  }};
  static constexpr Static::Vector<View::Bytes, 2> roots = {{
    "Core"_view,
    "Runtime"_view,
  }};

  // Reaching Terminal proves Import consumed the complete statement rather
  // than returning a route while leaving its delimiter for Dialect recovery.
  for (Count i = 0; i < accepted.get_size(); i++) {
    Allocator::Arena arena;
    Errors errors;
    Tokenizer tokenizer(arena, accepted[i], "accepted-import.ttx"_view);
    Cursor cursor(tokenizer, errors);
    auto import = Library::Language::Import::parse(cursor);
    ASSERT(import);
    const auto& reference = import->get_type_reference();
    EXPECT_EQ(reference.get_size(), i + 1);
    EXPECT_TEXT(reference.get_root(), roots[i]);
    EXPECT_TEXT(reference.get_name(reference.get_size() - 1), "Core"_view);
    EXPECT(cursor.matches(Code::Type::Terminal));
    EXPECT(errors.is_empty());
  }

  static constexpr Static::Vector<View::Bytes, 11> rejected = {{
    "Using Core;"_view,
    "use Core;"_view,
    "using core;"_view,
    "using Runtime ::Core;"_view,
    "using Runtime:: Core;"_view,
    "using;"_view,
    "using Core Other;"_view,
    "using Core"_view,
    "using Core trailing;"_view,
    "using Fixed[Unsigned_8, 4];"_view,
    "using Core[];"_view,
  }};

  for (Count i = 0; i < rejected.get_size(); i++) {
    Allocator::Arena arena;
    Errors errors;
    Tokenizer tokenizer(arena, rejected[i], "rejected-import.ttx"_view);
    Cursor cursor(tokenizer, errors);
    auto import = Library::Language::Import::parse(cursor);
    EXPECT_NOT(import);
    EXPECT(cursor.matches(Code::Type::Terminal));
    EXPECT_NOT(errors.is_empty());
  }
}

PERIMORTEM_UNIT_TEST(LibraryImports, exact_identity_and_exclusions) {
  static constexpr View::Bytes first_source =
      "public first : func = [] -> [] {}\n"
      "private hidden : func = [] -> [] {}"_view;
  static constexpr View::Bytes second_source =
      "public second : func = [] -> [] {}"_view;
  static constexpr View::Bytes dependency_source =
      "public dependency_only : func = [] -> [] {}"_view;
  static constexpr View::Bytes importer_source =
      "private local_private : func = [] -> [] {}\n"
      "using Runtime::Core;\n"
      "public local_public : func = [] -> [] {}"_view;

  Allocator::Arena arena;
  ImportRegistry registry;
  Library::Dialect library_dialect;
  Package::Dialect package_dialect;
  auto first =
      interpret_library(arena, library_dialect, registry, first_source);
  auto second =
      interpret_library(arena, library_dialect, registry, second_source);
  auto dependency_library =
      interpret_library(arena, library_dialect, registry, dependency_source);
  ASSERT(first && second && dependency_library);

  // This test drives Monographs directly, so it supplies the provider link
  // barrier that Environment Retention would run before Alias observation.
  ASSERT(first->link());
  ASSERT(second->link());

  // The selected Package mixes direct Library members with a nested Package
  // and a Dependency. Each accepted binding crosses through a new importer
  // Alias while the Package member and provider identities remain distinct.
  Package::Language::Monograph& dependency_package =
      create_package(arena, package_dialect);
  Bool dependency_member_bound =
      dependency_package.bind_member("DependencyApi"_view, *dependency_library);
  ASSERT(dependency_member_bound);

  Package::Language::Monograph& nested_package =
      create_package(arena, package_dialect);
  Package::Language::Monograph& target_package =
      create_package_with_dependency(arena, package_dialect, "Dependency"_view);
  Bool dependency_bound = target_package.bind_dependency(
      target_package.get_dependencies().get_data()[0], dependency_package);
  Bool first_member_bound = target_package.bind_member("FirstApi"_view, *first);
  Bool nested_member_bound =
      target_package.bind_member("Nested"_view, nested_package);
  Bool second_member_bound =
      target_package.bind_member("SecondApi"_view, *second);
  ASSERT(dependency_bound);
  ASSERT(first_member_bound);
  ASSERT(nested_member_bound);
  ASSERT(second_member_bound);

  Package::Language::Monograph& runtime_package =
      create_package(arena, package_dialect);
  ASSERT(runtime_package.bind_member("Core"_view, target_package));
  Package::Language::Monograph& source_package =
      create_package_with_dependency(arena, package_dialect, "Runtime"_view);
  Bool source_dependency_bound =
      bind_only_dependency(source_package, runtime_package);
  ASSERT(source_dependency_bound);
  auto importer = interpret_library(
      arena, library_dialect, source_package, importer_source);
  ASSERT(importer);
  const auto& first_scope = first->get_source();
  const auto& second_scope = second->get_source();
  const auto& importer_scope = importer->get_source();
  auto first_public =
      first_scope.get_callables(Tetrodotoxin::Language::Visibility::Public);
  ASSERT(first_public != first_public.end());
  const Abstract& first_identity = (*first_public).get();
  ASSERT(first_identity.is<Library::Language::Function>());
  EXPECT_TEXT(first_identity.get_name(), "first"_view);
  ++first_public;
  ASSERT(first_public == first_public.end());
  auto second_public =
      second_scope.get_callables(Tetrodotoxin::Language::Visibility::Public);
  ASSERT(second_public != second_public.end());
  const Abstract& second_identity = (*second_public).get();
  ASSERT(second_identity.is<Library::Language::Function>());
  EXPECT_TEXT(second_identity.get_name(), "second"_view);
  auto authored = importer_scope.get_callables();
  ASSERT(authored != authored.end());
  ASSERT((*authored).get().is<Library::Language::Function>());
  const auto& private_local =
      static_cast<const Library::Language::Function&>((*authored).get());
  ++authored;
  ASSERT(authored != authored.end());
  ASSERT((*authored).get().is<Library::Language::Function>());
  const auto& local_host =
      static_cast<const Library::Language::Function&>((*authored).get());
  const Abstract& public_local = local_host;
  EXPECT(
      &importer->resolve_context("local_public"_view) ==
      &Invalid::get_invalid());
  EXPECT(
      &importer->resolve_context("local_private"_view) ==
      &Invalid::get_invalid());
  auto public_candidates =
      importer_scope.get_callables(Tetrodotoxin::Language::Visibility::Public);
  ASSERT(public_candidates != public_candidates.end());
  EXPECT(&(*public_candidates).get() == &public_local);
  ++public_candidates;
  EXPECT(public_candidates == public_candidates.end());

  Bool import_completed = importer->link();
  ASSERT(import_completed);
  auto local_candidates =
      importer_scope.get_callables(Tetrodotoxin::Language::Visibility::Private);
  ASSERT(local_candidates != local_candidates.end());
  EXPECT(&(*local_candidates).get() == &private_local);
  ++local_candidates;
  ASSERT(local_candidates != local_candidates.end());
  EXPECT(&(*local_candidates).get() == &local_host);
  ++local_candidates;
  ASSERT(local_candidates != local_candidates.end());
  const Abstract& first_alias = (*local_candidates).get();
  ++local_candidates;
  ASSERT(local_candidates != local_candidates.end());
  const Abstract& second_alias = (*local_candidates).get();
  ++local_candidates;
  ASSERT(local_candidates == local_candidates.end());
  ASSERT(first_alias.is<Ttx::Model::Alias>());
  ASSERT(second_alias.is<Ttx::Model::Alias>());
  EXPECT_TEXT(first_alias.get_name(), "first"_view);
  EXPECT_TEXT(second_alias.get_name(), "second"_view);
  EXPECT(&first_alias != &first_identity);
  EXPECT(&second_alias != &second_identity);
  EXPECT(&first_alias.resolve() == &first_identity);
  EXPECT(&second_alias.resolve() == &second_identity);
  auto target_members = target_package.get_members();
  ASSERT_EQ(target_members.get_size(), Count(3));
  const Ttx::Model::Alias& first_package_member =
      target_members.get_data()[0].get();
  EXPECT(&first_package_member != &first_alias);
  EXPECT(&first_package_member.resolve() == &*first);
  EXPECT(&first_alias.resolve() == &first_identity);
  EXPECT(&first_identity != &*first);
  EXPECT(&importer->resolve_context("first"_view) == &Invalid::get_invalid());
  EXPECT(&importer->resolve_context("second"_view) == &Invalid::get_invalid());
  EXPECT(&importer->resolve_context("hidden"_view) == &Invalid::get_invalid());
  EXPECT(&local_host.resolve_context("first"_view) == &Invalid::get_invalid());
  EXPECT(&local_host.resolve_context("second"_view) == &Invalid::get_invalid());
  EXPECT(&local_host.resolve_context("hidden"_view) == &Invalid::get_invalid());
  EXPECT(
      &local_host.resolve_context("dependency_only"_view) ==
      &Invalid::get_invalid());
  EXPECT(&local_host.resolve_context("Nested"_view) == &Invalid::get_invalid());
  EXPECT(
      &importer->resolve_context("local_public"_view) ==
      &Invalid::get_invalid());
  EXPECT(
      &local_host.resolve_context("local_private"_view) ==
      &Invalid::get_invalid());
  EXPECT(
      &local_host.resolve_context("Bool"_view) ==
      &library_dialect.resolve_intrinsic("Bool"_view));
  EXPECT(&importer->resolve_context("Bool"_view) == &Invalid::get_invalid());
  auto linked_public_candidates =
      importer_scope.get_callables(Tetrodotoxin::Language::Visibility::Public);
  ASSERT(linked_public_candidates != linked_public_candidates.end());
  EXPECT(&(*linked_public_candidates).get() == &public_local);
  ++linked_public_candidates;
  EXPECT(linked_public_candidates == linked_public_candidates.end());

  const Abstract& first_alias_identity = first_alias;
  ASSERT(importer->link());
  auto repeated_candidates =
      importer_scope.get_callables(Tetrodotoxin::Language::Visibility::Private);
  ASSERT(repeated_candidates != repeated_candidates.end());
  ++repeated_candidates;
  ASSERT(repeated_candidates != repeated_candidates.end());
  ++repeated_candidates;
  ASSERT(repeated_candidates != repeated_candidates.end());
  EXPECT(&(*repeated_candidates).get() == &first_alias_identity);
  ++repeated_candidates;
  ASSERT(repeated_candidates != repeated_candidates.end());
  ++repeated_candidates;
  EXPECT(repeated_candidates == repeated_candidates.end());
}

PERIMORTEM_UNIT_TEST(LibraryImports, source_field_keeps_provider_identity) {
  static constexpr View::Bytes provider_source =
      "public provided : Unsigned_8 = 7;\n"
      "private hidden : Unsigned_8;"_view;
  static constexpr View::Bytes importer_source =
      "using Core;\n"
      "private Consumer : struct {\n"
      "  public accept : func = [.value : Unsigned_8] -> [] {}\n"
      "}\n"
      "private consumer : func = [] -> [] {\n"
      "  Consumer -> accept(provided);\n"
      "}"_view;
  Allocator::Arena arena;
  ImportRegistry registry;
  Library::Dialect library_dialect;
  Package::Dialect package_dialect;
  auto provider =
      interpret_library(arena, library_dialect, registry, provider_source);
  ASSERT(provider);

  Package::Language::Monograph& target = create_package(arena, package_dialect);
  ASSERT(target.bind_member("Provider"_view, *provider));
  Package::Language::Monograph& context =
      create_package_with_dependency(arena, package_dialect, "Core"_view);
  ASSERT(bind_only_dependency(context, target));
  auto importer =
      interpret_library(arena, library_dialect, context, importer_source);
  ASSERT(importer);

  ASSERT(importer->link());
  ASSERT(provider->finalize());
  ASSERT(importer->finalize());
  const auto& provider_type = provider->get_source();
  const auto& importer_type = importer->get_source();
  auto provider_fields = provider_type.get_addressables();
  auto provider_field_iterator = provider_fields.begin();
  ASSERT(provider_field_iterator != provider_fields.end());
  const auto& provided = static_cast<const Library::Language::Field&>(
      (*provider_field_iterator).get());
  ++provider_field_iterator;
  ASSERT(provider_field_iterator != provider_fields.end());
  ++provider_field_iterator;
  EXPECT(provider_field_iterator == provider_fields.end());

  // Provider publication exposes only the public Field while preserving the
  // private Field as a source owned identity.
  EXPECT(&provider->resolve_context("provided"_view) == &provided);
  EXPECT(&provider->resolve_context("hidden"_view) == &Invalid::get_invalid());

  auto imported_fields = importer_type.get_addressables();
  auto imported_field_iterator = imported_fields.begin();
  ASSERT(imported_field_iterator != imported_fields.end());
  const Abstract& imported = (*imported_field_iterator).get();
  ++imported_field_iterator;
  ASSERT(imported_field_iterator == imported_fields.end());
  ASSERT(imported.is<Ttx::Model::Alias>());
  EXPECT_TEXT(imported.get_name(), "provided"_view);
  const auto& alias = static_cast<const Ttx::Model::Alias&>(imported);

  // Importer keeps its private Alias but resolution still reaches the exact
  // provider Field rather than a copied declaration or value.
  EXPECT(&alias.resolve() == &provided);
  EXPECT(
      &importer->resolve_context("provided"_view) == &Invalid::get_invalid());

  auto callables = importer_type.get_callables();
  auto callable_iterator = callables.begin();
  ASSERT(callable_iterator != callables.end());
  ASSERT((*callable_iterator).get().is<Library::Language::Function>());
  const auto& consumer = static_cast<const Library::Language::Function&>(
      (*callable_iterator).get());

  // Root linking resolves the call argument through the private Alias. A
  // completed Call proves that the real argument Pack fitted its selected
  // Callable without exposing a second input inventory for inspection.
  auto body = consumer.get_body();
  ASSERT(body);
  auto statements = body->get_statements();
  ASSERT_EQ(statements.get_size(), Count(1));
  ASSERT(statements.get_data()[0].get().is<Library::Language::Access::Call>());
  const auto& call = static_cast<const Library::Language::Access::Call&>(
      statements.get_data()[0].get());
  ASSERT(call.get_callable());
  EXPECT(&consumer.resolve_context("hidden"_view) == &Invalid::get_invalid());
}

PERIMORTEM_UNIT_TEST(LibraryImports, source_field_collision_is_transactional) {
  Allocator::Arena arena;
  ImportRegistry registry;
  Library::Dialect library_dialect;
  Package::Dialect package_dialect;
  auto provider = interpret_library(
      arena, library_dialect, registry, "public repeated : Bool;"_view);
  ASSERT(provider);

  Package::Language::Monograph& target = create_package(arena, package_dialect);
  ASSERT(target.bind_member("Provider"_view, *provider));
  Package::Language::Monograph& context =
      create_package_with_dependency(arena, package_dialect, "Core"_view);
  ASSERT(bind_only_dependency(context, target));
  auto importer = interpret_library(
      arena, library_dialect, context,
      "using Core;\npublic repeated : Bool;\n"
      "public later : func = [] -> [] {}"_view);
  ASSERT(importer);

  // Retained provider Fields preflight the Addressable category before either
  // source publishes them or lets a later Callable signature advance.
  ASSERT_NOT(importer->link());
  const auto& provider_type = provider->get_source();
  const auto& importer_type = importer->get_source();
  auto provider_fields = provider_type.get_addressables();
  auto importer_fields = importer_type.get_addressables();
  auto provider_field_iterator = provider_fields.begin();
  ASSERT(provider_field_iterator != provider_fields.end());
  EXPECT(
      &(*provider_field_iterator).get().resolve() == &Invalid::get_invalid());
  ++provider_field_iterator;
  ASSERT(provider_field_iterator == provider_fields.end());
  auto importer_field_iterator = importer_fields.begin();
  ASSERT(importer_field_iterator != importer_fields.end());
  EXPECT(
      &(*importer_field_iterator).get().resolve() == &Invalid::get_invalid());
  ++importer_field_iterator;
  ASSERT(importer_field_iterator == importer_fields.end());
  auto provider_public_fields = provider_type.get_addressables(
      Tetrodotoxin::Language::Visibility::Public);
  EXPECT(provider_public_fields.begin() == provider_public_fields.end());
  auto importer_public =
      importer_type.get_callables(Tetrodotoxin::Language::Visibility::Public);
  auto importer_public_iterator = importer_public.begin();
  ASSERT(importer_public_iterator != importer_public.end());
  EXPECT((*importer_public_iterator).get().is<Library::Language::Function>());
  EXPECT(diagnostic_matches(
      *importer, 0,
      "using Core;\npublic repeated : Bool;\n"
      "public later : func = [] -> [] {}"_view,
      "using Core;"_view,
      "Imported Static binding collides with its source category."_view));
}

PERIMORTEM_UNIT_TEST(LibraryImports, provider_import_is_not_reexported) {
  Allocator::Arena arena;
  ImportRegistry registry;
  Library::Dialect library_dialect;
  Package::Dialect package_dialect;
  auto upstream = interpret_library(
      arena, library_dialect, registry,
      "public upstream : func = [] -> [] {}"_view);
  ASSERT(upstream);

  // Provider linking can install an Alias before its target links. Settle the
  // upstream Function here because this test observes resolution immediately.
  ASSERT(upstream->link());

  Package::Language::Monograph& upstream_target =
      create_package(arena, package_dialect);
  Bool upstream_member_bound =
      upstream_target.bind_member("UpstreamApi"_view, *upstream);
  ASSERT(upstream_member_bound);
  Package::Language::Monograph& provider_context =
      create_package_with_dependency(arena, package_dialect, "Upstream"_view);
  Bool provider_dependency_bound =
      bind_only_dependency(provider_context, upstream_target);
  ASSERT(provider_dependency_bound);
  auto provider = interpret_library(
      arena, library_dialect, provider_context,
      "using Upstream;\npublic direct : func = [] -> [] {}"_view);
  ASSERT(provider);
  Bool provider_completed = provider->link();
  ASSERT(provider_completed);
  const auto& provider_source = provider->get_source();
  auto provider_bindings = provider_source.get_callables();
  auto provider_iterator = provider_bindings.begin();
  ASSERT(provider_iterator != provider_bindings.end());
  const Abstract& provider_host_identity = (*provider_iterator).get();
  ASSERT(provider_host_identity.is<Library::Language::Function>());
  EXPECT_TEXT(provider_host_identity.get_name(), "direct"_view);
  const auto& provider_host =
      static_cast<const Library::Language::Function&>(provider_host_identity);
  ++provider_iterator;
  ASSERT(provider_iterator != provider_bindings.end());
  const Abstract& upstream_alias = (*provider_iterator).get();
  ASSERT(upstream_alias.is<Ttx::Model::Alias>());
  EXPECT_TEXT(upstream_alias.get_name(), "upstream"_view);
  ++provider_iterator;
  ASSERT(provider_iterator == provider_bindings.end());
  EXPECT(
      &provider->resolve_context("upstream"_view) == &Invalid::get_invalid());
  EXPECT(
      &provider_host.resolve_context("upstream"_view) ==
      &Invalid::get_invalid());
  const auto& upstream_source = upstream->get_source();
  auto upstream_callables = upstream_source.get_callables(
      Tetrodotoxin::Language::Visibility::Private);
  auto upstream_iterator = upstream_callables.begin();
  ASSERT(upstream_iterator != upstream_callables.end());
  const Abstract& upstream_identity = (*upstream_iterator).get();
  EXPECT(&upstream_alias.resolve() == &upstream_identity);
  auto provider_public =
      provider_source.get_callables(Tetrodotoxin::Language::Visibility::Public);
  auto provider_public_iterator = provider_public.begin();
  ASSERT(provider_public_iterator != provider_public.end());
  EXPECT(&(*provider_public_iterator).get() == &provider_host);
  ++provider_public_iterator;
  ASSERT(provider_public_iterator == provider_public.end());

  // The provider retains its upstream candidate locally, but its public view
  // contains only the authored declaration. A downstream Import consumes that
  // narrow view instead of repeating the provider link result.
  Package::Language::Monograph& provider_target =
      create_package(arena, package_dialect);
  Bool provider_member_bound =
      provider_target.bind_member("ProviderApi"_view, *provider);
  ASSERT(provider_member_bound);
  Package::Language::Monograph& importer_context =
      create_package_with_dependency(arena, package_dialect, "Provider"_view);
  Bool importer_dependency_bound =
      bind_only_dependency(importer_context, provider_target);
  ASSERT(importer_dependency_bound);
  auto importer = interpret_library(
      arena, library_dialect, importer_context,
      "using Provider;\nprivate consumer : func = [] -> [] {}"_view);
  ASSERT(importer);
  Bool importer_completed = importer->link();
  ASSERT(importer_completed);
  const auto& importer_source = importer->get_source();
  auto importer_candidates = importer_source.get_callables(
      Tetrodotoxin::Language::Visibility::Private);
  auto importer_iterator = importer_candidates.begin();
  ASSERT(importer_iterator != importer_candidates.end());
  const Abstract& importer_host_identity = (*importer_iterator).get();
  ASSERT(importer_host_identity.is<Library::Language::Function>());
  EXPECT_TEXT(importer_host_identity.get_name(), "consumer"_view);
  const auto& importer_host =
      static_cast<const Library::Language::Function&>(importer_host_identity);
  ++importer_iterator;
  ASSERT(importer_iterator != importer_candidates.end());
  const Abstract& direct_alias = (*importer_iterator).get();
  ASSERT(direct_alias.is<Ttx::Model::Alias>());
  EXPECT_TEXT(direct_alias.get_name(), "direct"_view);
  ++importer_iterator;
  ASSERT(importer_iterator == importer_candidates.end());
  EXPECT(&importer->resolve_context("direct"_view) == &Invalid::get_invalid());
  EXPECT(
      &importer_host.resolve_context("direct"_view) == &Invalid::get_invalid());
  EXPECT(&direct_alias.resolve() == &provider_host);
  auto importer_public =
      importer_source.get_callables(Tetrodotoxin::Language::Visibility::Public);
  EXPECT(importer_public.begin() == importer_public.end());
}

PERIMORTEM_UNIT_TEST(LibraryImports, category_collisions_are_atomic) {
  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect;
    Package::Dialect package_dialect;
    auto unique = interpret_library(
        arena, library_dialect, registry, "public Unique : struct {}"_view);
    auto colliding = interpret_library(
        arena, library_dialect, registry, "public Clash : struct {}"_view);
    ASSERT(unique && colliding);

    // Member order encounters unique before the later local collision. Its
    // absence afterward proves candidate discovery never mutates lookup.
    Package::Language::Monograph& target =
        create_package(arena, package_dialect);
    Bool first_member_bound = target.bind_member("First"_view, *unique);
    Bool second_member_bound = target.bind_member("Second"_view, *colliding);
    ASSERT(first_member_bound);
    ASSERT(second_member_bound);
    Package::Language::Monograph& context =
        create_package_with_dependency(arena, package_dialect, "Core"_view);
    Bool dependency_bound = bind_only_dependency(context, target);
    ASSERT(dependency_bound);
    auto importer = interpret_library(
        arena, library_dialect, context,
        "using Core;\npublic Clash : struct {}"_view);
    ASSERT(importer);
    const Abstract& local = importer->resolve_context("Clash"_view);
    Bool completed = importer->link();
    ASSERT_NOT(completed);
    EXPECT(
        &importer->resolve_context("Unique"_view) == &Invalid::get_invalid());
    EXPECT(&importer->resolve_context("Clash"_view) == &local);
    ASSERT_EQ(importer->get_diagnostics().get_size(), Count(1));
    EXPECT(diagnostic_matches(
        *importer, 0, "using Core;\npublic Clash : struct {}"_view,
        "using Core;"_view,
        "Imported Static binding collides with its source category."_view));
  }

  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect;
    Package::Dialect package_dialect;
    auto first = interpret_library(
        arena, library_dialect, registry,
        "public unique : func = [] -> [] {}\n"
        "public repeated : func = [] -> [] {}"_view);
    auto second = interpret_library(
        arena, library_dialect, registry,
        "public repeated : func = [] -> [] {}"_view);
    ASSERT(first && second);
    ASSERT(first->link());
    ASSERT(second->link());

    // Callable uniqueness is proven before any imported Alias becomes visible.
    // The earlier unique candidate therefore remains absent beside the later
    // collision in the same receiver role.
    Package::Language::Monograph& target =
        create_package(arena, package_dialect);
    Bool first_member_bound = target.bind_member("FirstProvider"_view, *first);
    Bool second_member_bound =
        target.bind_member("SecondProvider"_view, *second);
    ASSERT(first_member_bound);
    ASSERT(second_member_bound);
    Package::Language::Monograph& context =
        create_package_with_dependency(arena, package_dialect, "Core"_view);
    Bool dependency_bound = bind_only_dependency(context, target);
    ASSERT(dependency_bound);
    auto importer =
        interpret_library(arena, library_dialect, context, "using Core;"_view);
    ASSERT(importer);
    Bool completed = importer->link();
    ASSERT_NOT(completed);
    EXPECT(
        &importer->resolve_context("unique"_view) == &Invalid::get_invalid());
    EXPECT(
        &importer->resolve_context("repeated"_view) == &Invalid::get_invalid());
    const auto& importer_source = importer->get_source();
    auto retained = importer_source.get_callables(
        Tetrodotoxin::Language::Visibility::Private);
    EXPECT(retained.begin() == retained.end());
    EXPECT_NOT(importer->get_diagnostics().is_empty());
  }

  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect;
    Package::Dialect package_dialect;
    auto provider = interpret_library(
        arena, library_dialect, registry,
        "public only : func = [] -> [] {}"_view);
    ASSERT(provider);
    Package::Language::Monograph& target =
        create_package(arena, package_dialect);
    Bool provider_bound = target.bind_member("Provider"_view, *provider);
    ASSERT(provider_bound);
    Package::Language::Monograph& context =
        create_package_with_dependency(arena, package_dialect, "Core"_view);
    Bool dependency_bound = bind_only_dependency(context, target);
    ASSERT(dependency_bound);
    auto importer = interpret_library(
        arena, library_dialect, context, "using Core;\nusing Core;"_view);
    ASSERT(importer);
    Bool completed = importer->link();
    ASSERT_NOT(completed);
    EXPECT(&importer->resolve_context("only"_view) == &Invalid::get_invalid());
    const auto& importer_source = importer->get_source();
    auto importer_private = importer_source.get_callables(
        Tetrodotoxin::Language::Visibility::Private);
    EXPECT(importer_private.begin() == importer_private.end());
    ASSERT_EQ(importer->get_diagnostics().get_size(), Count(1));
    EXPECT(diagnostic_matches(
        *importer, 0, "using Core;\nusing Core;"_view, "using Core;"_view,
        "Library source repeats one exact Import route."_view));
  }

  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect;
    Package::Dialect package_dialect;
    auto provider = interpret_library(
        arena, library_dialect, registry, "public Occupied : struct {}"_view);
    ASSERT(provider);

    Package::Language::Monograph& target =
        create_package(arena, package_dialect);
    Bool provider_bound = target.bind_member("Provider"_view, *provider);
    ASSERT(provider_bound);
    Package::Language::Monograph& context =
        create_package_with_dependency(arena, package_dialect, "Core"_view);
    Bool dependency_bound = bind_only_dependency(context, target);
    Bool occupied_bound = context.bind_member("Occupied"_view, *provider);
    ASSERT(dependency_bound);
    ASSERT(occupied_bound);

    // The Package context occupies this name for internal binding checks. It
    // does not leak through the external Monograph view on either outcome.
    auto importer =
        interpret_library(arena, library_dialect, context, "using Core;"_view);
    ASSERT(importer);
    EXPECT(
        &importer->resolve_context("Occupied"_view) == &Invalid::get_invalid());
    Bool completed = importer->link();
    ASSERT_NOT(completed);
    EXPECT(
        &importer->resolve_context("Occupied"_view) == &Invalid::get_invalid());
    ASSERT_EQ(importer->get_diagnostics().get_size(), Count(1));
    EXPECT(diagnostic_matches(
        *importer, 0, "using Core;"_view, "using Core;"_view,
        "Imported Static binding collides with its source category."_view));
  }
}

PERIMORTEM_UNIT_TEST(LibraryImports, invalid_targets_are_atomic) {
  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect;
    auto importer = interpret_library(
        arena, library_dialect, registry,
        "using Core;\npublic local : func = [] -> [] {}"_view);
    ASSERT(importer);
    const auto& importer_source = importer->get_source();
    auto authored = importer_source.get_callables();
    auto authored_iterator = authored.begin();
    ASSERT(authored_iterator != authored.end());
    const Abstract& local = (*authored_iterator).get();

    // An Import needs its source Package even when its route could miss in any
    // Abstract. Rejection leaves the local declaration as the only candidate.
    Bool completed = importer->link();
    ASSERT_NOT(completed);
    EXPECT(&importer->resolve_context("local"_view) == &Invalid::get_invalid());
    auto candidates = importer_source.get_callables(
        Tetrodotoxin::Language::Visibility::Private);
    auto candidate_iterator = candidates.begin();
    ASSERT(candidate_iterator != candidates.end());
    EXPECT(&(*candidate_iterator).get() == &local);
    ++candidate_iterator;
    ASSERT(candidate_iterator == candidates.end());
    ASSERT_EQ(importer->get_diagnostics().get_size(), Count(1));
    EXPECT(diagnostic_matches(
        *importer, 0, "using Core;\npublic local : func = [] -> [] {}"_view,
        "using Core;"_view,
        "Library Import source context is not a Package Monograph."_view));
  }

  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect;
    Package::Dialect package_dialect;
    auto provider = interpret_library(
        arena, library_dialect, registry,
        "public staged : func = [] -> [] {}"_view);
    ASSERT(provider);
    Package::Language::Monograph& target =
        create_package(arena, package_dialect);
    Bool provider_bound = target.bind_member("Provider"_view, *provider);
    ASSERT(provider_bound);
    Package::Language::Monograph& context =
        create_package_with_dependency(arena, package_dialect, "Core"_view);
    Bool dependency_bound = bind_only_dependency(context, target);
    ASSERT(dependency_bound);
    // A valid earlier route can stage a Function before a later missing route.
    // Returning failure must still leave that Function absent.
    auto importer = interpret_library(
        arena, library_dialect, context, "using Core;\nusing Missing;"_view);
    ASSERT(importer);
    Bool completed = importer->link();
    ASSERT_NOT(completed);
    const auto& importer_source = importer->get_source();
    auto importer_private = importer_source.get_callables(
        Tetrodotoxin::Language::Visibility::Private);
    EXPECT(importer_private.begin() == importer_private.end());
    ASSERT_EQ(importer->get_diagnostics().get_size(), Count(1));
    EXPECT(diagnostic_matches(
        *importer, 0, "using Core;\nusing Missing;"_view, "using Missing;"_view,
        "Library Import route did not resolve to a Package Monograph."_view));
  }

  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect;
    Package::Dialect package_dialect;
    auto provider = interpret_library(
        arena, library_dialect, registry,
        "public staged : func = [] -> [] {}"_view);
    ASSERT(provider);
    Package::Language::Monograph& target =
        create_package(arena, package_dialect);
    Bool provider_bound = target.bind_member("Provider"_view, *provider);
    ASSERT(provider_bound);
    Package::Language::Monograph& context =
        create_package_with_dependency(arena, package_dialect, "Core"_view);
    Bool dependency_bound = bind_only_dependency(context, target);
    Bool direct_member_bound = context.bind_member("Direct"_view, *provider);
    ASSERT(dependency_bound);
    ASSERT(direct_member_bound);
    auto importer = interpret_library(
        arena, library_dialect, context, "using Core;\nusing Direct;"_view);
    ASSERT(importer);
    Bool completed = importer->link();
    ASSERT_NOT(completed);
    const auto& importer_source = importer->get_source();
    auto importer_private = importer_source.get_callables(
        Tetrodotoxin::Language::Visibility::Private);
    EXPECT(importer_private.begin() == importer_private.end());
    ASSERT_EQ(importer->get_diagnostics().get_size(), Count(1));
    EXPECT(diagnostic_matches(
        *importer, 0, "using Core;\nusing Direct;"_view, "using Direct;"_view,
        "Library Import route did not resolve to a Package Monograph."_view));
  }
}

PERIMORTEM_UNIT_TEST(LibraryImports, retry_preserves_local_alias) {
  static constexpr View::Bytes source =
      "using Core;\nprivate consumer : func = [] -> [] {}"_view;
  Allocator::Arena arena;
  ImportRegistry registry;
  Library::Dialect library_dialect;
  Package::Dialect package_dialect;
  auto provider = interpret_library(
      arena, library_dialect, registry,
      "public ready : func = [] -> [] {}"_view);
  ASSERT(provider);
  ASSERT(provider->link());

  Package::Language::Monograph& target = create_package(arena, package_dialect);
  ASSERT(target.bind_member("Provider"_view, *provider));
  Package::Language::Monograph& context =
      create_package_with_dependency(arena, package_dialect, "Core"_view);
  auto importer = interpret_library(arena, library_dialect, context, source);
  ASSERT(importer);
  const auto& importer_source = importer->get_source();
  auto authored = importer_source.get_callables();
  auto authored_iterator = authored.begin();
  ASSERT(authored_iterator != authored.end());
  ASSERT((*authored_iterator).get().is<Library::Language::Function>());
  const Abstract& consumer_identity = (*authored_iterator).get();

  // The missing dependency fails before the source signature barrier, leaving
  // the import transaction open for the exact Package edge to arrive later.
  ASSERT_NOT(importer->link());
  auto failed_callables = importer_source.get_callables(
      Tetrodotoxin::Language::Visibility::Private);
  auto failed_iterator = failed_callables.begin();
  ASSERT(failed_iterator != failed_callables.end());
  EXPECT(&(*failed_iterator).get() == &consumer_identity);
  ++failed_iterator;
  EXPECT(failed_iterator == failed_callables.end());
  ASSERT(bind_only_dependency(context, target));
  ASSERT(importer->link());
  auto linked_callables = importer_source.get_callables(
      Tetrodotoxin::Language::Visibility::Private);
  auto linked_iterator = linked_callables.begin();
  ASSERT(linked_iterator != linked_callables.end());
  EXPECT(&(*linked_iterator).get() == &consumer_identity);
  ++linked_iterator;
  ASSERT(linked_iterator != linked_callables.end());
  const Abstract& alias = (*linked_iterator).get();
  ASSERT(alias.is<Ttx::Model::Alias>());
  EXPECT_TEXT(alias.get_name(), "ready"_view);
  ++linked_iterator;
  ASSERT(linked_iterator == linked_callables.end());
  const auto& provider_source = provider->get_source();
  auto provider_callables = provider_source.get_callables(
      Tetrodotoxin::Language::Visibility::Private);
  auto provider_iterator = provider_callables.begin();
  ASSERT(provider_iterator != provider_callables.end());
  EXPECT(&alias.resolve() == &(*provider_iterator).get());
  const Abstract& alias_identity = alias;
  ASSERT(importer->link());
  auto repeated_callables = importer_source.get_callables(
      Tetrodotoxin::Language::Visibility::Private);
  auto repeated_iterator = repeated_callables.begin();
  ASSERT(repeated_iterator != repeated_callables.end());
  EXPECT(&(*repeated_iterator).get() == &consumer_identity);
  ++repeated_iterator;
  ASSERT(repeated_iterator != repeated_callables.end());
  EXPECT(&(*repeated_iterator).get() == &alias_identity);
  ++repeated_iterator;
  EXPECT(repeated_iterator == repeated_callables.end());
}

PERIMORTEM_UNIT_TEST(LibraryImports, private_type_alias_cannot_escape) {
  static constexpr View::Bytes source =
      "using Core;\npublic publish : func = [.value : Shared] -> [] {}"_view;
  Allocator::Arena arena;
  ImportRegistry registry;
  Library::Dialect library_dialect;
  Package::Dialect package_dialect;
  auto provider = interpret_library(
      arena, library_dialect, registry,
      "public Shared : struct { public state value : Bool; }"_view);
  ASSERT(provider);
  ASSERT(provider->link());
  const Abstract& shared = provider->resolve_context("Shared"_view);
  ASSERT(shared.is<Library::Language::Types::Structure>());

  Package::Language::Monograph& target = create_package(arena, package_dialect);
  ASSERT(target.bind_member("Provider"_view, *provider));
  Package::Language::Monograph& context =
      create_package_with_dependency(arena, package_dialect, "Core"_view);
  ASSERT(bind_only_dependency(context, target));
  auto importer = interpret_library(arena, library_dialect, context, source);
  ASSERT(importer);
  ASSERT(importer->link());
  const auto& importer_source = importer->get_source();
  auto callables = importer_source.get_callables();
  auto callable_iterator = callables.begin();
  ASSERT(callable_iterator != callables.end());
  ASSERT((*callable_iterator).get().is<Library::Language::Function>());
  const auto& publish = static_cast<const Library::Language::Function&>(
      (*callable_iterator).get());
  const Layout& parameters = publish.get_parameters();
  ASSERT_EQ(parameters.get_size(), Count(1));
  auto parameter = parameters.get_abstract(0);
  ASSERT(parameter && parameter->is<Ttx::Model::Addressable>());
  EXPECT(
      &static_cast<const Ttx::Model::Addressable&>(*parameter).get_type() ==
      &shared);
  EXPECT(&importer->resolve_context("Shared"_view) == &Invalid::get_invalid());
  ASSERT_NOT(importer->finalize());
  auto diagnostics = importer->get_diagnostics();
  ASSERT_EQ(diagnostics.get_size(), Count(1));
  ASSERT(diagnostics.get_data()[0].get_anchor());
  EXPECT_TEXT(
      diagnostics.get_data()[0].get_anchor()->get_span().caculate_text(source),
      "Shared"_view);
}

PERIMORTEM_UNIT_TEST(LibraryImports, provider_alias_identity_is_retained) {
  static constexpr View::Bytes provider_source =
      "public Shared : struct { public state value : Bool; }\n"
      "public Exported : alias = Shared;\n"
      "private Hidden : alias = Shared;"_view;
  Allocator::Arena arena;
  ImportRegistry registry;
  Library::Dialect library_dialect;
  Package::Dialect package_dialect;
  auto provider =
      interpret_library(arena, library_dialect, registry, provider_source);
  ASSERT(provider);
  const auto& provider_source_type = provider->get_source();
  const Abstract& shared = provider_source_type.resolve_context("Shared"_view);
  const Abstract& exported =
      provider_source_type.resolve_context("Exported"_view);
  ASSERT(shared.is<Library::Language::Types::Structure>());
  ASSERT(exported.is<Ttx::Model::Alias>());
  EXPECT(&exported.resolve() == &Invalid::get_invalid());
  EXPECT(
      &provider_source_type.resolve_context("Hidden"_view) ==
      &Invalid::get_invalid());

  Package::Language::Monograph& target = create_package(arena, package_dialect);
  ASSERT(target.bind_member("Provider"_view, *provider));
  Package::Language::Monograph& context =
      create_package_with_dependency(arena, package_dialect, "Core"_view);
  ASSERT(bind_only_dependency(context, target));
  auto importer = interpret_library(
      arena, library_dialect, context,
      "using Core;\npublic Local : alias = Exported;"_view);
  ASSERT(importer);

  // The importing transaction orders the real provider graph before resolving
  // either imported or authored Alias edges. No consumer may inspect an
  // Alias target to manufacture that ordering.
  ASSERT(importer->link());
  EXPECT(&exported.resolve() == &shared);
  const auto& importer_source = importer->get_source();
  auto imported_types = importer_source.get_types();
  Option<const Abstract&> imported;
  Option<const Abstract&> local;
  for (const Reference<Abstract>& binding : imported_types) {
    if (binding.get().get_name() == "Exported"_view) {
      imported = binding.get();
    }
    if (binding.get().get_name() == "Local"_view) {
      local = binding.get();
    }
    EXPECT(binding.get().get_name() != "Hidden"_view);
  }
  ASSERT(imported);
  ASSERT(local);
  ASSERT(imported->is<Ttx::Model::Alias>());
  EXPECT(&imported->resolve() == &shared);
  EXPECT(&local->resolve() == &shared);
  EXPECT(
      &importer->resolve_context("Exported"_view) == &Invalid::get_invalid());
  const Abstract& imported_identity = *imported;
  ASSERT(importer->link());
  auto repeated_types = importer_source.get_types();
  Bool retained = False;
  for (const Reference<Abstract>& binding : repeated_types) {
    if (binding.get().get_name() == "Exported"_view) {
      EXPECT(&binding.get() == &imported_identity);
      retained = True;
    }
  }
  EXPECT(retained);
}

PERIMORTEM_UNIT_TEST(
    LibraryImports,
    generic_alias_argument_uses_completed_imported_provider) {
  static constexpr View::Bytes provider_source =
      "public Shared : struct { public state value : Bool; }\n"
      "public Exported : alias = Shared;"_view;
  Allocator::Arena arena;
  ImportRegistry registry;
  Library::Dialect library_dialect;
  Package::Dialect package_dialect;
  auto provider =
      interpret_library(arena, library_dialect, registry, provider_source);
  ASSERT(provider);

  Package::Language::Monograph& target = create_package(arena, package_dialect);
  ASSERT(target.bind_member("Provider"_view, *provider));
  Package::Language::Monograph& context =
      create_package_with_dependency(arena, package_dialect, "Core"_view);
  ASSERT(bind_only_dependency(context, target));
  auto importer = interpret_library(
      arena, library_dialect, context,
      "using Core;\npublic SharedView : alias = View[Exported];"_view);
  ASSERT(importer);

  // Provider first closure completion settles Exported before the importer
  // observes its opaque edge as one Generic argument. The importing Alias owns
  // only its local DFS and materializes the canonical View identity.
  ASSERT(importer->link());
  const Abstract& provider_shared =
      provider->get_source().resolve_context("Shared"_view);
  ASSERT(provider_shared.is<Library::Language::Types::Structure>());
  const Abstract& local =
      importer->get_source().resolve_context("SharedView"_view);
  ASSERT(local.is<Ttx::Model::Alias>());
  auto view = local.resolve().select<Library::Language::Types::View>();
  ASSERT(view);
  EXPECT(&view->get_element_type() == &provider_shared);
  EXPECT_EQ(importer->get_materializations().get_size(), Count(1));
  ASSERT(importer->finalize());
}

static constexpr Count temporary_path_capacity = 160;

static auto join_path(View::Bytes root, View::Bytes member) -> Dynamic::Bytes {
  Dynamic::Bytes path(root);
  path.append('/');
  path.concat(member);
  return path;
}

class TemporaryImportPackage {
 public:
  TemporaryImportPackage() {
    Signed_32 written = snprintf(
        Data::cast<char>(root_path.get_data()), root_path.get_size(),
        "/tmp/tetrodotoxin_library_import_XXXXXX");
    if (written <= 0 || Count(written) >= root_path.get_size()) {
      return;
    }

    valid = mkdtemp(Data::cast<char>(root_path.get_data())) != nullptr;
  }

  TemporaryImportPackage(const TemporaryImportPackage&) = delete;
  auto operator=(const TemporaryImportPackage&)
      -> TemporaryImportPackage& = delete;

  ~TemporaryImportPackage() {
    if (!valid) {
      return;
    }

    remove("package.ttx"_view);
    remove("main.ttx"_view);
    remove("nested/package.ttx"_view);
    remove("nested/api.ttx"_view);
    remove("nested"_view);
    File::remove(get_root());
  }

  operator bool() const { return bool(valid); }

  auto get_root() const -> View::Bytes {
    return NullTerminated::to_view(
        Data::cast<const char>(root_path.get_data()));
  }

  auto create_nested() const -> Bool {
    Dynamic::Bytes path = join_path(get_root(), "nested"_view);
    path.append('\0');
    Bool created =
        mkdir(Data::cast<char>(path.get_access().get_data()), S_IRWXU) == 0;
    return created;
  }

  auto write(View::Bytes member, View::Bytes contents) const -> Bool {
    Bool written = File::write(contents, join_path(get_root(), member));
    return written;
  }

 private:
  auto remove(View::Bytes member) const -> void {
    File::remove(join_path(get_root(), member));
  }

  Static::Bytes<temporary_path_capacity> root_path;
  Bool valid = False;
};

PERIMORTEM_UNIT_TEST(
    LibraryImports,
    workspace_links_then_finalizes_complete_staging) {
  TemporaryImportPackage package;
  ASSERT(package);
  Bool nested_created = package.create_nested();
  Bool root_written = package.write(
      "package.ttx"_view,
      "// Root Package\n"
      "dialect : Package;\n"
      "source Core from \"nested/package.ttx\";\n"
      "source Main from \"main.ttx\";\n"_view);
  Bool nested_written = package.write(
      "nested/package.ttx"_view,
      "// Core Package\n"
      "dialect : Package;\n"
      "source Api from \"nested/api.ttx\";\n"_view);
  Bool provider_written = package.write(
      "nested/api.ttx"_view,
      "// Provider Library\n"
      "dialect : Library;\n"
      "public Shared : struct { public state value : Bool; }\n"
      "public Domain : struct {\n"
      "  public Mode : enum[Unsigned_8] { ready = 1; }\n"
      "}\n"
      "public provided : func = [] -> [] {}\n"_view);
  Bool importer_written = package.write(
      "main.ttx"_view,
      "// Importing Library\n"
      "dialect : Library;\n"
      "using Core;\n"
      "private Holder : struct {\n"
      "  private shared : Shared;\n"
      "  private mode : Domain::Mode;\n"
      "}\n"
      "private local : func = [\n"
      "  .value : Shared, .mode : Domain::Mode\n"
      "] -> Shared { return value; }\n"_view);
  ASSERT(nested_created);
  ASSERT(root_written);
  ASSERT(nested_written);
  ASSERT(provider_written);
  ASSERT(importer_written);

  // Main enters Retention before the nested Api source. Success proves
  // Workspace drains the complete staging queue before Library linking and
  // finalization, and that Main retained the root Package as its context.
  Environment::Workspace workspace;
  Bool package_installed =
      workspace.install_dialect<Package::Dialect>("Package"_view);
  Bool library_installed =
      workspace.install_dialect<Library::Dialect>("Library"_view);
  ASSERT(package_installed);
  ASSERT(library_installed);
  Allocator::Arena repository_arena;
  auto repository = Package::Repository::Repository::create(
      repository_arena, View::Vector<Package::Repository::Input>(),
      View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  ASSERT(repository);

  Errors errors;
  auto imported = workspace.import_package(
      errors, package.get_root(), "Root"_view, "package.ttx"_view,
      "Pkg.Root"_view, Version(1, 0), *repository);
  auto root_result = imported.visit(
      [](Language::Monograph& root) { return &root; },
      [](Package::Repository::SelectionError) {
        return static_cast<Language::Monograph*>(nullptr);
      });
  ASSERT(root_result);
  ASSERT(root_result->is<Package::Language::Monograph>());
  const auto& root =
      static_cast<const Package::Language::Monograph&>(*root_result);
  const Abstract& core_abstract = root.resolve_context("Core"_view).resolve();
  const Abstract& main_abstract = root.resolve_context("Main"_view).resolve();
  ASSERT(core_abstract.is<Package::Language::Monograph>());
  ASSERT(main_abstract.is<Library::Language::Monograph>());
  const auto& core =
      static_cast<const Package::Language::Monograph&>(core_abstract);
  const auto& main =
      static_cast<const Library::Language::Monograph&>(main_abstract);
  const Abstract& api_abstract = core.resolve_context("Api"_view).resolve();
  ASSERT(api_abstract.is<Library::Language::Monograph>());
  const auto& api =
      static_cast<const Library::Language::Monograph&>(api_abstract);
  const auto& main_source = main.get_source();
  auto main_types = main_source.get_types();
  auto main_type_iterator = main_types.begin();
  ASSERT(main_type_iterator != main_types.end());
  const Abstract& holder_identity = (*main_type_iterator).get();
  auto main_callables = main_source.get_callables();
  auto main_callable_iterator = main_callables.begin();
  ASSERT(main_callable_iterator != main_callables.end());
  const Abstract& local_identity = (*main_callable_iterator).get();
  ++main_callable_iterator;
  ASSERT(main_callable_iterator != main_callables.end());
  const Abstract& provided_alias = (*main_callable_iterator).get();
  ++main_callable_iterator;
  ASSERT(main_callable_iterator == main_callables.end());
  ASSERT(holder_identity.is<Library::Language::Types::Structure>());
  ASSERT(local_identity.is<Library::Language::Function>());
  ASSERT(provided_alias.is<Ttx::Model::Alias>());
  EXPECT_TEXT(holder_identity.get_name(), "Holder"_view);
  EXPECT_TEXT(local_identity.get_name(), "local"_view);
  EXPECT_TEXT(provided_alias.get_name(), "provided"_view);
  const auto& holder =
      static_cast<const Library::Language::Types::Structure&>(holder_identity);
  const auto& local =
      static_cast<const Library::Language::Function&>(local_identity);
  const auto& api_source = api.get_source();
  const Abstract& shared = api.resolve_context("Shared"_view);
  const Abstract& domain = api.resolve_context("Domain"_view);
  ASSERT(shared.is<Library::Language::Types::Structure>());
  ASSERT(domain.is<Library::Language::Types::Structure>());
  const Abstract& mode = domain.resolve_context("Mode"_view);
  ASSERT(mode.is<Library::Language::Types::Enumeration>());
  auto holder_fields = holder.get_addressables();
  auto holder_field_iterator = holder_fields.begin();
  ASSERT(holder_field_iterator != holder_fields.end());
  const auto& shared_field = static_cast<const Library::Language::Field&>(
      (*holder_field_iterator).get());
  ++holder_field_iterator;
  ASSERT(holder_field_iterator != holder_fields.end());
  const auto& mode_field = static_cast<const Library::Language::Field&>(
      (*holder_field_iterator).get());
  EXPECT(&shared_field.get_type() == &shared);
  EXPECT(&mode_field.get_type() == &mode);
  const Layout& local_parameters = local.get_parameters();
  const Layout& local_results = local.get_results();
  ASSERT_EQ(local_parameters.get_size(), Count(2));
  ASSERT_EQ(local_results.get_size(), Count(1));
  auto value_parameter = local_parameters.get_abstract(0);
  auto mode_parameter = local_parameters.get_abstract(1);
  ASSERT(value_parameter && value_parameter->is<Ttx::Model::Addressable>());
  ASSERT(mode_parameter && mode_parameter->is<Ttx::Model::Addressable>());
  EXPECT(
      &static_cast<const Ttx::Model::Addressable&>(*value_parameter)
           .get_type() == &shared);
  EXPECT(
      &static_cast<const Ttx::Model::Addressable&>(*mode_parameter)
           .get_type() == &mode);
  ASSERT(local_results.get_abstract(0));
  EXPECT(&*local_results.get_abstract(0) == &shared);
  auto api_callables =
      api_source.get_callables(Tetrodotoxin::Language::Visibility::Private);
  auto api_callable_iterator = api_callables.begin();
  ASSERT(api_callable_iterator != api_callables.end());
  EXPECT(&provided_alias.resolve() == &(*api_callable_iterator).get());
  EXPECT(&main.resolve_context("provided"_view) == &Invalid::get_invalid());
  EXPECT(&main.resolve_context("local"_view) == &Invalid::get_invalid());
  EXPECT(&main.resolve_context("source"_view) == &main_source);
  auto main_public =
      main_source.get_callables(Tetrodotoxin::Language::Visibility::Public);
  EXPECT(main_public.begin() == main_public.end());
  EXPECT(errors.is_empty());
}
