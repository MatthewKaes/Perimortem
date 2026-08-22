// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/dialect.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/package/fixture.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/package/language/dependency.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/package/language/source.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/span.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/alias.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

static_assert(!__is_constructible(
    Package::Language::Monograph,
    const Package::Language::Monograph&));
static_assert(!__is_constructible(
    Package::Language::Monograph,
    Package::Language::Monograph&&));

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

static auto has_diagnostic_marker(const Errors& errors, Count width) -> Bool {
  Dynamic::Bytes marker("^"_view);
  for (Count i = 1; i < width; i++) {
    marker.concat("-"_view);
  }

  marker.concat("\n"_view);
  return has_diagnostic(errors, marker);
}

struct DependencyDescription {
  View::Bytes local_name;
  View::Bytes package_name;
  Version version;

  constexpr auto matches(const Package::Language::Dependency& value) const
      -> Bool {
    return value.get_local_name() == local_name &&
           value.get_package_name() == package_name &&
           value.get_version() == version;
  }
};

struct SourceDescription {
  View::Bytes local_name;
  View::Bytes source_path;

  constexpr auto matches(const Package::Language::Source& value) const -> Bool {
    return value.get_local_name() == local_name &&
           value.get_source_path() == source_path;
  }
};

struct RejectedBody {
  View::Bytes path;
  View::Bytes body;
  View::Bytes diagnostic;
};

struct RejectedFile {
  View::Bytes path;
  View::Bytes diagnostic;
};

template <typename value_type, typename description_type, Count extent>
static auto matches_description_table(
    View::Vector<value_type> actual,
    const description_type (&expected)[extent]) -> Bool {
  if (actual.get_size() != extent) {
    return False;
  }

  for (Count i = 0; i < actual.get_size(); i++) {
    if (!expected[i].matches(actual.get_data()[i])) {
      return False;
    }
  }

  return True;
}

// Grammar failures belong to Package interpretation. Workspace appears only
// in the atomicity case so its direct Package rejection cannot mask the Dialect
// diagnostic under test.
static auto rejects_source(
    View::Bytes path,
    View::Bytes source,
    View::Bytes diagnostic) -> Bool {
  Package::Dialect dialect;
  Allocator::Arena arena;
  Errors errors;
  const auto interpreted =
      interpret_package(arena, dialect, errors, source, path);

  return !interpreted && has_diagnostic(errors, diagnostic);
}

static auto rejects_body(
    View::Bytes path,
    View::Bytes body,
    View::Bytes diagnostic) -> Bool {
  Dynamic::Bytes source("// Rejected Package\ndialect : Package;\n"_view);
  source.concat(body);
  return rejects_source(path, source, diagnostic);
}

static auto rejects_package_file(View::Bytes path, View::Bytes diagnostic)
    -> Bool {
  auto source = File::read(path);
  if (!source) {
    return False;
  }

  return rejects_source(path, *source, diagnostic);
}

static Harness PackageDialect = {
  .name = "Tetrodotoxin::Package::Dialect"_view,
};

class ScopeMember : public Language::Monograph {
 public:
  ScopeMember(
      Allocator::Arena& arena,
      Language::Dialect& dialect,
      View::Bytes name)
      : Monograph(arena, dialect, Documentation::get_empty(), dialect),
        name(name) {}

  auto get_name() const -> View::Bytes override { return name; }

  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }

 private:
  View::Bytes name;
};

PERIMORTEM_UNIT_TEST(PackageDialect, dependency_statement) {
  static constexpr View::Bytes source =
      "resolve Runtime::Math : Perimortem.Graphics.Math = \"12.34\";\n"
      "source Main from \"main.ttx\";"_view;
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, source, "dependency.ttx"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  auto parsed = Package::Language::Dependency::parse(cursor);

  ASSERT(parsed);
  Span span = parsed->get_span();
  EXPECT_TEXT(parsed->get_local_name(), "Runtime::Math"_view);
  EXPECT_TEXT(parsed->get_package_name(), "Perimortem.Graphics.Math"_view);
  EXPECT(parsed->get_version() == Version(12, 34));
  EXPECT(span.get_start().get_code() == Code::Type::Resolve);
  EXPECT_EQ(span.get_start().get_offset(), U16(0));
  EXPECT_TEXT(span.get_start().caculate_text(source), "resolve"_view);
  EXPECT(span.get_end().get_code() == Code::Type::EndStatement);
  EXPECT_EQ(span.get_end().get_offset(), U16(58));
  EXPECT_TEXT(span.get_end().caculate_text(source), ";"_view);
  EXPECT_TEXT(
      span.caculate_text(source),
      "resolve Runtime::Math : Perimortem.Graphics.Math = \"12.34\";"_view);
  EXPECT(cursor.matches(Code::Type::Source));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(PackageDialect, source_statement) {
  static constexpr View::Bytes source =
      "source Scenes::Splash from \"scenes/./splash.ttx\";\n"
      "resolve Runtime : Example.Runtime = \"1.0\";"_view;
  Allocator::Arena arena;
  Errors errors;
  Tokenizer tokenizer(arena, source, "source.ttx"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  auto parsed = Package::Language::Source::parse(cursor);

  ASSERT(parsed);
  Span span = parsed->get_span();
  EXPECT_TEXT(parsed->get_local_name(), "Scenes::Splash"_view);
  EXPECT_TEXT(parsed->get_source_path(), "scenes/splash.ttx"_view);
  EXPECT(span.get_start().get_code() == Code::Type::Source);
  EXPECT(span.get_end().get_code() == Code::Type::EndStatement);
  EXPECT_TEXT(
      span.caculate_text(source),
      "source Scenes::Splash from \"scenes/./splash.ttx\";"_view);
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
  Package::Dialect dialect;
  Allocator::Arena arena;
  Errors errors;

  auto interpreted = interpret_package(
      arena, dialect, errors, source, "synthetic/package.ttx"_view);
  ASSERT(interpreted);
  source.set('x');

  const Package::Language::Monograph& monograph = *interpreted;
  const DependencyDescription expected_dependencies[] = {
    {"Runtime::Math"_view, "Perimortem.Graphics.Math"_view, Version(12, 34)},
    {"Assets"_view, "Example.Assets"_view, Version(2, 7)},
  };
  const SourceDescription expected_sources[] = {
    {"Scenes::Splash"_view, "scenes/splash.ttx"_view},
    {"Main"_view, "main.ttx"_view},
  };
  View::Vector<Package::Language::Dependency> dependencies =
      monograph.get_dependencies();
  View::Vector<Package::Language::Source> sources = monograph.get_sources();
  EXPECT(matches_description_table(dependencies, expected_dependencies));
  EXPECT(matches_description_table(sources, expected_sources));

  const auto* dependency_data = dependencies.get_data();
  const auto* source_data = sources.get_data();
  EXPECT_EQ(dependency_data[0].get_span().get_start().get_line(), U16(4));
  EXPECT_EQ(dependency_data[1].get_span().get_start().get_line(), U16(5));
  EXPECT_EQ(source_data[0].get_span().get_start().get_line(), U16(6));
  EXPECT_EQ(source_data[1].get_span().get_start().get_line(), U16(7));

  ASSERT_EQ(monograph.get_documentation().line_count(), 1);
  EXPECT_TEXT(
      monograph.get_documentation().get_line(0), "Synthetic Package"_view);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(PackageDialect, canonical_inventory) {
  static constexpr View::Bytes path =
      "apps/ttx/scene_lifetime/package.ttx"_view;
  auto source = File::read(path);
  ASSERT(source);

  Package::Dialect dialect;
  Allocator::Arena arena;
  Errors errors;

  auto interpreted = interpret_package(arena, dialect, errors, *source, path);
  ASSERT(interpreted);
  const Package::Language::Monograph& monograph = *interpreted;
  const DependencyDescription expected_dependencies[] = {
    {"Math"_view, "Perimortem.Math"_view, Version(1, 0)},
    {"Graphics"_view, "Perimortem.Graphics"_view, Version(1, 0)},
    {"System"_view, "Perimortem.System"_view, Version(1, 0)},
  };
  const SourceDescription expected_sources[] = {
    {"Scenes::Splash"_view, "scenes/splash.ttx"_view},
    {"Scenes::Title"_view, "scenes/title.ttx"_view},
    {"Main"_view, "main.ttx"_view},
  };

  EXPECT(matches_description_table(
      monograph.get_dependencies(), expected_dependencies));
  EXPECT(matches_description_table(monograph.get_sources(), expected_sources));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(PackageDialect, prior_diagnostics) {
  Package::Dialect dialect;
  Allocator::Arena arena;
  Errors errors;
  {
    Errors::Report report(
        errors, "prior-package.ttx"_view, View::Bytes(),
        Anchor::create(Span()));
    report << "Earlier independent diagnostic."_view;
  }

  auto interpreted = interpret_package(
      arena, dialect, errors,
      "// Minimal\ndialect : Package;\nsource Main from \"./main.ttx\";\n"_view,
      "minimal.ttx"_view);
  ASSERT(interpreted);
  EXPECT_EQ(errors.get_size(), 1);
}

PERIMORTEM_UNIT_TEST(PackageDialect, value_local_provenance) {
  Span core_span(
      Token(0, 1, 1, 7, Code::Type::Resolve),
      Token(40, 1, 41, 1, Code::Type::EndStatement));
  Span memory_span(
      Token(42, 2, 1, 7, Code::Type::Resolve),
      Token(84, 2, 43, 1, Code::Type::EndStatement));
  Span main_span(
      Token(86, 3, 1, 6, Code::Type::Source),
      Token(113, 3, 28, 1, Code::Type::EndStatement));
  Package::Language::Dependency dependencies[] = {
    Package::Language::Dependency(
        "Core"_view, "Perimortem.Core"_view, Version(1, 0), core_span),
    Package::Language::Dependency(
        "Memory"_view, "Perimortem.Memory"_view, Version(1, 0), memory_span),
  };
  Package::Language::Source sources[] = {
    Package::Language::Source("Main"_view, "main.ttx"_view, main_span),
  };
  Package::Dialect dialect;
  Allocator::Arena arena;
  auto authored = Package::Language::Monograph::create_authored(
      arena, dialect, Documentation::get_empty(), dialect, dependencies,
      sources);
  ASSERT(authored);
  const auto& authored_package = *authored;
  EXPECT(
      authored_package.get_dependencies().get_data()[0].get_span() ==
      core_span);
  EXPECT(
      authored_package.get_dependencies().get_data()[1].get_span() ==
      memory_span);
  EXPECT(authored_package.get_sources().get_data()[0].get_span() == main_span);

  auto empty_authored = Package::Language::Monograph::create_authored(
      arena, dialect, Documentation::get_empty(), dialect, {}, {});
  EXPECT_NOT(empty_authored);

  Package::Language::Dependency restored_dependencies[] = {
    Package::Language::Dependency(
        "Core"_view, "Perimortem.Core"_view, Version(1, 0)),
    Package::Language::Dependency(
        "Memory"_view, "Perimortem.Memory"_view, Version(1, 0)),
  };
  auto& source_free = Package::Language::Monograph::create_synthetic(
      arena, dialect, dialect, restored_dependencies);
  ASSERT_EQ(source_free.get_dependencies().get_size(), Count(2));
  EXPECT_NOT(source_free.get_dependencies().get_data()[0].get_span());
  EXPECT_NOT(source_free.get_dependencies().get_data()[1].get_span());
  EXPECT(source_free.get_sources().is_empty());
}

PERIMORTEM_UNIT_TEST(PackageDialect, exact_scope) {
  Package::Language::Dependency dependencies[] = {
    Package::Language::Dependency(
        "Runtime::Core"_view, "Example.Core"_view, Version(1, 0)),
  };
  Package::Language::Source sources[] = {
    Package::Language::Source("Qualified::Member"_view, "member.ttx"_view),
    Package::Language::Source("Second::Member"_view, "second.ttx"_view),
    Package::Language::Source("Self::Member"_view, "self.ttx"_view),
  };
  Package::Dialect dialect;
  Allocator::Arena arena;
  auto root_result = Package::Language::Monograph::create_authored(
      arena, dialect, Documentation::get_empty(), dialect, dependencies,
      sources);
  ASSERT(root_result);
  auto& root = *root_result;
  auto& member =
      arena.construct<ScopeMember>(arena, dialect, "Original member"_view);
  auto& second_member =
      arena.construct<ScopeMember>(arena, dialect, "Second member"_view);
  auto& replacement =
      arena.construct<ScopeMember>(arena, dialect, "Replacement member"_view);
  auto& dependency_root = Package::Language::Monograph::create_synthetic(
      arena, dialect, dialect, {});
  auto& replacement_dependency = Package::Language::Monograph::create_synthetic(
      arena, dialect, dialect, {});

  ASSERT(root.bind_member("Qualified::Member"_view, member));
  ASSERT(root.bind_member("Second::Member"_view, second_member));

  const Abstract& qualified_scope = root.resolve_context("Qualified"_view);
  const Abstract& second_scope = root.resolve_context("Second"_view);
  const Abstract& member_edge = qualified_scope.resolve_context("Member"_view);
  const Abstract& second_member_edge =
      second_scope.resolve_context("Member"_view);
  EXPECT_TEXT(member_edge.get_name(), "Member"_view);
  EXPECT_TEXT(second_member_edge.get_name(), "Member"_view);
  EXPECT(&member_edge.resolve() == &member);
  EXPECT(&second_member_edge.resolve() == &second_member);

  EXPECT_NOT(root.bind_member("Qualified::Member"_view, replacement));
  EXPECT(&qualified_scope.resolve_context("Member"_view) == &member_edge);
  EXPECT(&member_edge.resolve() == &member);

  EXPECT_NOT(root.bind_member(View::Bytes(), replacement));
  EXPECT(&root.resolve_context(View::Bytes()) == &Invalid::get_invalid());

  EXPECT_NOT(root.bind_member("Undeclared::Member"_view, replacement));
  EXPECT(
      &root.resolve_context("Undeclared::Member"_view) ==
      &Invalid::get_invalid());

  EXPECT_NOT(root.bind_member("Self::Member"_view, root));
  EXPECT(&root.resolve_context("Self::Member"_view) == &Invalid::get_invalid());

  Package::Language::Dependency forged_dependency(
      "Runtime::Core"_view, "Example.Core"_view, Version(1, 0));
  EXPECT_NOT(root.bind_dependency(forged_dependency, dependency_root));
  EXPECT(
      &root.resolve_context("Runtime::Core"_view) == &Invalid::get_invalid());

  const auto& retained_dependency = root.get_dependencies().get_data()[0];
  ASSERT(root.bind_dependency(retained_dependency, dependency_root));

  const Abstract& runtime_scope = root.resolve_context("Runtime"_view);
  const Abstract& dependency_edge = runtime_scope.resolve_context("Core"_view);
  ASSERT(dependency_edge.is<Ttx::Model::Alias>());
  EXPECT_TEXT(dependency_edge.get_name(), "Core"_view);
  EXPECT(&dependency_edge.resolve() == &dependency_root);
  EXPECT_NOT(root.bind_dependency(retained_dependency, replacement_dependency));
  EXPECT(&runtime_scope.resolve_context("Core"_view) == &dependency_edge);
  EXPECT(&dependency_edge.resolve() == &dependency_root);

  EXPECT(&root.resolve_context("Qualified"_view) == &qualified_scope);
  EXPECT(&root.resolve_context("Member"_view) == &Invalid::get_invalid());
  EXPECT(
      &root.resolve_context("Qualified::Member"_view) ==
      &Invalid::get_invalid());
  EXPECT(
      &root.resolve_context("qualified::Member"_view) ==
      &Invalid::get_invalid());
  EXPECT(
      &root.resolve_context("Qualified::Member::Tail"_view) ==
      &Invalid::get_invalid());
  EXPECT(
      &root.resolve_context("Qualified.Member"_view) ==
      &Invalid::get_invalid());
  EXPECT_NOT(root.bind_member("Runtime::Core"_view, replacement));
  EXPECT(&runtime_scope.resolve_context("Core"_view) == &dependency_edge);

  Package::Language::Dependency undeclared(
      "Qualified::Member"_view, "Example.Other"_view, Version(1, 0));
  EXPECT_NOT(root.bind_dependency(undeclared, replacement_dependency));
}

PERIMORTEM_UNIT_TEST(PackageDialect, source_free_scope) {
  Package::Language::Dependency dependencies[] = {
    Package::Language::Dependency(
        "External"_view, "Example.External"_view, Version(2, 4)),
    Package::Language::Dependency(
        "Later"_view, "Example.Later"_view, Version(3, 1)),
  };
  const DependencyDescription expected_dependencies[] = {
    {"External"_view, "Example.External"_view, Version(2, 4)},
    {"Later"_view, "Example.Later"_view, Version(3, 1)},
  };
  Package::Dialect dialect;
  Allocator::Arena arena;
  auto& root = Package::Language::Monograph::create_synthetic(
      arena, dialect, dialect, dependencies);
  auto& later_member = arena.construct<ScopeMember>(
      arena, dialect, "Later restored identity"_view);
  auto& earlier_member = arena.construct<ScopeMember>(
      arena, dialect, "Earlier restored identity"_view);
  auto& dependency_root = Package::Language::Monograph::create_synthetic(
      arena, dialect, dialect, {});

  EXPECT(matches_description_table(
      root.get_dependencies(), expected_dependencies));
  EXPECT(root.get_sources().is_empty());
  ASSERT(root.bind_member("Restored::Later"_view, later_member));
  ASSERT(root.bind_member("Restored::Earlier"_view, earlier_member));
  ASSERT(root.bind_dependency(
      root.get_dependencies().get_data()[0], dependency_root));

  const Abstract& dependency_edge = root.resolve_context("External"_view);
  const Abstract& restored_scope = root.resolve_context("Restored"_view);
  const Abstract& later_member_edge =
      restored_scope.resolve_context("Later"_view);
  const Abstract& earlier_member_edge =
      restored_scope.resolve_context("Earlier"_view);
  ASSERT(dependency_edge.is<Ttx::Model::Alias>());
  EXPECT_TEXT(later_member_edge.get_name(), "Later"_view);
  EXPECT_TEXT(earlier_member_edge.get_name(), "Earlier"_view);
  EXPECT(
      &root.resolve_context("Restored::Later"_view) == &Invalid::get_invalid());
  EXPECT(&later_member_edge.resolve() == &later_member);
  EXPECT(&earlier_member_edge.resolve() == &earlier_member);
  EXPECT(&dependency_edge.resolve() == &dependency_root);
  EXPECT_NOT(root.bind_member("External"_view, later_member));
  EXPECT(&root.resolve_context("External"_view) == &dependency_edge);
}

PERIMORTEM_UNIT_TEST(PackageDialect, collision_atomicity) {
  static constexpr View::Bytes collision_source =
      "// Scope collision\n"
      "dialect : Package;\n"
      "resolve Runtime : Example.Runtime = \"1.0\";\n"
      "source Runtime from \"runtime.ttx\";\n"
      "source Main from \"main.ttx\";\n"_view;
  static constexpr View::Bytes duplicate_source =
      "// Duplicate dimensions\n"
      "dialect : Package;\n"
      "source First from \"./member.ttx\";\n"
      "source First from \"member.ttx\";\n"_view;
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install<Package::Dialect>("Package"_view));
  Environment::Workspace workspace(toolchain);

  Errors collision_errors;
  EXPECT_NOT(workspace.interpret_source(
      collision_errors, "Rejected"_view, "collision.ttx"_view,
      collision_source));
  EXPECT(
      &workspace.resolve_context("Rejected"_view) == &Invalid::get_invalid());
  EXPECT_EQ(collision_errors.get_size(), Count(1));
  EXPECT(has_diagnostic(
      collision_errors,
      "Source semantic name collides with a Dependency local alias in this "
      "Package."_view));
  EXPECT(has_diagnostic_marker(collision_errors, "source"_view.get_size()));

  Errors duplicate_errors;
  EXPECT_NOT(workspace.interpret_source(
      duplicate_errors, "Rejected"_view, "duplicates.ttx"_view,
      duplicate_source));
  EXPECT(
      &workspace.resolve_context("Rejected"_view) == &Invalid::get_invalid());
  EXPECT(has_diagnostic(
      duplicate_errors,
      "Duplicate Source semantic name in this Package."_view));
  EXPECT(has_diagnostic(
      duplicate_errors,
      "Duplicate normalized Source path in this Package."_view));
  EXPECT_NOT(has_diagnostic(
      duplicate_errors,
      "Package requires at least one complete Source statement."_view));
}

PERIMORTEM_UNIT_TEST(PackageDialect, frozen_negative_fixtures) {
  static constexpr RejectedFile rejected[] = {
    {
      "validation/data/ttx/package/float_version.ttx"_view,
      "Package dependency versions must be quoted."_view,
    },
    {
      "validation/data/ttx/package/noncanonical_version.ttx"_view,
      "Package dependency version is not canonical `Major.Minor` text."_view,
    },
    {
      "validation/data/ttx/package/duplicate_semantic_name.ttx"_view,
      "Duplicate Source semantic name in this Package."_view,
    },
    {
      "validation/data/ttx/package/duplicate_normalized_path.ttx"_view,
      "Duplicate normalized Source path in this Package."_view,
    },
  };

  for (const RejectedFile& rejection : rejected) {
    EXPECT(rejects_package_file(rejection.path, rejection.diagnostic));
  }
}

PERIMORTEM_UNIT_TEST(PackageDialect, authored_rejections) {
  static constexpr RejectedBody rejected[] = {
    {
      "dependency.ttx"_view,
      "resolve Runtime : First.Runtime = \"1.0\";\n"
      "resolve Runtime : Second.Runtime = \"2.0\";\n"
      "source Main from \"main.ttx\";\n"_view,
      "Duplicate Dependency local alias in this Package."_view,
    },
    {
      "dependencies_only.ttx"_view,
      "resolve Runtime : Example.Runtime = \"1.0\";\n"_view,
      "Package requires at least one complete Source statement."_view,
    },
    {
      "late_resolve.ttx"_view,
      "source Main from \"main.ttx\";\n"
      "resolve Runtime : Example.Runtime = \"1.0\";\n"_view,
      "Resolve statements must precede every Source statement."_view,
    },
    {
      "missing_version.ttx"_view,
      "resolve Runtime : Example.Runtime = ;\n"
      "source Main from \"main.ttx\";\n"_view,
      "Package dependency versions must be quoted."_view,
    },
    {
      "repeated_assignment.ttx"_view,
      "resolve Runtime : Example.Runtime = = \"1.0\";\n"
      "source Main from \"main.ttx\";\n"_view,
      "Package dependency versions must be quoted."_view,
    },
    {
      "unterminated_version.ttx"_view,
      "resolve Runtime : Example.Runtime = \"1.0"_view,
      "Package dependency version String is unterminated."_view,
    },
    {
      "empty_version.ttx"_view,
      "resolve Runtime : Example.Runtime = \"\";\n"
      "source Main from \"main.ttx\";\n"_view,
      "Package dependency version is not canonical `Major.Minor` text."_view,
    },
    {
      "null_version.ttx"_view,
      "resolve Runtime : Example.Runtime = \"0.0\";\n"
      "source Main from \"main.ttx\";\n"_view,
      "Package dependency version is not canonical `Major.Minor` text."_view,
    },
    {
      "overflow_version.ttx"_view,
      "resolve Runtime : Example.Runtime = \"65536.0\";\n"
      "source Main from \"main.ttx\";\n"_view,
      "Package dependency version is not canonical `Major.Minor` text."_view,
    },
    {
      "local_spacing.ttx"_view,
      "source Scenes :: Splash from \"splash.ttx\";\n"_view,
      "Semantic names cannot contain whitespace around `::`."_view,
    },
    {
      "external_spacing.ttx"_view,
      "resolve Math : Perimortem . Math = \"1.0\";\n"
      "source Main from \"main.ttx\";\n"_view,
      "External Package names cannot contain whitespace around `.`."_view,
    },
    {
      "malformed_qualification.ttx"_view,
      "source Scenes:: from \"splash.ttx\";\n"_view,
      "Semantic name qualification requires a Type segment after `::`."_view,
    },
    {
      "anonymous.ttx"_view,
      "source \"main.ttx\";\n"_view,
      "Expected an authored Type shaped semantic name."_view,
    },
    {
      "missing_from.ttx"_view,
      "source Main \"main.ttx\";\n"_view,
      "Source statements require `from`."_view,
    },
    {
      "wrong_relation.ttx"_view,
      "source Main via \"main.ttx\";\n"_view,
      "Source statements require `from`."_view,
    },
    {
      "missing_path.ttx"_view,
      "source Main from;\n"_view,
      "Source paths must be closed quoted String values."_view,
    },
    {
      "unquoted_path.ttx"_view,
      "source Main from main;\n"_view,
      "Source paths must be closed quoted String values."_view,
    },
    {
      "unterminated_path.ttx"_view,
      "source Main from \"main.ttx"_view,
      "Source path String is unterminated."_view,
    },
    {
      "missing_terminator.ttx"_view,
      "source Main from \"main.ttx\""_view,
      "Source statements require a terminating `;`."_view,
    },
    {
      "resolve_terminator.ttx"_view,
      "resolve Runtime : Example.Runtime = \"1.0\"\n"
      "source Main from \"main.ttx\";"_view,
      "Resolve statements require a terminating `;`."_view,
    },
  };

  for (const RejectedBody& rejection : rejected) {
    EXPECT(rejects_body(rejection.path, rejection.body, rejection.diagnostic));
  }
}
