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
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
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

class SourceSpoofMonograph : public Tetrodotoxin::Language::Monograph {
 public:
  SourceSpoofMonograph(Allocator::Arena& arena, const Abstract& claimed_source)
      : Monograph(arena, Documentation::get_empty()),
        claimed_source(claimed_source) {}

  auto get_name() const -> View::Bytes override { return "SourceSpoof"_view; }

  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    return route == "source"_view ? claimed_source : Invalid::get_invalid();
  }

 private:
  const Abstract& claimed_source;
};

static auto interpret_library(
    Allocator::Arena& arena,
    Library::Dialect& dialect,
    Abstract& context,
    View::Bytes source) -> Option<Library::Language::Monograph&> {
  Errors errors;
  Tokenizer tokenizer(arena, source, "library-import.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto interpreted =
      dialect.interpret(arena, cursor, Documentation::get_empty(), context);
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
  static constexpr Static::Vector<View::Bytes, 2> routes = {{
    "Core"_view,
    "Runtime::Core"_view,
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
    EXPECT_TEXT(import->get_route(), routes[i]);
    EXPECT(cursor.matches(Code::Type::Terminal));
    EXPECT(errors.is_empty());
  }

  static constexpr Static::Vector<View::Bytes, 9> rejected = {{
    "Using Core;"_view,
    "use Core;"_view,
    "using core;"_view,
    "using Runtime ::Core;"_view,
    "using Runtime:: Core;"_view,
    "using;"_view,
    "using Core Other;"_view,
    "using Core"_view,
    "using Core trailing;"_view,
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
      "public func first[] -> Void {}\n"
      "private func hidden[] -> Void {}"_view;
  static constexpr View::Bytes second_source =
      "public func second[] -> Void {}"_view;
  static constexpr View::Bytes dependency_source =
      "public func dependency_only[] -> Void {}"_view;
  static constexpr View::Bytes importer_source =
      "private func local_private[] -> Void {}\n"
      "using Runtime::Core;\n"
      "public func local_public[] -> Void {}"_view;

  Allocator::Arena arena;
  ImportRegistry registry;
  Library::Dialect library_dialect(registry);
  Package::Dialect package_dialect(registry);
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

  Package::Language::Monograph& source_package = create_package_with_dependency(
      arena, package_dialect, "Runtime::Core"_view);
  Bool source_dependency_bound =
      bind_only_dependency(source_package, target_package);
  ASSERT(source_dependency_bound);
  auto importer = interpret_library(
      arena, library_dialect, source_package, importer_source);
  ASSERT(importer);

  const Abstract& first_identity = first->resolve_context("first"_view);
  const Abstract& second_identity = second->resolve_context("second"_view);
  const Abstract& public_local = importer->resolve_context("local_public"_view);
  ASSERT(&first_identity != &Invalid::get_invalid());
  ASSERT(&second_identity != &Invalid::get_invalid());
  ASSERT(&public_local != &Invalid::get_invalid());
  EXPECT(
      &importer->resolve_context("local_private"_view) ==
      &Invalid::get_invalid());
  ASSERT(importer->get_source().is<Library::Language::Types::Structure>());
  const auto& importer_scope =
      static_cast<const Library::Language::Types::Structure&>(
          importer->get_source());
  auto authored = importer->get_authored_bindings();
  ASSERT_EQ(authored.get_size(), Count(2));
  ASSERT(authored.get_data()[0].get().is<Library::Language::Function>());
  ASSERT(authored.get_data()[1].get().is<Library::Language::Function>());
  const auto& private_local = static_cast<const Library::Language::Function&>(
      authored.get_data()[0].get());
  const auto& local_host = static_cast<const Library::Language::Function&>(
      authored.get_data()[1].get());
  ASSERT_EQ(importer_scope.get_external_static_bindings().get_size(), Count(1));
  EXPECT(
      &importer_scope.get_external_static_bindings().get_data()[0].get() ==
      &public_local);

  Bool import_completed = importer->link();
  ASSERT(import_completed);
  const Abstract& first_alias = local_host.resolve_context("first"_view);
  const Abstract& second_alias = local_host.resolve_context("second"_view);
  ASSERT(first_alias.is<Ttx::Model::Alias>());
  ASSERT(second_alias.is<Ttx::Model::Alias>());
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
  EXPECT(&local_host.resolve_context("hidden"_view) == &Invalid::get_invalid());
  EXPECT(
      &local_host.resolve_context("dependency_only"_view) ==
      &Invalid::get_invalid());
  EXPECT(&local_host.resolve_context("Nested"_view) == &Invalid::get_invalid());
  EXPECT(&importer->resolve_context("local_public"_view) == &public_local);
  EXPECT(&local_host.resolve_context("local_private"_view) == &private_local);
  EXPECT(
      &local_host.resolve_context("Bool"_view) ==
      &library_dialect.resolve_intrinsic("Bool"_view));
  EXPECT(&importer->resolve_context("Bool"_view) == &Invalid::get_invalid());
  EXPECT_EQ(importer_scope.get_static_bindings().get_size(), Count(4));
  EXPECT_EQ(importer_scope.get_external_static_bindings().get_size(), Count(1));
  EXPECT(
      &importer_scope.get_external_static_bindings().get_data()[0].get() ==
      &public_local);

  const Abstract* first_alias_identity = &first_alias;
  ASSERT(importer->link());
  EXPECT(&local_host.resolve_context("first"_view) == first_alias_identity);
  EXPECT_EQ(importer_scope.get_static_bindings().get_size(), Count(4));
}

PERIMORTEM_UNIT_TEST(LibraryImports, provider_import_is_not_reexported) {
  Allocator::Arena arena;
  ImportRegistry registry;
  Library::Dialect library_dialect(registry);
  Package::Dialect package_dialect(registry);
  auto upstream = interpret_library(
      arena, library_dialect, registry,
      "public func upstream[] -> Void {}"_view);
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
      "using Upstream;\npublic func direct[] -> Void {}"_view);
  ASSERT(provider);
  Bool provider_completed = provider->link();
  ASSERT(provider_completed);
  auto provider_bindings = provider->get_authored_bindings();
  ASSERT_EQ(provider_bindings.get_size(), Count(1));
  ASSERT(
      provider_bindings.get_data()[0].get().is<Library::Language::Function>());
  const auto& provider_host = static_cast<const Library::Language::Function&>(
      provider_bindings.get_data()[0].get());
  const Abstract& upstream_alias =
      provider_host.resolve_context("upstream"_view);
  ASSERT(upstream_alias.is<Ttx::Model::Alias>());
  EXPECT(
      &upstream_alias.resolve() == &upstream->resolve_context("upstream"_view));
  EXPECT(
      &provider->resolve_context("upstream"_view) == &Invalid::get_invalid());
  ASSERT(provider->get_source().is<Library::Language::Types::Structure>());
  const auto& provider_source =
      static_cast<const Library::Language::Types::Structure&>(
          provider->get_source());
  ASSERT_EQ(
      provider_source.get_external_static_bindings().get_size(), Count(1));

  // The provider can use its upstream Function locally, but its public view
  // retains only its authored declaration. A downstream Import consumes that
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
      "using Provider;\nprivate func consumer[] -> Void {}"_view);
  ASSERT(importer);
  Bool importer_completed = importer->link();
  ASSERT(importer_completed);

  auto importer_bindings = importer->get_authored_bindings();
  ASSERT_EQ(importer_bindings.get_size(), Count(1));
  ASSERT(
      importer_bindings.get_data()[0].get().is<Library::Language::Function>());
  const auto& importer_host = static_cast<const Library::Language::Function&>(
      importer_bindings.get_data()[0].get());
  const Abstract& direct_alias = importer_host.resolve_context("direct"_view);
  ASSERT(direct_alias.is<Ttx::Model::Alias>());
  EXPECT(&direct_alias.resolve() == &provider->resolve_context("direct"_view));
  EXPECT(&importer->resolve_context("direct"_view) == &Invalid::get_invalid());
  EXPECT(
      &importer_host.resolve_context("upstream"_view) ==
      &Invalid::get_invalid());
  ASSERT(importer->get_source().is<Library::Language::Types::Structure>());
  const auto& importer_source =
      static_cast<const Library::Language::Types::Structure&>(
          importer->get_source());
  EXPECT(importer_source.get_external_static_bindings().is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryImports, collisions_are_atomic) {
  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect(registry);
    Package::Dialect package_dialect(registry);
    auto unique = interpret_library(
        arena, library_dialect, registry,
        "public func unique[] -> Void {}"_view);
    auto colliding = interpret_library(
        arena, library_dialect, registry,
        "public func clash[] -> Void {}"_view);
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
        "using Core;\npublic func clash[] -> Void {}"_view);
    ASSERT(importer);
    const Abstract& local = importer->resolve_context("clash"_view);
    Bool completed = importer->link();
    ASSERT_NOT(completed);
    EXPECT(
        &importer->resolve_context("unique"_view) == &Invalid::get_invalid());
    EXPECT(&importer->resolve_context("clash"_view) == &local);
    ASSERT_EQ(importer->get_diagnostics().get_size(), Count(1));
    EXPECT(diagnostic_matches(
        *importer, 0, "using Core;\npublic func clash[] -> Void {}"_view,
        "using Core;"_view,
        "Imported Static binding collides with an occupied source name."_view));
  }

  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect(registry);
    Package::Dialect package_dialect(registry);
    auto first = interpret_library(
        arena, library_dialect, registry,
        "public func unique[] -> Void {}\n"
        "public func repeated[] -> Void {}"_view);
    auto second = interpret_library(
        arena, library_dialect, registry,
        "public func repeated[] -> Void {}"_view);
    ASSERT(first && second);

    // Both providers stage before publication. Their shared Import occurrence
    // owns the collision diagnostic while failure keeps both Functions absent.
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
    ASSERT_EQ(importer->get_diagnostics().get_size(), Count(1));
    EXPECT(diagnostic_matches(
        *importer, 0, "using Core;"_view, "using Core;"_view,
        "Two Library Imports publish the same Static binding name."_view));
  }

  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect(registry);
    Package::Dialect package_dialect(registry);
    auto provider = interpret_library(
        arena, library_dialect, registry, "public func only[] -> Void {}"_view);
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
    ASSERT_EQ(importer->get_diagnostics().get_size(), Count(1));
    EXPECT(diagnostic_matches(
        *importer, 0, "using Core;\nusing Core;"_view, "using Core;"_view,
        "Library source repeats one exact Import route."_view));
  }

  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect(registry);
    Package::Dialect package_dialect(registry);
    auto provider = interpret_library(
        arena, library_dialect, registry,
        "public func occupied[] -> Void {}"_view);
    ASSERT(provider);

    Package::Language::Monograph& target =
        create_package(arena, package_dialect);
    Bool provider_bound = target.bind_member("Provider"_view, *provider);
    ASSERT(provider_bound);
    Package::Language::Monograph& context =
        create_package_with_dependency(arena, package_dialect, "Core"_view);
    Bool dependency_bound = bind_only_dependency(context, target);
    Bool occupied_bound = context.bind_member("occupied"_view, *provider);
    ASSERT(dependency_bound);
    ASSERT(occupied_bound);

    // The Package context occupies this name for internal binding checks. It
    // does not leak through the external Monograph view on either outcome.
    auto importer =
        interpret_library(arena, library_dialect, context, "using Core;"_view);
    ASSERT(importer);
    EXPECT(
        &importer->resolve_context("occupied"_view) == &Invalid::get_invalid());
    Bool completed = importer->link();
    ASSERT_NOT(completed);
    EXPECT(
        &importer->resolve_context("occupied"_view) == &Invalid::get_invalid());
    ASSERT_EQ(importer->get_diagnostics().get_size(), Count(1));
    EXPECT(diagnostic_matches(
        *importer, 0, "using Core;"_view, "using Core;"_view,
        "Imported Static binding collides with an occupied source name."_view));
  }
}

PERIMORTEM_UNIT_TEST(LibraryImports, invalid_targets_are_atomic) {
  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect(registry);
    auto importer = interpret_library(
        arena, library_dialect, registry,
        "using Core;\npublic func local[] -> Void {}"_view);
    ASSERT(importer);
    const Abstract& local = importer->resolve_context("local"_view);

    // An Import needs its source Package even when its route could miss in any
    // Abstract. Rejection leaves the local declaration as the only lookup edge.
    Bool completed = importer->link();
    ASSERT_NOT(completed);
    EXPECT(&importer->resolve_context("local"_view) == &local);
    ASSERT_EQ(importer->get_diagnostics().get_size(), Count(1));
    EXPECT(diagnostic_matches(
        *importer, 0, "using Core;\npublic func local[] -> Void {}"_view,
        "using Core;"_view,
        "Library Import source context is not a Package Monograph."_view));
  }

  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect(registry);
    Package::Dialect package_dialect(registry);
    auto provider = interpret_library(
        arena, library_dialect, registry,
        "public func staged[] -> Void {}"_view);
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
    EXPECT(
        &importer->resolve_context("staged"_view) == &Invalid::get_invalid());
    ASSERT_EQ(importer->get_diagnostics().get_size(), Count(1));
    EXPECT(diagnostic_matches(
        *importer, 0, "using Core;\nusing Missing;"_view, "using Missing;"_view,
        "Library Import route did not resolve to a Package Monograph."_view));
  }

  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect(registry);
    Package::Dialect package_dialect(registry);
    auto provider = interpret_library(
        arena, library_dialect, registry,
        "public func staged[] -> Void {}"_view);
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
    EXPECT(
        &importer->resolve_context("staged"_view) == &Invalid::get_invalid());
    ASSERT_EQ(importer->get_diagnostics().get_size(), Count(1));
    EXPECT(diagnostic_matches(
        *importer, 0, "using Core;\nusing Direct;"_view, "using Direct;"_view,
        "Library Import route did not resolve to a Package Monograph."_view));
  }

  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect(registry);
    Package::Dialect package_dialect(registry);
    auto complete = interpret_library(
        arena, library_dialect, registry,
        "public func staged[] -> Void {}"_view);
    ASSERT(complete);
    Library::Dialect incomplete_dialect(registry);
    Library::Language::Materializations materializations(arena);
    auto& incomplete = Library::Language::Monograph::create_authored(
        arena, Documentation::get_empty(), incomplete_dialect, registry,
        materializations);
    Errors errors;
    Tokenizer tokenizer(
        arena, "public func incomplete[] -> Void {}"_view,
        "incomplete-provider.ttx"_view);
    Cursor cursor(tokenizer, errors);
    auto function = Library::Language::Function::reserve(
        arena, cursor, Documentation::get_empty(), incomplete,
        incomplete.get_source(), materializations);
    ASSERT(function);
    Bool incomplete_bound =
        incomplete.bind_static(*function, function->get_visibility());
    ASSERT(incomplete_bound);

    Package::Language::Monograph& target =
        create_package(arena, package_dialect);
    Bool complete_member_bound = target.bind_member("Complete"_view, *complete);
    Bool incomplete_member_bound =
        target.bind_member("Incomplete"_view, incomplete);
    ASSERT(complete_member_bound);
    ASSERT(incomplete_member_bound);
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
        &importer->resolve_context("staged"_view) == &Invalid::get_invalid());
    EXPECT(
        &importer->resolve_context("incomplete"_view) ==
        &Invalid::get_invalid());
    ASSERT_EQ(importer->get_diagnostics().get_size(), Count(1));
    EXPECT(diagnostic_matches(
        *importer, 0, "using Core;"_view, "using Core;"_view,
        "Library Import exposes an incomplete Static binding."_view));
  }
}

PERIMORTEM_UNIT_TEST(LibraryImports, source_spoof_is_rejected) {
  static constexpr View::Bytes source =
      "using Core;\npublic func local[] -> Void {}"_view;
  Allocator::Arena arena;
  ImportRegistry registry;
  Library::Dialect library_dialect(registry);
  Package::Dialect package_dialect(registry);
  auto library = interpret_library(
      arena, library_dialect, registry,
      "public func provider[] -> Void {}"_view);
  ASSERT(library);
  SourceSpoofMonograph spoof(arena, library->get_source());

  Package::Language::Monograph& target = create_package(arena, package_dialect);
  ASSERT(target.bind_member("Spoof"_view, spoof));
  Package::Language::Monograph& context =
      create_package_with_dependency(arena, package_dialect, "Core"_view);
  ASSERT(bind_only_dependency(context, target));
  auto importer = interpret_library(arena, library_dialect, context, source);
  ASSERT(importer);

  const Abstract& local = importer->resolve_context("local"_view);
  ASSERT(&local != &Invalid::get_invalid());
  ASSERT_NOT(importer->link());
  EXPECT(&importer->resolve_context("local"_view) == &local);
  ASSERT(importer->get_source().is<Library::Language::Types::Structure>());
  const auto& importer_source =
      static_cast<const Library::Language::Types::Structure&>(
          importer->get_source());
  EXPECT_EQ(importer_source.get_static_bindings().get_size(), Count(1));
  ASSERT_EQ(importer->get_diagnostics().get_size(), Count(1));
  EXPECT(diagnostic_matches(
      *importer, 0, source, "using Core;"_view,
      "Non Library Package member claimed a Library source Structure."_view));
}

PERIMORTEM_UNIT_TEST(LibraryImports, retry_preserves_local_alias) {
  static constexpr View::Bytes source =
      "using Core;\nprivate func consumer[] -> Void {}"_view;
  Allocator::Arena arena;
  ImportRegistry registry;
  Library::Dialect library_dialect(registry);
  Package::Dialect package_dialect(registry);
  auto provider = interpret_library(
      arena, library_dialect, registry, "public func ready[] -> Void {}"_view);
  ASSERT(provider);
  ASSERT(provider->link());

  Package::Language::Monograph& target = create_package(arena, package_dialect);
  ASSERT(target.bind_member("Provider"_view, *provider));
  Package::Language::Monograph& context =
      create_package_with_dependency(arena, package_dialect, "Core"_view);
  auto importer = interpret_library(arena, library_dialect, context, source);
  ASSERT(importer);
  auto authored = importer->get_authored_bindings();
  ASSERT_EQ(authored.get_size(), Count(1));
  ASSERT(authored.get_data()[0].get().is<Library::Language::Function>());
  const auto& consumer = static_cast<const Library::Language::Function&>(
      authored.get_data()[0].get());

  // The missing dependency fails before the source signature barrier, leaving
  // the import transaction open for the exact Package edge to arrive later.
  ASSERT_NOT(importer->link());
  EXPECT(&consumer.resolve_context("ready"_view) == &Invalid::get_invalid());
  ASSERT(bind_only_dependency(context, target));
  ASSERT(importer->link());
  const Abstract& alias = consumer.resolve_context("ready"_view);
  ASSERT(alias.is<Ttx::Model::Alias>());
  EXPECT(&alias.resolve() == &provider->resolve_context("ready"_view));
  const Abstract* alias_identity = &alias;
  ASSERT(importer->link());
  EXPECT(&consumer.resolve_context("ready"_view) == alias_identity);
  ASSERT(importer->get_source().is<Library::Language::Types::Structure>());
  const auto& importer_source =
      static_cast<const Library::Language::Types::Structure&>(
          importer->get_source());
  EXPECT_EQ(importer_source.get_static_bindings().get_size(), Count(2));
  Count import_size = importer->get_imports().get_size();
  Library::Language::Import late("Late"_view);
  EXPECT_NOT(importer->retain_import(late));
  EXPECT_EQ(importer->get_imports().get_size(), import_size);
}

PERIMORTEM_UNIT_TEST(LibraryImports, private_type_alias_cannot_escape) {
  static constexpr View::Bytes source =
      "using Core;\npublic func publish[Shared] -> Void {}"_view;
  Allocator::Arena arena;
  ImportRegistry registry;
  Library::Dialect library_dialect(registry);
  Package::Dialect package_dialect(registry);
  auto provider = interpret_library(
      arena, library_dialect, registry, "public Shared : struct {}"_view);
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
  auto authored = importer->get_authored_bindings();
  ASSERT_EQ(authored.get_size(), Count(1));
  ASSERT(authored.get_data()[0].get().is<Library::Language::Function>());
  const auto& publish = static_cast<const Library::Language::Function&>(
      authored.get_data()[0].get());
  auto signature = publish.get_signature();
  ASSERT(signature);
  auto parameter_type = signature->get_parameter_type(0);
  ASSERT(parameter_type);
  EXPECT(&*parameter_type == &shared);
  EXPECT(&importer->resolve_context("Shared"_view) == &Invalid::get_invalid());
  ASSERT_NOT(importer->finalize());
  auto diagnostics = importer->get_diagnostics();
  ASSERT_EQ(diagnostics.get_size(), Count(1));
  ASSERT(diagnostics.get_data()[0].get_anchor());
  EXPECT_TEXT(
      diagnostics.get_data()[0].get_anchor()->get_span().caculate_text(source),
      "Shared"_view);
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
      "public func provided[] -> Void {}\n"_view);
  Bool importer_written = package.write(
      "main.ttx"_view,
      "// Importing Library\n"
      "dialect : Library;\n"
      "using Core;\n"
      "public func local[] -> Void {}\n"_view);
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

  auto main_bindings = main.get_authored_bindings();
  ASSERT_EQ(main_bindings.get_size(), Count(1));
  ASSERT(main_bindings.get_data()[0].get().is<Library::Language::Function>());
  const auto& local = static_cast<const Library::Language::Function&>(
      main_bindings.get_data()[0].get());
  const Abstract& provided_alias = local.resolve_context("provided"_view);
  ASSERT(provided_alias.is<Ttx::Model::Alias>());
  EXPECT(&provided_alias.resolve() == &api.resolve_context("provided"_view));
  EXPECT(&main.resolve_context("provided"_view) == &Invalid::get_invalid());
  EXPECT(&main.resolve_context("local"_view) != &Invalid::get_invalid());
  ASSERT(main.get_source().is<Library::Language::Types::Structure>());
  const auto& main_source =
      static_cast<const Library::Language::Types::Structure&>(
          main.get_source());
  EXPECT(&main.resolve_context("source"_view) == &main_source);
  ASSERT_EQ(main_source.get_external_static_bindings().get_size(), Count(1));
  EXPECT_TEXT(
      main_source.get_external_static_bindings().get_data()[0].get().get_name(),
      "local"_view);
  EXPECT(errors.is_empty());
}
