// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/parser/package/workspace.hpp"

#include "validation/unit_test.hpp"

#ifdef PERI_LINUX
#include <fcntl.h>
#include <ftw.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#else
#error Tetrodotoxin package tests require the Linux confined Workspace.
#endif

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/data.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Parser::Package;
using namespace Validation;

static Harness TetrodotoxinPackageTests = {
  .name = "Tetrodotoxin::Parser::Package::Workspace"_view,
};

static auto remove_temporary_entry(
    const char* path,
    const struct stat*,
    int,
    struct FTW*) -> int {
  return ::remove(path);
}

// TemporaryDirectory creates a private physical oracle for Workspace tests.
// Cleanup never follows symlinks and removes only the exact mkdtemp root.
class TemporaryDirectory {
 public:
  TemporaryDirectory() {
    char pattern[] = "/tmp/ttx-package-XXXXXX";
    char* created = ::mkdtemp(pattern);
    if (created != nullptr) {
      root = View::Bytes(
          Data::cast<const Unsigned_8>(created), Count(::strlen(created)));
    }
  }

  TemporaryDirectory(const TemporaryDirectory&) = delete;
  auto operator=(const TemporaryDirectory&) -> TemporaryDirectory& = delete;

  ~TemporaryDirectory() {
    if (root.is_empty()) {
      return;
    }

    Dynamic::Bytes location = c_path();
    int removed = ::nftw(
        Data::cast<const char>(location.get_view().get_data()),
        remove_temporary_entry, 32, FTW_DEPTH | FTW_PHYS);
    (void)removed;
  }

  auto get_path() const -> View::Bytes { return root; }

  auto c_path(View::Bytes route = View::Bytes()) const -> Dynamic::Bytes {
    Dynamic::Bytes path(root);
    if (!route.is_empty()) {
      path.append('/');
      path.concat(route);
    }
    path.append('\0');
    return path;
  }

  auto directory(View::Bytes route) -> Bool {
    Dynamic::Bytes path = c_path(route);
    int created =
        ::mkdir(Data::cast<const char>(path.get_view().get_data()), 0700);
    return created == 0;
  }

  auto write(View::Bytes route, View::Bytes contents) -> Bool {
    Dynamic::Bytes path = c_path(route);
    int file = ::open(
        Data::cast<const char>(path.get_view().get_data()),
        O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (file == -1) {
      return False;
    }

    Count written = 0;
    while (written < contents.get_size()) {
      Signed_64 amount = Signed_64(
          ::write(
              file, contents.get_data() + written,
              contents.get_size() - written));
      if (amount == -1) {
        int closed = ::close(file);
        (void)closed;
        return False;
      }

      written += Count(amount);
    }

    int closed = ::close(file);
    return closed == 0;
  }

  auto symbolic_link(View::Bytes target, View::Bytes route) -> Bool {
    Dynamic::Bytes target_path(target);
    target_path.append('\0');
    Dynamic::Bytes link_path = c_path(route);
    int linked = ::symlink(
        Data::cast<const char>(target_path.get_view().get_data()),
        Data::cast<const char>(link_path.get_view().get_data()));
    return linked == 0;
  }

  auto permissions(View::Bytes route, mode_t mode) -> Bool {
    Dynamic::Bytes path = c_path(route);
    int changed =
        ::chmod(Data::cast<const char>(path.get_view().get_data()), mode);
    return changed == 0;
  }

 private:
  Dynamic::Bytes root;
};

static auto succeeds_with(
    const Workspace& workspace,
    View::Bytes route,
    View::Bytes expected) -> Bool {
  auto read = workspace.read(route);
  return read.visit(
      [&](View::Bytes bytes) { return bytes == expected; },
      [](Workspace::Failure) { return False; });
}

static auto fails_with(
    const Workspace& workspace,
    View::Bytes route,
    Workspace::Failure expected) -> Bool {
  auto read = workspace.read(route);
  return read.visit(
      [](View::Bytes) { return False; },
      [&](Workspace::Failure failure) { return Bool(failure == expected); });
}

PERIMORTEM_UNIT_TEST(
    TetrodotoxinPackageTests,
    confines_complete_reads_and_distinguishes_empty_data) {
  TemporaryDirectory package;
  TemporaryDirectory outside;
  ASSERT_NOT(package.get_path().is_empty());
  ASSERT_NOT(outside.get_path().is_empty());
  ASSERT(package.directory("nested"_view));
  ASSERT(package.directory("folder"_view));
  ASSERT(package.write("member.ttx"_view, "dialect : Library;\n"_view));
  ASSERT(package.write("empty.ttx"_view, View::Bytes()));
  ASSERT(package.write("locked.ttx"_view, "secret"_view));
  ASSERT(package.write("nested/neighbor.bin"_view, "neighbor"_view));
  ASSERT(package.symbolic_link("member.ttx"_view, "inside-link.ttx"_view));
  ASSERT(outside.write("outside.ttx"_view, "outside"_view));
  Dynamic::Bytes outside_file = outside.c_path("outside.ttx"_view);
  View::Bytes outside_target =
      outside_file.get_view().slice(0, outside_file.get_size() - 1);
  ASSERT(package.symbolic_link(outside_target, "outside-link.ttx"_view));
  ASSERT(package.permissions("locked.ttx"_view, 0000));

  auto opened = Workspace::open(package.get_path());
  Bool verified = opened.visit(
      [&](const Workspace& workspace) {
        Bool result = True;
        result &= succeeds_with(
            workspace, "member.ttx"_view, "dialect : Library;\n"_view);
        result &= succeeds_with(
            workspace, "inside-link.ttx"_view, "dialect : Library;\n"_view);
        result &= succeeds_with(workspace, "empty.ttx"_view, View::Bytes());
        result &= fails_with(
            workspace, "missing.ttx"_view, Workspace::Failure::Missing);
        result &=
            fails_with(workspace, "folder"_view, Workspace::Failure::NonFile);
        result &= fails_with(
            workspace, "locked.ttx"_view, Workspace::Failure::Unreadable);
        result &= fails_with(
            workspace, "outside-link.ttx"_view,
            Workspace::Failure::OutsideRoot);
        result &= fails_with(
            workspace, "/absolute.ttx"_view, Workspace::Failure::OutsideRoot);
        result &= fails_with(
            workspace, "../outside.ttx"_view, Workspace::Failure::OutsideRoot);
        result &= fails_with(
            workspace, "neighbor.bin"_view, Workspace::Failure::Missing);
        return result;
      },
      [](Workspace::Failure) { return False; });

  EXPECT(verified);
  EXPECT(package.permissions("locked.ttx"_view, 0600));
}

PERIMORTEM_UNIT_TEST(
    TetrodotoxinPackageTests,
    never_falls_back_to_the_process_working_directory) {
  TemporaryDirectory package;
  TemporaryDirectory working;
  ASSERT(working.write("cwd-only.bin"_view, "wrong root"_view));
  int previous_directory = ::open(".", O_PATH | O_DIRECTORY | O_CLOEXEC);
  ASSERT(previous_directory != -1);
  Dynamic::Bytes working_path = working.c_path();
  int changed =
      ::chdir(Data::cast<const char>(working_path.get_view().get_data()));
  ASSERT_EQ(changed, 0);

  auto opened = Workspace::open(package.get_path());
  Bool isolated = opened.visit(
      [](const Workspace& workspace) {
        return fails_with(
            workspace, "cwd-only.bin"_view, Workspace::Failure::Missing);
      },
      [](Workspace::Failure) { return False; });

  int restored = ::fchdir(previous_directory);
  int closed = ::close(previous_directory);
  EXPECT_EQ(restored, 0);
  EXPECT_EQ(closed, 0);
  EXPECT(isolated);
}

PERIMORTEM_UNIT_TEST(
    TetrodotoxinPackageTests,
    package_roots_isolate_the_same_logical_route) {
  TemporaryDirectory first;
  TemporaryDirectory second;
  ASSERT(first.write("shared.bin"_view, "first package"_view));
  ASSERT(second.write("shared.bin"_view, "second package"_view));
  auto first_opened = Workspace::open(first.get_path());
  auto second_opened = Workspace::open(second.get_path());

  Bool isolated = first_opened.visit(
      [&](const Workspace& first_workspace) {
        return second_opened.visit(
            [&](const Workspace& second_workspace) {
              return Bool(
                  succeeds_with(
                      first_workspace, "shared.bin"_view,
                      "first package"_view) &&
                  succeeds_with(
                      second_workspace, "shared.bin"_view,
                      "second package"_view));
            },
            [](Workspace::Failure) { return False; });
      },
      [](Workspace::Failure) { return False; });

  EXPECT(isolated);
}

PERIMORTEM_UNIT_TEST(
    TetrodotoxinPackageTests,
    closed_results_transfer_their_selected_owner) {
  TemporaryDirectory first;
  TemporaryDirectory second;
  ASSERT(first.write("selected.bin"_view, "first bytes"_view));
  ASSERT(second.write("selected.bin"_view, "second bytes"_view));
  auto first_opened = Workspace::open(first.get_path());
  auto second_opened = Workspace::open(second.get_path());

  second_opened = Data::take(first_opened);
  Bool transferred = second_opened.visit(
      [](const Workspace& workspace) {
        auto first_read = workspace.read("selected.bin"_view);
        auto second_read = workspace.read("selected.bin"_view);
        second_read = Data::take(first_read);
        return second_read.visit(
            [](View::Bytes bytes) { return bytes == "first bytes"_view; },
            [](Workspace::Failure) { return False; });
      },
      [](Workspace::Failure) { return False; });

  EXPECT(transferred);
}

PERIMORTEM_UNIT_TEST(
    TetrodotoxinPackageTests,
    symlink_replacement_cannot_select_outside_bytes) {
  TemporaryDirectory package;
  TemporaryDirectory outside;
  ASSERT(package.write("inside.bin"_view, "inside bytes"_view));
  ASSERT(outside.write("outside.bin"_view, "outside bytes"_view));
  ASSERT(package.symbolic_link("inside.bin"_view, "selected.bin"_view));
  Dynamic::Bytes selected = package.c_path("selected.bin"_view);
  Dynamic::Bytes replacement = package.c_path("replacement.bin"_view);
  Dynamic::Bytes outside_file = outside.c_path("outside.bin"_view);

  auto opened = Workspace::open(package.get_path());
  Bool safe = opened.visit(
      [&](const Workspace& workspace) {
        pid_t writer = ::fork();
        if (writer == -1) {
          return False;
        }
        if (writer == 0) {
          const char* selected_path =
              Data::cast<const char>(selected.get_view().get_data());
          const char* replacement_path =
              Data::cast<const char>(replacement.get_view().get_data());
          const char* outside_path =
              Data::cast<const char>(outside_file.get_view().get_data());
          while (True) {
            int unlinked = ::unlink(replacement_path);
            (void)unlinked;
            int linked = ::symlink("inside.bin", replacement_path);
            if (linked == 0) {
              int replaced = ::rename(replacement_path, selected_path);
              (void)replaced;
            }

            unlinked = ::unlink(replacement_path);
            (void)unlinked;
            linked = ::symlink(outside_path, replacement_path);
            if (linked == 0) {
              int replaced = ::rename(replacement_path, selected_path);
              (void)replaced;
            }
          }
        }

        Bool result = True;
        for (Count i = 0; i < 2000; i++) {
          auto read = workspace.read("selected.bin"_view);
          Bool observation = read.visit(
              [](View::Bytes bytes) { return bytes == "inside bytes"_view; },
              [](Workspace::Failure failure) {
                return Bool(
                    failure == Workspace::Failure::OutsideRoot ||
                    failure == Workspace::Failure::Missing);
              });
          result &= observation;
        }

        int stopped = ::kill(writer, SIGTERM);
        int status = 0;
        pid_t waited = ::waitpid(writer, &status, 0);
        result &= stopped == 0;
        result &= waited == writer;
        return result;
      },
      [](Workspace::Failure) { return False; });

  EXPECT(safe);
}

PERIMORTEM_UNIT_TEST(
    TetrodotoxinPackageTests,
    root_acquisition_rejects_missing_and_non_directory) {
  TemporaryDirectory package;
  ASSERT(package.write("file.bin"_view, "file"_view));
  Dynamic::Bytes missing = package.c_path("missing"_view);
  Dynamic::Bytes file = package.c_path("file.bin"_view);
  View::Bytes missing_path =
      missing.get_view().slice(0, missing.get_size() - 1);
  View::Bytes file_path = file.get_view().slice(0, file.get_size() - 1);

  auto missing_open = Workspace::open(missing_path);
  Bool missing_rejected = missing_open.visit(
      [](const Workspace&) { return False; },
      [](Workspace::Failure failure) {
        return Bool(failure == Workspace::Failure::Missing);
      });
  auto file_open = Workspace::open(file_path);
  Bool file_rejected = file_open.visit(
      [](const Workspace&) { return False; },
      [](Workspace::Failure failure) {
        return Bool(failure == Workspace::Failure::NonFile);
      });

  EXPECT(missing_rejected);
  EXPECT(file_rejected);
}
