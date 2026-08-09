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
#include "tetrodotoxin/library/language/types/enumeration.hpp"
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

static auto select_binding(
    View::Vector<Reference<const Abstract>> bindings,
    View::Bytes name,
    Count occurrence = 0) -> const Abstract& {
  for (Count i = 0; i < bindings.get_size(); i++) {
    const Abstract& binding = bindings.get_data()[i].get();
    if (binding.get_name() != name) {
      continue;
    }
    if (occurrence == 0) {
      return binding;
    }

    occurrence--;
  }

  return Invalid::get_invalid();
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

  ASSERT(first->get_source().is<Library::Language::Types::Structure>());
  ASSERT(second->get_source().is<Library::Language::Types::Structure>());
  ASSERT(importer->get_source().is<Library::Language::Types::Structure>());
  const auto& first_scope =
      static_cast<const Library::Language::Types::Structure&>(
          first->get_source());
  const auto& second_scope =
      static_cast<const Library::Language::Types::Structure&>(
          second->get_source());
  const auto& importer_scope =
      static_cast<const Library::Language::Types::Structure&>(
          importer->get_source());
  const Abstract& first_identity =
      select_binding(first_scope.get_callable_bindings(), "first"_view);
  const Abstract& second_identity =
      select_binding(second_scope.get_callable_bindings(), "second"_view);
  ASSERT(&first_identity != &Invalid::get_invalid());
  ASSERT(&second_identity != &Invalid::get_invalid());
  auto authored = importer->get_authored_bindings();
  ASSERT_EQ(authored.get_size(), Count(2));
  ASSERT(authored.get_data()[0].get().is<Library::Language::Function>());
  ASSERT(authored.get_data()[1].get().is<Library::Language::Function>());
  const auto& private_local = static_cast<const Library::Language::Function&>(
      authored.get_data()[0].get());
  const auto& local_host = static_cast<const Library::Language::Function&>(
      authored.get_data()[1].get());
  const Abstract& public_local = local_host;
  EXPECT(
      &importer->resolve_context("local_public"_view) ==
      &Invalid::get_invalid());
  EXPECT(
      &importer->resolve_context("local_private"_view) ==
      &Invalid::get_invalid());
  ASSERT_EQ(importer_scope.get_callable_bindings().get_size(), Count(1));
  EXPECT(
      &importer_scope.get_callable_bindings().get_data()[0].get() ==
      &public_local);

  Bool import_completed = importer->link();
  ASSERT(import_completed);
  auto local_candidates = importer_scope.get_callable_bindings(local_host);
  const Abstract& first_alias = select_binding(local_candidates, "first"_view);
  const Abstract& second_alias =
      select_binding(local_candidates, "second"_view);
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
      &select_binding(local_candidates, "local_private"_view) ==
      &private_local);
  EXPECT(
      &local_host.resolve_context("Bool"_view) ==
      &library_dialect.resolve_intrinsic("Bool"_view));
  EXPECT(&importer->resolve_context("Bool"_view) == &Invalid::get_invalid());
  EXPECT_EQ(importer_scope.get_static_bindings().get_size(), Count(4));
  EXPECT_EQ(importer_scope.get_callable_bindings().get_size(), Count(1));
  EXPECT(
      &importer_scope.get_callable_bindings().get_data()[0].get() ==
      &public_local);

  const Abstract& first_alias_identity = first_alias;
  ASSERT(importer->link());
  auto repeated_candidates = importer_scope.get_callable_bindings(local_host);
  EXPECT(
      &select_binding(repeated_candidates, "first"_view) ==
      &first_alias_identity);
  EXPECT_EQ(importer_scope.get_static_bindings().get_size(), Count(4));
}

PERIMORTEM_UNIT_TEST(LibraryImports, provider_import_is_not_reexported) {
  Allocator::Arena arena;
  ImportRegistry registry;
  Library::Dialect library_dialect;
  Package::Dialect package_dialect;
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
  EXPECT(
      &provider->resolve_context("upstream"_view) == &Invalid::get_invalid());
  EXPECT(
      &provider_host.resolve_context("upstream"_view) ==
      &Invalid::get_invalid());
  ASSERT(provider->get_source().is<Library::Language::Types::Structure>());
  const auto& provider_source =
      static_cast<const Library::Language::Types::Structure&>(
          provider->get_source());
  const Abstract& upstream_alias = select_binding(
      provider_source.get_callable_bindings(provider_host), "upstream"_view);
  ASSERT(upstream_alias.is<Ttx::Model::Alias>());
  ASSERT(upstream->get_source().is<Library::Language::Types::Structure>());
  const auto& upstream_source =
      static_cast<const Library::Language::Types::Structure&>(
          upstream->get_source());
  const Abstract& upstream_identity =
      select_binding(upstream_source.get_callable_bindings(), "upstream"_view);
  EXPECT(&upstream_alias.resolve() == &upstream_identity);
  ASSERT_EQ(provider_source.get_callable_bindings().get_size(), Count(1));

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
  EXPECT(&importer->resolve_context("direct"_view) == &Invalid::get_invalid());
  EXPECT(
      &importer_host.resolve_context("direct"_view) == &Invalid::get_invalid());
  ASSERT(importer->get_source().is<Library::Language::Types::Structure>());
  const auto& importer_source =
      static_cast<const Library::Language::Types::Structure&>(
          importer->get_source());
  auto importer_candidates =
      importer_source.get_callable_bindings(importer_host);
  const Abstract& direct_alias =
      select_binding(importer_candidates, "direct"_view);
  ASSERT(direct_alias.is<Ttx::Model::Alias>());
  const Abstract& direct_identity =
      select_binding(provider_source.get_callable_bindings(), "direct"_view);
  EXPECT(&direct_alias.resolve() == &direct_identity);
  EXPECT(
      &select_binding(importer_candidates, "upstream"_view) ==
      &Invalid::get_invalid());
  EXPECT(importer_source.get_callable_bindings().is_empty());
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
        "Imported Static binding collides with an occupied source name."_view));
  }

  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect;
    Package::Dialect package_dialect;
    auto first = interpret_library(
        arena, library_dialect, registry,
        "public func unique[] -> Void {}\n"
        "public func repeated[] -> Void {}"_view);
    auto second = interpret_library(
        arena, library_dialect, registry,
        "public func repeated[] -> Void {}"_view);
    ASSERT(first && second);
    ASSERT(first->link());
    ASSERT(second->link());

    // Both providers contribute exact Alias identities. A repeated Callable
    // name remains an ordered candidate set for the future invocation owner.
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
    ASSERT(completed);
    EXPECT(
        &importer->resolve_context("unique"_view) == &Invalid::get_invalid());
    EXPECT(
        &importer->resolve_context("repeated"_view) == &Invalid::get_invalid());
    ASSERT(importer->get_source().is<Library::Language::Types::Structure>());
    const auto& importer_source =
        static_cast<const Library::Language::Types::Structure&>(
            importer->get_source());
    auto candidates = importer_source.get_callable_bindings(*importer);
    ASSERT_EQ(candidates.get_size(), Count(3));
    const Abstract& unique_alias = candidates.get_data()[0].get();
    const Abstract& first_repeated_alias = candidates.get_data()[1].get();
    const Abstract& second_repeated_alias = candidates.get_data()[2].get();
    ASSERT(unique_alias.is<Ttx::Model::Alias>());
    ASSERT(first_repeated_alias.is<Ttx::Model::Alias>());
    ASSERT(second_repeated_alias.is<Ttx::Model::Alias>());
    EXPECT_TEXT(unique_alias.get_name(), "unique"_view);
    EXPECT_TEXT(first_repeated_alias.get_name(), "repeated"_view);
    EXPECT_TEXT(second_repeated_alias.get_name(), "repeated"_view);
    ASSERT(first->get_source().is<Library::Language::Types::Structure>());
    ASSERT(second->get_source().is<Library::Language::Types::Structure>());
    const auto& first_source =
        static_cast<const Library::Language::Types::Structure&>(
            first->get_source());
    const auto& second_source =
        static_cast<const Library::Language::Types::Structure&>(
            second->get_source());
    EXPECT(
        &unique_alias.resolve() ==
        &select_binding(first_source.get_callable_bindings(), "unique"_view));
    EXPECT(
        &first_repeated_alias.resolve() ==
        &select_binding(first_source.get_callable_bindings(), "repeated"_view));
    EXPECT(
        &second_repeated_alias.resolve() ==
        &select_binding(
            second_source.get_callable_bindings(), "repeated"_view));
    EXPECT(importer->get_diagnostics().is_empty());
  }

  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect;
    Package::Dialect package_dialect;
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
    ASSERT(importer->get_source().is<Library::Language::Types::Structure>());
    const auto& importer_source =
        static_cast<const Library::Language::Types::Structure&>(
            importer->get_source());
    EXPECT(importer_source.get_callable_bindings(*importer).is_empty());
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
        "Imported Static binding collides with an occupied source name."_view));
  }
}

PERIMORTEM_UNIT_TEST(LibraryImports, invalid_targets_are_atomic) {
  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect;
    auto importer = interpret_library(
        arena, library_dialect, registry,
        "using Core;\npublic func local[] -> Void {}"_view);
    ASSERT(importer);
    auto authored = importer->get_authored_bindings();
    ASSERT_EQ(authored.get_size(), Count(1));
    const Abstract& local = authored.get_data()[0].get();
    ASSERT(importer->get_source().is<Library::Language::Types::Structure>());
    const auto& importer_source =
        static_cast<const Library::Language::Types::Structure&>(
            importer->get_source());

    // An Import needs its source Package even when its route could miss in any
    // Abstract. Rejection leaves the local declaration as the only candidate.
    Bool completed = importer->link();
    ASSERT_NOT(completed);
    EXPECT(&importer->resolve_context("local"_view) == &Invalid::get_invalid());
    auto candidates = importer_source.get_callable_bindings(*importer);
    ASSERT_EQ(candidates.get_size(), Count(1));
    EXPECT(&candidates.get_data()[0].get() == &local);
    ASSERT_EQ(importer->get_diagnostics().get_size(), Count(1));
    EXPECT(diagnostic_matches(
        *importer, 0, "using Core;\npublic func local[] -> Void {}"_view,
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
    ASSERT(importer->get_source().is<Library::Language::Types::Structure>());
    const auto& importer_source =
        static_cast<const Library::Language::Types::Structure&>(
            importer->get_source());
    EXPECT(importer_source.get_callable_bindings(*importer).is_empty());
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
    ASSERT(importer->get_source().is<Library::Language::Types::Structure>());
    const auto& importer_source =
        static_cast<const Library::Language::Types::Structure&>(
            importer->get_source());
    EXPECT(importer_source.get_callable_bindings(*importer).is_empty());
    ASSERT_EQ(importer->get_diagnostics().get_size(), Count(1));
    EXPECT(diagnostic_matches(
        *importer, 0, "using Core;\nusing Direct;"_view, "using Direct;"_view,
        "Library Import route did not resolve to a Package Monograph."_view));
  }

  {
    Allocator::Arena arena;
    ImportRegistry registry;
    Library::Dialect library_dialect;
    Package::Dialect package_dialect;
    auto complete = interpret_library(
        arena, library_dialect, registry,
        "public func staged[] -> Void {}"_view);
    ASSERT(complete);
    Library::Dialect incomplete_dialect;
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
    ASSERT(importer->get_source().is<Library::Language::Types::Structure>());
    const auto& importer_source =
        static_cast<const Library::Language::Types::Structure&>(
            importer->get_source());
    EXPECT(importer_source.get_callable_bindings(*importer).is_empty());
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
  Library::Dialect library_dialect;
  Package::Dialect package_dialect;
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

  auto authored = importer->get_authored_bindings();
  ASSERT_EQ(authored.get_size(), Count(1));
  const Abstract& local = authored.get_data()[0].get();
  ASSERT_NOT(importer->link());
  EXPECT(&importer->resolve_context("local"_view) == &Invalid::get_invalid());
  ASSERT(importer->get_source().is<Library::Language::Types::Structure>());
  const auto& importer_source =
      static_cast<const Library::Language::Types::Structure&>(
          importer->get_source());
  EXPECT_EQ(importer_source.get_static_bindings().get_size(), Count(1));
  auto candidates = importer_source.get_callable_bindings(*importer);
  ASSERT_EQ(candidates.get_size(), Count(1));
  EXPECT(&candidates.get_data()[0].get() == &local);
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
  Library::Dialect library_dialect;
  Package::Dialect package_dialect;
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
  ASSERT(importer->get_source().is<Library::Language::Types::Structure>());
  const auto& importer_source =
      static_cast<const Library::Language::Types::Structure&>(
          importer->get_source());
  EXPECT(
      &select_binding(
          importer_source.get_callable_bindings(consumer), "ready"_view) ==
      &Invalid::get_invalid());
  ASSERT(bind_only_dependency(context, target));
  ASSERT(importer->link());
  const Abstract& alias = select_binding(
      importer_source.get_callable_bindings(consumer), "ready"_view);
  ASSERT(alias.is<Ttx::Model::Alias>());
  ASSERT(provider->get_source().is<Library::Language::Types::Structure>());
  const auto& provider_source =
      static_cast<const Library::Language::Types::Structure&>(
          provider->get_source());
  EXPECT(
      &alias.resolve() ==
      &select_binding(provider_source.get_callable_bindings(), "ready"_view));
  const Abstract& alias_identity = alias;
  ASSERT(importer->link());
  EXPECT(
      &select_binding(
          importer_source.get_callable_bindings(consumer), "ready"_view) ==
      &alias_identity);
  EXPECT_EQ(importer_source.get_static_bindings().get_size(), Count(2));
  Count import_size = importer->get_imports().get_size();
  static constexpr View::Bytes late_source = "using Late;"_view;
  Errors errors;
  Tokenizer tokenizer(arena, late_source, "late-import.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto late = Library::Language::Import::parse(cursor);
  ASSERT(late);
  EXPECT_NOT(importer->retain_import(*late));
  EXPECT_EQ(importer->get_imports().get_size(), import_size);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(LibraryImports, private_type_alias_cannot_escape) {
  static constexpr View::Bytes source =
      "using Core;\npublic func publish[Shared] -> Void {}"_view;
  Allocator::Arena arena;
  ImportRegistry registry;
  Library::Dialect library_dialect;
  Package::Dialect package_dialect;
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

PERIMORTEM_UNIT_TEST(
    LibraryImports,
    reachable_closure_links_transitively_and_ignores_unrelated) {
  Allocator::Arena arena;
  ImportRegistry registry;
  Library::Dialect library_dialect;
  Package::Dialect package_dialect;
  auto leaf = interpret_library(
      arena, library_dialect, registry, "public func leaf[] -> Void {}"_view);
  auto unrelated = interpret_library(
      arena, library_dialect, registry,
      "public func unrelated[] -> Void {}"_view);
  ASSERT(leaf && unrelated);

  Package::Language::Monograph& leaf_target =
      create_package(arena, package_dialect);
  ASSERT(leaf_target.bind_member("LeafApi"_view, *leaf));
  Package::Language::Monograph& middle_context =
      create_package_with_dependency(arena, package_dialect, "Leaf"_view);
  ASSERT(bind_only_dependency(middle_context, leaf_target));
  auto middle = interpret_library(
      arena, library_dialect, middle_context,
      "using Leaf;\npublic func middle[] -> Void {}"_view);
  ASSERT(middle);

  Package::Language::Monograph& middle_target =
      create_package(arena, package_dialect);
  ASSERT(middle_target.bind_member("MiddleApi"_view, *middle));
  Package::Language::Monograph& root_context =
      create_package_with_dependency(arena, package_dialect, "Middle"_view);
  ASSERT(bind_only_dependency(root_context, middle_target));
  auto root = interpret_library(
      arena, library_dialect, root_context,
      "using Middle;\npublic func root[] -> Void {}"_view);
  ASSERT(root);

  const auto& leaf_function = static_cast<const Library::Language::Function&>(
      leaf->get_authored_bindings().get_data()[0].get());
  const auto& middle_function = static_cast<const Library::Language::Function&>(
      middle->get_authored_bindings().get_data()[0].get());
  const auto& root_function = static_cast<const Library::Language::Function&>(
      root->get_authored_bindings().get_data()[0].get());
  const auto& unrelated_function =
      static_cast<const Library::Language::Function&>(
          unrelated->get_authored_bindings().get_data()[0].get());
  EXPECT_NOT(leaf_function.is_signature_linked());
  EXPECT_NOT(middle_function.is_signature_linked());
  EXPECT_NOT(root_function.is_signature_linked());
  EXPECT_NOT(unrelated_function.is_signature_linked());

  ASSERT(root->link());
  EXPECT(leaf_function.is_signature_linked());
  EXPECT(middle_function.is_signature_linked());
  EXPECT(root_function.is_signature_linked());
  EXPECT(leaf_function.is_linked());
  EXPECT(middle_function.is_linked());
  EXPECT(root_function.is_linked());
  EXPECT_NOT(unrelated_function.is_signature_linked());
  EXPECT_NOT(unrelated_function.is_linked());
  EXPECT(leaf->get_diagnostics().is_empty());
  EXPECT(middle->get_diagnostics().is_empty());
  EXPECT(root->get_diagnostics().is_empty());
  EXPECT(unrelated->get_diagnostics().is_empty());
}

PERIMORTEM_UNIT_TEST(
    LibraryImports,
    reachable_signature_failure_stops_every_body) {
  Allocator::Arena arena;
  ImportRegistry registry;
  Library::Dialect library_dialect;
  Package::Dialect package_dialect;
  auto blocked = interpret_library(
      arena, library_dialect, registry,
      "public func blocked[Missing] -> Void {}"_view);
  auto ready = interpret_library(
      arena, library_dialect, registry, "public func ready[] -> Void {}"_view);
  ASSERT(blocked && ready);

  Package::Language::Monograph& target = create_package(arena, package_dialect);
  ASSERT(target.bind_member("BlockedApi"_view, *blocked));
  ASSERT(target.bind_member("ReadyApi"_view, *ready));
  Package::Language::Monograph& root_context =
      create_package_with_dependency(arena, package_dialect, "Providers"_view);
  ASSERT(bind_only_dependency(root_context, target));
  auto root = interpret_library(
      arena, library_dialect, root_context,
      "using Providers;\npublic func root[] -> Void {}"_view);
  ASSERT(root);

  const auto& blocked_function =
      static_cast<const Library::Language::Function&>(
          blocked->get_authored_bindings().get_data()[0].get());
  const auto& ready_function = static_cast<const Library::Language::Function&>(
      ready->get_authored_bindings().get_data()[0].get());
  const auto& root_function = static_cast<const Library::Language::Function&>(
      root->get_authored_bindings().get_data()[0].get());
  ASSERT_NOT(root->link());
  EXPECT_NOT(blocked_function.is_signature_linked());
  EXPECT(ready_function.is_signature_linked());
  EXPECT(root_function.is_signature_linked());
  EXPECT_NOT(blocked_function.is_linked());
  EXPECT_NOT(ready_function.is_linked());
  EXPECT_NOT(root_function.is_linked());
  EXPECT_EQ(blocked->get_diagnostics().get_size(), Count(1));
  EXPECT(ready->get_diagnostics().is_empty());
  EXPECT(root->get_diagnostics().is_empty());
}

PERIMORTEM_UNIT_TEST(
    LibraryImports,
    reachable_cycle_preserves_exact_identities) {
  Allocator::Arena arena;
  Library::Dialect library_dialect;
  Package::Dialect package_dialect;
  Package::Language::Monograph& first_context =
      create_package_with_dependency(arena, package_dialect, "Second"_view);
  Package::Language::Monograph& second_context =
      create_package_with_dependency(arena, package_dialect, "First"_view);
  auto first = interpret_library(
      arena, library_dialect, first_context,
      "using Second;\npublic func first[] -> Void {}"_view);
  auto second = interpret_library(
      arena, library_dialect, second_context,
      "using First;\npublic func second[] -> Void {}"_view);
  ASSERT(first && second);

  Package::Language::Monograph& first_target =
      create_package(arena, package_dialect);
  Package::Language::Monograph& second_target =
      create_package(arena, package_dialect);
  ASSERT(first_target.bind_member("FirstApi"_view, *first));
  ASSERT(second_target.bind_member("SecondApi"_view, *second));
  ASSERT(bind_only_dependency(first_context, second_target));
  ASSERT(bind_only_dependency(second_context, first_target));
  ASSERT(first->link());

  const auto& first_function = static_cast<const Library::Language::Function&>(
      first->get_authored_bindings().get_data()[0].get());
  const auto& second_function = static_cast<const Library::Language::Function&>(
      second->get_authored_bindings().get_data()[0].get());
  ASSERT(first->get_source().is<Library::Language::Types::Structure>());
  ASSERT(second->get_source().is<Library::Language::Types::Structure>());
  const auto& first_source =
      static_cast<const Library::Language::Types::Structure&>(
          first->get_source());
  const auto& second_source =
      static_cast<const Library::Language::Types::Structure&>(
          second->get_source());
  const Abstract& second_alias = select_binding(
      first_source.get_callable_bindings(first_function), "second"_view);
  const Abstract& first_alias = select_binding(
      second_source.get_callable_bindings(second_function), "first"_view);
  ASSERT(second_alias.is<Ttx::Model::Alias>());
  ASSERT(first_alias.is<Ttx::Model::Alias>());
  EXPECT(&second_alias.resolve() == &second_function);
  EXPECT(&first_alias.resolve() == &first_function);
  EXPECT(first_function.is_linked());
  EXPECT(second_function.is_linked());
  ASSERT(second->link());
  EXPECT(&second_alias.resolve() == &second_function);
  EXPECT(&first_alias.resolve() == &first_function);
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
      "public Shared : struct {}\n"
      "public Mode : enum[Unsigned_8] { ready = 1; }\n"
      "public func provided[] -> Void {}\n"_view);
  Bool importer_written = package.write(
      "main.ttx"_view,
      "// Importing Library\n"
      "dialect : Library;\n"
      "using Core;\n"
      "private Holder : struct {\n"
      "  private shared : Shared;\n"
      "  private mode : Mode;\n"
      "}\n"
      "private func local[Shared, Mode] -> Shared {}\n"_view);
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
  ASSERT_EQ(main_bindings.get_size(), Count(2));
  ASSERT(main_bindings.get_data()[0]
             .get()
             .is<Library::Language::Types::Structure>());
  ASSERT(main_bindings.get_data()[1].get().is<Library::Language::Function>());
  const auto& holder = static_cast<const Library::Language::Types::Structure&>(
      main_bindings.get_data()[0].get());
  const auto& local = static_cast<const Library::Language::Function&>(
      main_bindings.get_data()[1].get());
  ASSERT(main.get_source().is<Library::Language::Types::Structure>());
  const auto& main_source =
      static_cast<const Library::Language::Types::Structure&>(
          main.get_source());
  const Abstract& provided_alias =
      select_binding(main_source.get_callable_bindings(local), "provided"_view);
  ASSERT(provided_alias.is<Ttx::Model::Alias>());
  ASSERT(api.get_source().is<Library::Language::Types::Structure>());
  const auto& api_source =
      static_cast<const Library::Language::Types::Structure&>(api.get_source());
  const Abstract& shared = api.resolve_context("Shared"_view);
  const Abstract& mode = api.resolve_context("Mode"_view);
  ASSERT(shared.is<Library::Language::Types::Structure>());
  ASSERT(mode.is<Library::Language::Types::Enumeration>());
  auto holder_fields = holder.get_fields();
  ASSERT_EQ(holder_fields.get_size(), Count(2));
  EXPECT(&holder_fields.get_data()[0].get().get_type() == &shared);
  EXPECT(&holder_fields.get_data()[1].get().get_type() == &mode);
  auto local_signature = local.get_signature();
  ASSERT(local_signature);
  ASSERT(local_signature->get_parameter_type(0));
  ASSERT(local_signature->get_parameter_type(1));
  ASSERT(local_signature->get_result_type(0));
  EXPECT(&*local_signature->get_parameter_type(0) == &shared);
  EXPECT(&*local_signature->get_parameter_type(1) == &mode);
  EXPECT(&*local_signature->get_result_type(0) == &shared);
  EXPECT(
      &provided_alias.resolve() ==
      &select_binding(api_source.get_callable_bindings(), "provided"_view));
  EXPECT(&main.resolve_context("provided"_view) == &Invalid::get_invalid());
  EXPECT(&main.resolve_context("local"_view) == &Invalid::get_invalid());
  EXPECT(&main.resolve_context("source"_view) == &main_source);
  EXPECT(main_source.get_callable_bindings().is_empty());
  EXPECT(errors.is_empty());
}
