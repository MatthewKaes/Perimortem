// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/resources.hpp"

#include "validation/unit_test.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
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

static constexpr Count temporary_path_capacity = 160;

static auto join_path(View::Bytes base, View::Bytes member) -> Dynamic::Bytes {
  Dynamic::Bytes path(base);
  path.append('/');
  path.concat(member);
  return path;
}

static auto native_path(Dynamic::Bytes& path) -> char* {
  path.append('\0');
  return Data::cast<char>(path.get_access().get_data());
}

static auto remove_member(View::Bytes root, View::Bytes member) -> void {
  Dynamic::Bytes path = join_path(root, member);
  File::remove(path);
}

static auto cleanup_tree(View::Bytes root) -> void {
  if (root.is_empty()) {
    return;
  }

  remove_member(root, "resources/table.bin"_view);
  remove_member(root, "resources/other.bin"_view);
  remove_member(root, "resources/later.bin"_view);
  remove_member(root, "resources/missing.bin"_view);
  remove_member(root, "resources/empty.bin"_view);
  remove_member(root, "resources"_view);
  File::remove(root);
}

class TemporaryResources {
 public:
  TemporaryResources() {
    S32 written = snprintf(
        Data::cast<char>(root_path.get_data()), root_path.get_size(),
        "/tmp/tetrodotoxin_package_resources_XXXXXX");
    if (written <= 0 || Count(written) >= root_path.get_size()) {
      return;
    }

    char* created = mkdtemp(Data::cast<char>(root_path.get_data()));
    valid = created != nullptr;
  }

  TemporaryResources(const TemporaryResources&) = delete;
  auto operator=(const TemporaryResources&) -> TemporaryResources& = delete;

  ~TemporaryResources() {
    if (valid) {
      cleanup_tree(get_root());
    }
  }

  operator bool() const { return bool(valid); }

  auto get_root() const -> View::Bytes {
    return NullTerminated::to_view(
        Data::cast<const char>(root_path.get_data()));
  }

  auto create_directory(View::Bytes member) const -> Bool {
    Dynamic::Bytes path = join_path(get_root(), member);
    S32 created = mkdir(native_path(path), S_IRWXU);
    return created == 0;
  }

  auto write(View::Bytes member, View::Bytes contents) const -> Bool {
    Dynamic::Bytes path = join_path(get_root(), member);
    return File::write(contents, path);
  }

 private:
  Static::Bytes<temporary_path_capacity> root_path;
  Bool valid = False;
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

static auto describes(const Abstract& abstract, View::Bytes expected) -> Bool {
  if (!abstract.is<Tetrodotoxin::Language::Error>()) {
    return False;
  }

  Errors errors;
  {
    Errors::Report report(
        errors, "resource.ttx"_view, "$[resource]"_view,
        Anchor::create(Span()));
    const auto& error =
        static_cast<const Tetrodotoxin::Language::Error&>(abstract);
    error.describe(report);
  }

  Allocator::Arena arena;
  return errors.get_size() == 1 &&
         contains(errors.render_message(arena, 0), expected);
}

static Harness PackageResources = {
  .name = "Tetrodotoxin::Package::Resources"_view,
};

PERIMORTEM_UNIT_TEST(PackageResources, identity_and_seal) {
  TemporaryResources temporary;
  ASSERT(temporary);
  ASSERT(temporary.create_directory("resources"_view));
  ASSERT(temporary.write("resources/table.bin"_view, "table bytes"_view));
  ASSERT(temporary.write("resources/other.bin"_view, "table bytes"_view));
  ASSERT(temporary.write("resources/later.bin"_view, "later bytes"_view));
  ASSERT(temporary.write("resources/empty.bin"_view, View::Bytes()));

  Allocator::Arena arena;
  auto storage = Package::Storage::open(arena, temporary.get_root());
  ASSERT(storage);
  Package::Resources resources(arena);
  ASSERT(resources.connect(*storage));
  EXPECT_NOT(resources.connect(*storage));

  Dynamic::Bytes route("resources/cache/../table.bin"_view);
  const Abstract& first = resources.resolve(route);
  ASSERT(first.is<Tetrodotoxin::Language::Resource>());
  route.set('x');

  const Abstract& alias = resources.resolve("resources/./table.bin"_view);
  const Abstract& empty = resources.resolve("resources/empty.bin"_view);
  const Abstract& other = resources.resolve("resources/other.bin"_view);
  ASSERT(empty.is<Tetrodotoxin::Language::Resource>());
  ASSERT(other.is<Tetrodotoxin::Language::Resource>());

  const auto& first_resource =
      static_cast<const Tetrodotoxin::Language::Resource&>(first);
  const auto& empty_resource =
      static_cast<const Tetrodotoxin::Language::Resource&>(empty);
  const auto& other_resource =
      static_cast<const Tetrodotoxin::Language::Resource&>(other);
  EXPECT(&first == &alias);
  EXPECT_TEXT(first_resource.get_value(), "table bytes"_view);
  EXPECT(empty_resource.get_value().is_empty());
  EXPECT_TEXT(other_resource.get_value(), first_resource.get_value());
  EXPECT(&first != &other);

  resources.seal();
  EXPECT_NOT(resources.connect(*storage));
  EXPECT(&resources.resolve("resources/table.bin"_view) == &first);
  EXPECT(
      &resources.resolve("resources/later.bin"_view) ==
      &Invalid::get_invalid());
}

PERIMORTEM_UNIT_TEST(PackageResources, error_identity) {
  TemporaryResources temporary;
  ASSERT(temporary);
  ASSERT(temporary.create_directory("resources"_view));

  Allocator::Arena arena;
  auto storage = Package::Storage::open(arena, temporary.get_root());
  ASSERT(storage);
  Package::Resources resources(arena);
  ASSERT(resources.connect(*storage));

  const Abstract& missing =
      resources.resolve("resources/cache/../missing.bin"_view);
  ASSERT(missing.is<Tetrodotoxin::Language::Error>());
  ASSERT(temporary.write("resources/missing.bin"_view, "available"_view));
  const Abstract& missing_alias =
      resources.resolve("resources/missing.bin"_view);
  EXPECT(&missing == &missing_alias);
  EXPECT(describes(missing, "resources/missing.bin could not be read"_view));

  Dynamic::Bytes invalid_route("inside/.."_view);
  const Abstract& invalid = resources.resolve(invalid_route);
  invalid_route.set('x');
  const Abstract& invalid_again = resources.resolve("inside/.."_view);
  const Abstract& empty_invalid = resources.resolve(View::Bytes());
  const Abstract& distinct_invalid = resources.resolve("."_view);
  EXPECT(&invalid == &invalid_again);
  EXPECT(&invalid != &empty_invalid);
  EXPECT(&invalid != &distinct_invalid);
  EXPECT(describes(invalid, "route is empty or lexically invalid"_view));

  const Abstract& rooted = resources.resolve("/outside.bin"_view);
  const Abstract& rooted_alias = resources.resolve("\\outside.bin"_view);
  EXPECT(&rooted == &rooted_alias);
  EXPECT(describes(rooted, "/outside.bin is not a confined"_view));

  const Abstract& directory = resources.resolve("resources"_view);
  EXPECT(describes(directory, "resources could not be read"_view));

  resources.seal();
  EXPECT(&resources.resolve("inside/.."_view) == &invalid);
  EXPECT(
      &resources.resolve("resources/new.bin"_view) == &Invalid::get_invalid());
}

PERIMORTEM_UNIT_TEST(PackageResources, monograph_dispatch) {
  static constexpr View::Bytes complete = "$[resources/table.bin]"_view;
  static constexpr View::Bytes partial = "$[resources/table.bin]tail"_view;
  Package::Language::Source sources[] = {
    Package::Language::Source("Member"_view, "member.ttx"_view),
    Package::Language::Source(complete, "shadow.ttx"_view),
    Package::Language::Source(partial, "partial.ttx"_view),
    Package::Language::Source("Qualified::Member"_view, "qualified.ttx"_view),
  };
  Allocator::Arena arena;
  Package::Dialect dialect;
  Errors errors;
  Tokenizer tokenizer(arena, {}, "package-resources.ttx"_view);
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);
  auto root_result = Package::Language::Monograph::create_authored(
      arena, dialect, Documentation::get_empty(), dialect, {}, sources);
  ASSERT(root_result);
  auto& root = *root_result;
  auto& member =
      arena.construct<ScopeMember>(arena, dialect, "Member value"_view);
  auto& shadow =
      arena.construct<ScopeMember>(arena, dialect, "Shadow value"_view);
  auto& partial_member =
      arena.construct<ScopeMember>(arena, dialect, "Partial value"_view);
  auto& qualified =
      arena.construct<ScopeMember>(arena, dialect, "Qualified value"_view);
  ASSERT(root.bind_member("Member"_view, member));
  ASSERT(root.bind_member(complete, shadow));
  ASSERT(root.bind_member(partial, partial_member));
  ASSERT(root.bind_member("Qualified::Member"_view, qualified));

  const Abstract& member_edge = root.resolve_context("Member"_view);
  const Abstract& partial_edge = root.resolve_context(partial);
  const Abstract& qualified_scope = root.resolve_context("Qualified"_view);
  const Abstract& qualified_edge =
      qualified_scope.resolve_context("Member"_view);
  EXPECT(&member_edge.resolve() == &member);
  EXPECT(&partial_edge.resolve() == &partial_member);
  EXPECT(&qualified_edge.resolve() == &qualified);
  EXPECT(
      &root.resolve_context("Qualified::Member"_view) ==
      &Invalid::get_invalid());
  EXPECT(&root.resolve_context(complete) == &Invalid::get_invalid());
  EXPECT(
      &root.resolve_context("$[resources/table.bin"_view) ==
      &Invalid::get_invalid());
  EXPECT(
      &root.resolve_context("resources/table.bin"_view) ==
      &Invalid::get_invalid());
  auto storage = Package::Storage::open(
      arena, "validation/data/ttx/package_resources"_view);
  ASSERT(storage);
  ASSERT(root.get_resources().connect(*storage));
  const Abstract& resource = root.resolve_context(complete);
  ASSERT(resource.is<Tetrodotoxin::Language::Resource>());
  root.get_resources().seal();
  EXPECT_NOT(root.get_resources().connect(*storage));
  EXPECT(&root.resolve_context(complete) == &resource);

  auto& source_free = Package::Language::Monograph::create_synthetic(
      arena, dialect, dialect, {});
  EXPECT_NOT(source_free.get_resources().connect(*storage));
  EXPECT(&source_free.resolve_context(complete) == &Invalid::get_invalid());
}
