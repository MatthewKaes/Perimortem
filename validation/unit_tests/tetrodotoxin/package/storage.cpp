// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/package/storage.hpp"

#include "validation/unit_test.hpp"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/package/content.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Validation;

static constexpr Count temporary_path_capacity = 160;
static constexpr Count cache_growth_count = 48;

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

static auto remove_member(View::Bytes base, View::Bytes member) -> void {
  Dynamic::Bytes path = join_path(base, member);
  File::remove(path);
}

static auto cleanup_tree(View::Bytes root) -> void {
  if (root.is_empty()) {
    return;
  }

  remove_member(root, "shared_a.ttx"_view);
  remove_member(root, "resource.bin"_view);
  remove_member(root, "empty.bin"_view);
  remove_member(root, "stable.bin"_view);
  remove_member(root, "replacement.bin"_view);
  remove_member(root, "later.bin"_view);
  remove_member(root, "identity.bin"_view);
  remove_member(root, "hard_a.bin"_view);
  remove_member(root, "hard_b.bin"_view);
  remove_member(root, "escape.bin"_view);
  remove_member(root, "cwd_only.bin"_view);
  remove_member(root, "outside.bin"_view);
  remove_member(root, "sources/main.ttx"_view);
  remove_member(root, "sources/local.bin"_view);
  remove_member(root, "sources"_view);
  remove_member(root, "directory"_view);

  for (Count i = 0; i < cache_growth_count; i++) {
    Static::Bytes<32> member;
    Signed_32 written = snprintf(
        Data::cast<char>(member.get_data()), member.get_size(),
        "cache_%02llu.bin", Unsigned_64(i));
    if (written <= 0 || Count(written) >= member.get_size()) {
      continue;
    }

    remove_member(root, member.slice(0, Count(written)));
  }

  File::remove(root);
}

class TemporaryPackage {
 public:
  TemporaryPackage() {
    Signed_32 root_written = snprintf(
        Data::cast<char>(root_path.get_data()), root_path.get_size(),
        "/tmp/tetrodotoxin_package_input_root_XXXXXX");
    if (root_written <= 0 || Count(root_written) >= root_path.get_size()) {
      return;
    }

    char* created_root = mkdtemp(Data::cast<char>(root_path.get_data()));
    if (created_root == nullptr) {
      return;
    }

    Signed_32 outside_written = snprintf(
        Data::cast<char>(outside_path.get_data()), outside_path.get_size(),
        "/tmp/tetrodotoxin_package_input_outside_XXXXXX");
    if (outside_written <= 0 ||
        Count(outside_written) >= outside_path.get_size()) {
      cleanup_tree(get_root());
      return;
    }

    char* created_outside = mkdtemp(Data::cast<char>(outside_path.get_data()));
    if (created_outside == nullptr) {
      cleanup_tree(get_root());
      return;
    }

    valid = True;
  }

  TemporaryPackage(const TemporaryPackage&) = delete;
  auto operator=(const TemporaryPackage&) -> TemporaryPackage& = delete;

  ~TemporaryPackage() {
    if (!valid) {
      return;
    }

    cleanup_tree(get_root());
    cleanup_tree(get_moved_root());
    cleanup_tree(get_outside_root());
  }

  operator bool() const { return bool(valid); }

  auto get_root() const -> View::Bytes {
    return NullTerminated::to_view(
        Data::cast<const char>(root_path.get_data()));
  }

  auto get_moved_root() const -> View::Bytes {
    return NullTerminated::to_view(
        Data::cast<const char>(moved_path.get_data()));
  }

  auto get_outside_root() const -> View::Bytes {
    return NullTerminated::to_view(
        Data::cast<const char>(outside_path.get_data()));
  }

  auto write(View::Bytes member, View::Bytes contents) const -> Bool {
    Dynamic::Bytes path = join_path(get_root(), member);
    return File::write(contents, path);
  }

  auto write_outside(View::Bytes member, View::Bytes contents) const -> Bool {
    Dynamic::Bytes path = join_path(get_outside_root(), member);
    return File::write(contents, path);
  }

  auto remove(View::Bytes member) const -> Bool {
    Dynamic::Bytes path = join_path(get_root(), member);
    return File::remove(path);
  }

  auto create_directory(View::Bytes member) const -> Bool {
    Dynamic::Bytes path = join_path(get_root(), member);
    Signed_32 created = mkdir(native_path(path), S_IRWXU);
    return created == 0;
  }

  auto create_escape_link(View::Bytes member, View::Bytes outside_member) const
      -> Bool {
    Dynamic::Bytes path = join_path(get_root(), member);
    Dynamic::Bytes target = join_path(get_outside_root(), outside_member);
    Signed_32 linked = symlink(native_path(target), native_path(path));
    return linked == 0;
  }

  auto create_hard_link(View::Bytes existing_member, View::Bytes linked_member)
      const -> Bool {
    Dynamic::Bytes existing = join_path(get_root(), existing_member);
    Dynamic::Bytes linked = join_path(get_root(), linked_member);
    Signed_32 created = link(native_path(existing), native_path(linked));
    return created == 0;
  }

  auto replace(View::Bytes replacement_member, View::Bytes target_member) const
      -> Bool {
    Dynamic::Bytes replacement = join_path(get_root(), replacement_member);
    Dynamic::Bytes target = join_path(get_root(), target_member);
    Signed_32 moved = rename(native_path(replacement), native_path(target));
    return moved == 0;
  }

  auto rename_root() -> Bool {
    Signed_32 written = snprintf(
        Data::cast<char>(moved_path.get_data()), moved_path.get_size(),
        "%s_moved", Data::cast<const char>(root_path.get_data()));
    if (written <= 0 || Count(written) >= moved_path.get_size()) {
      return False;
    }

    Signed_32 moved = rename(
        Data::cast<const char>(root_path.get_data()),
        Data::cast<const char>(moved_path.get_data()));
    return moved == 0;
  }

  auto create_replacement_root() const -> Bool {
    Signed_32 created =
        mkdir(Data::cast<const char>(root_path.get_data()), S_IRWXU);
    return created == 0;
  }

 private:
  Static::Bytes<temporary_path_capacity> root_path;
  Static::Bytes<temporary_path_capacity> moved_path;
  Static::Bytes<temporary_path_capacity> outside_path;
  Bool valid = False;
};

class WorkingDirectory {
 public:
  WorkingDirectory(View::Bytes location) {
    descriptor = open(".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (descriptor < 0) {
      return;
    }

    Dynamic::Bytes native(location);
    Signed_32 changed = chdir(native_path(native));
    entered = changed == 0;
  }

  WorkingDirectory(const WorkingDirectory&) = delete;
  auto operator=(const WorkingDirectory&) -> WorkingDirectory& = delete;

  ~WorkingDirectory() {
    if (descriptor < 0) {
      return;
    }

    fchdir(descriptor);
    close(descriptor);
  }

  operator bool() const { return bool(entered); }

  auto restore() -> Bool {
    if (descriptor < 0) {
      return False;
    }

    Signed_32 restored = fchdir(descriptor);
    Signed_32 closed = close(descriptor);
    descriptor = -1;
    entered = False;
    return restored == 0 && closed == 0;
  }

 private:
  Signed_32 descriptor = -1;
  Bool entered = False;
};

struct TestConsumer {
  View::Bytes contents;
};

static Harness PackageStorage = {
  .name = "Tetrodotoxin::Package::Storage"_view,
};

PERIMORTEM_UNIT_TEST(PackageStorage, source_and_resources) {
  static constexpr View::Bytes root =
      "validation/data/ttx/package_resources"_view;
  static constexpr View::Bytes expected_header =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/"_view;
  Allocator::Arena arena;
  Dynamic::Bytes caller_route("resources/cache/../table.bin"_view);
  auto storage = Package::Storage::open(arena, root);
  ASSERT(storage);

  auto source = (*storage).read("./shared_a.ttx"_view);
  auto first_resource = (*storage).read(caller_route);
  ASSERT(source);
  ASSERT(first_resource);

  caller_route.set('x');
  auto second_resource = (*storage).read("resources/./table.bin"_view);
  auto empty = (*storage).read("resources/empty.bin"_view);
  ASSERT(second_resource);
  ASSERT(empty);

  EXPECT_TEXT((*source).get_diagnostic_path(), "shared_a.ttx"_view);
  EXPECT_NOT((*source).get_contents().is_empty());
  EXPECT_TEXT(
      (*first_resource).get_diagnostic_path(), "resources/table.bin"_view);
  EXPECT(
      (*first_resource).get_contents().slice(0, expected_header.get_size()) ==
      expected_header);
  EXPECT(
      (*first_resource).get_contents().get_data() ==
      (*second_resource).get_contents().get_data());
  EXPECT(
      (*first_resource).get_diagnostic_path().get_data() ==
      (*second_resource).get_diagnostic_path().get_data());
  EXPECT(&*first_resource == &*second_resource);
  EXPECT_TEXT((*empty).get_diagnostic_path(), "resources/empty.bin"_view);
  EXPECT((*empty).get_contents().is_empty());

  TestConsumer first_consumer{(*first_resource).get_contents()};
  TestConsumer second_consumer{(*second_resource).get_contents()};
  EXPECT(&first_consumer != &second_consumer);
  EXPECT(
      first_consumer.contents.get_data() ==
      second_consumer.contents.get_data());
}

PERIMORTEM_UNIT_TEST(PackageStorage, content_stability) {
  auto frozen = File::read(
      "validation/data/ttx/package_resources/resources/table.bin"_view);
  ASSERT(frozen);

  TemporaryPackage temporary;
  ASSERT(temporary);
  ASSERT(temporary.write("stable.bin"_view, *frozen));

  Allocator::Arena arena;
  auto storage = Package::Storage::open(arena, temporary.get_root());
  ASSERT(storage);

  auto original = (*storage).read("stable.bin"_view);
  ASSERT(original);
  const Unsigned_8* original_identity = (*original).get_contents().get_data();

  ASSERT(temporary.write("stable.bin"_view, "mutated"_view));
  auto after_mutation = (*storage).read("./stable.bin"_view);
  ASSERT(after_mutation);
  EXPECT_TEXT((*after_mutation).get_contents(), (*frozen).get_view());
  EXPECT((*after_mutation).get_contents().get_data() == original_identity);

  ASSERT(temporary.write("replacement.bin"_view, "replacement"_view));
  ASSERT(temporary.replace("replacement.bin"_view, "stable.bin"_view));
  auto after_replacement = (*storage).read("stable.bin"_view);
  ASSERT(after_replacement);
  EXPECT_TEXT((*after_replacement).get_contents(), (*frozen).get_view());
  EXPECT((*after_replacement).get_contents().get_data() == original_identity);

  ASSERT(temporary.remove("stable.bin"_view));
  auto after_removal = (*storage).read("stable.bin"_view);
  ASSERT(after_removal);
  EXPECT_TEXT((*after_removal).get_contents(), (*frozen).get_view());
  EXPECT((*after_removal).get_contents().get_data() == original_identity);
}

PERIMORTEM_UNIT_TEST(PackageStorage, retry_after_failure) {
  TemporaryPackage temporary;
  ASSERT(temporary);

  Allocator::Arena arena;
  auto storage = Package::Storage::open(arena, temporary.get_root());
  ASSERT(storage);
  EXPECT_NOT((*storage).read("later.bin"_view));

  ASSERT(temporary.write("later.bin"_view, "available"_view));
  auto available = (*storage).read("later.bin"_view);
  ASSERT(available);
  EXPECT_TEXT((*available).get_contents(), "available"_view);
}

PERIMORTEM_UNIT_TEST(PackageStorage, cache_growth_and_move) {
  TemporaryPackage temporary;
  ASSERT(temporary);
  ASSERT(temporary.write("stable.bin"_view, "stable"_view));

  Allocator::Arena arena;
  auto opened = Package::Storage::open(arena, temporary.get_root());
  ASSERT(opened);

  auto stable = (*opened).read("stable.bin"_view);
  ASSERT(stable);
  Package::Content* stable_content = &*stable;
  View::Bytes stable_path = (*stable).get_diagnostic_path();
  View::Bytes stable_contents = (*stable).get_contents();

  for (Count i = 0; i < cache_growth_count; i++) {
    Static::Bytes<32> member;
    Signed_32 written = snprintf(
        Data::cast<char>(member.get_data()), member.get_size(),
        "cache_%02llu.bin", Unsigned_64(i));
    ASSERT(written > 0 && Count(written) < member.get_size());

    View::Bytes route = member.slice(0, Count(written));
    ASSERT(temporary.write(route, route));
    auto cached = (*opened).read(route);
    ASSERT(cached);
    EXPECT_TEXT((*cached).get_diagnostic_path(), route);
    EXPECT_TEXT((*cached).get_contents(), route);
  }

  EXPECT_TEXT(stable_path, "stable.bin"_view);
  EXPECT_TEXT(stable_contents, "stable"_view);

  Package::Storage moved(static_cast<Package::Storage&&>(*opened));
  auto repeated = moved.read("./stable.bin"_view);
  ASSERT(repeated);
  EXPECT(&*repeated == stable_content);
  EXPECT(
      (*repeated).get_diagnostic_path().get_data() == stable_path.get_data());
  EXPECT((*repeated).get_contents().get_data() == stable_contents.get_data());
}

PERIMORTEM_UNIT_TEST(PackageStorage, opened_root_identity) {
  TemporaryPackage temporary;
  ASSERT(temporary);
  ASSERT(temporary.write("identity.bin"_view, "original root"_view));

  Allocator::Arena arena;
  auto storage = Package::Storage::open(arena, temporary.get_root());
  ASSERT(storage);
  ASSERT(temporary.rename_root());
  ASSERT(temporary.create_replacement_root());
  ASSERT(temporary.write("identity.bin"_view, "replacement root"_view));

  auto identity = (*storage).read("identity.bin"_view);
  ASSERT(identity);
  EXPECT_TEXT((*identity).get_contents(), "original root"_view);
}

PERIMORTEM_UNIT_TEST(PackageStorage, route_rejections) {
  TemporaryPackage temporary;
  ASSERT(temporary);
  ASSERT(temporary.create_directory("directory"_view));
  ASSERT(temporary.create_directory("sources"_view));
  ASSERT(temporary.write("sources/main.ttx"_view, "source"_view));
  ASSERT(temporary.write("sources/local.bin"_view, "local"_view));
  ASSERT(temporary.write_outside("outside.bin"_view, "outside"_view));
  ASSERT(temporary.write_outside("cwd_only.bin"_view, "cwd"_view));
  ASSERT(temporary.create_escape_link("escape.bin"_view, "outside.bin"_view));

  Allocator::Arena arena;
  auto storage = Package::Storage::open(arena, temporary.get_root());
  ASSERT(storage);

  EXPECT_NOT((*storage).read(View::Bytes()));
  EXPECT_NOT((*storage).read("."_view));
  EXPECT_NOT((*storage).read("inside/.."_view));
  EXPECT_NOT((*storage).read("/absolute.bin"_view));
  EXPECT_NOT((*storage).read("\\rooted.bin"_view));
  EXPECT_NOT((*storage).read("../outside.bin"_view));
  EXPECT_NOT((*storage).read("inside/../../outside.bin"_view));
  EXPECT_NOT((*storage).read("missing.bin"_view));
  EXPECT_NOT((*storage).read("directory"_view));
  EXPECT_NOT((*storage).read("escape.bin"_view));

  Static::Bytes<9> nul_route{'n', 'u', 'l', 'l', '\0', '.', 'b', 'i', 'n'};
  EXPECT_NOT((*storage).read(nul_route));

  auto source = (*storage).read("sources/main.ttx"_view);
  ASSERT(source);
  EXPECT_NOT((*storage).read("local.bin"_view));

  WorkingDirectory outside(temporary.get_outside_root());
  ASSERT(outside);
  auto cwd_fallback = (*storage).read("cwd_only.bin"_view);
  Bool restored = outside.restore();
  EXPECT_NOT(cwd_fallback);
  EXPECT(restored);
}

PERIMORTEM_UNIT_TEST(PackageStorage, hard_link_routes) {
  TemporaryPackage temporary;
  ASSERT(temporary);
  ASSERT(temporary.write("hard_a.bin"_view, "same bytes"_view));
  ASSERT(temporary.create_hard_link("hard_a.bin"_view, "hard_b.bin"_view));

  Allocator::Arena arena;
  auto storage = Package::Storage::open(arena, temporary.get_root());
  ASSERT(storage);

  auto first = (*storage).read("hard_a.bin"_view);
  auto second = (*storage).read("hard_b.bin"_view);
  ASSERT(first);
  ASSERT(second);

  EXPECT_TEXT((*first).get_contents(), (*second).get_contents());
  EXPECT(
      (*first).get_contents().get_data() !=
      (*second).get_contents().get_data());
  EXPECT(
      (*first).get_diagnostic_path().get_data() !=
      (*second).get_diagnostic_path().get_data());

  auto repeated = (*storage).read("./hard_a.bin"_view);
  ASSERT(repeated);
  EXPECT(&*first != &*second);
  EXPECT(&*first == &*repeated);
  EXPECT(
      (*first).get_contents().get_data() ==
      (*repeated).get_contents().get_data());
}
