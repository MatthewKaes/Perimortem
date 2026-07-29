// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/file.hpp"

#include "validation/unit_test.hpp"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Validation;

constexpr auto test_file = ".bin/bin/validation/system_file_test.json"_view;
constexpr auto test_output =
    ".bin/bin/validation/system_file_test_out.json"_view;
constexpr auto test_replacement =
    ".bin/bin/validation/system_file_test_replacement.json"_view;
constexpr auto test_contents = "{\"value\":42}"_view;
constexpr auto replacement_contents = "{\"other\":24}"_view;

static constexpr const char* test_output_path =
    ".bin/bin/validation/system_file_test_out.json";
static constexpr const char* test_replacement_path =
    ".bin/bin/validation/system_file_test_replacement.json";

enum class TraceMutation {
  ReplacePath,
  TruncateAfterMetadata,
};

static auto is_open_call(Signed_64 call) -> Bool {
  return call == SYS_open || call == SYS_openat;
}

static auto is_metadata_call(Signed_64 call) -> Bool {
  return call == SYS_stat || call == SYS_fstat || call == SYS_newfstatat;
}

static auto traced_child(
    const char* path,
    View::Bytes expected,
    Bool expected_present) -> void {
  if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) != 0) {
    _exit(20);
  }

  raise(SIGSTOP);

  auto source = File::read(NullTerminated::to_view(path));
  if (!expected_present) {
    _exit(source ? 1 : 0);
  }

  _exit(source && *source == expected ? 0 : 1);
}

static auto mutate_path(
    TraceMutation mutation,
    const char* path,
    const char* replacement) -> Bool {
  if (mutation == TraceMutation::ReplacePath) {
    int renamed = rename(replacement, path);
    return renamed == 0;
  }

  int truncated = truncate(path, 0);
  return truncated == 0;
}

static auto trace_read(
    const char* path,
    View::Bytes expected,
    Bool expected_present,
    TraceMutation mutation,
    const char* replacement = nullptr) -> Bool {
  pid_t child = fork();
  if (child == 0) {
    traced_child(path, expected, expected_present);
  }

  if (child < 0) {
    return False;
  }

  // Stop at every child system call so the pathname can change at an exact
  // boundary without adding a production test seam.
  int status = 0;
  pid_t waited = waitpid(child, &status, 0);
  if (waited != child || !WIFSTOPPED(status)) {
    return False;
  }

  long option_result =
      ptrace(PTRACE_SETOPTIONS, child, nullptr, PTRACE_O_TRACESYSGOOD);
  if (option_result != 0) {
    kill(child, SIGKILL);
    waitpid(child, &status, 0);
    return False;
  }

  Bool entering = True;
  Bool opened = False;
  Bool mutated = False;
  while (true) {
    long trace_result = ptrace(PTRACE_SYSCALL, child, nullptr, nullptr);
    if (trace_result != 0) {
      kill(child, SIGKILL);
      waitpid(child, &status, 0);
      return False;
    }

    waited = waitpid(child, &status, 0);
    if (waited != child) {
      return False;
    }

    if (WIFEXITED(status)) {
      return mutated && WEXITSTATUS(status) == 0;
    }

    if (!WIFSTOPPED(status) || WSTOPSIG(status) != (SIGTRAP | 0x80)) {
      continue;
    }

    struct user_regs_struct registers;
    long registers_result = ptrace(PTRACE_GETREGS, child, nullptr, &registers);
    if (registers_result != 0) {
      kill(child, SIGKILL);
      waitpid(child, &status, 0);
      return False;
    }

    Signed_64 call = Signed_64(registers.orig_rax);
    if (entering && mutation == TraceMutation::TruncateAfterMetadata &&
        opened && call == SYS_read && !mutated) {
      mutated = mutate_path(mutation, path, replacement);
    }

    entering = !entering;
    if (entering) {
      Signed_64 call_result = Signed_64(registers.rax);
      if (is_open_call(call) && call_result >= 0) {
        opened = True;
        if (mutation == TraceMutation::ReplacePath && !mutated) {
          mutated = mutate_path(mutation, path, replacement);
        }
      } else if (is_metadata_call(call) && call_result == 0) {
        if (mutation == TraceMutation::ReplacePath && !opened && !mutated) {
          mutated = mutate_path(mutation, path, replacement);
        } else if (
            mutation == TraceMutation::TruncateAfterMetadata && opened &&
            !mutated) {
          mutated = mutate_path(mutation, path, replacement);
        }
      }
    }
  }
}

static Harness SystemFile = {
  .name = "System::File"_view,
  .setup = []() { File::write(test_contents, test_file); },
  .teardown =
      []() {
        File::remove(test_file);
        File::remove(test_output);
        File::remove(test_replacement);
      },
};

PERIMORTEM_UNIT_TEST(SystemFile, read) {
  auto source = File::read(test_file);
  ASSERT(source);
  EXPECT_TEXT(*source, test_contents);
}

PERIMORTEM_UNIT_TEST(SystemFile, write) {
  Bool written = File::write(test_contents, test_output);
  ASSERT(written);

  auto source = File::read(test_output);
  ASSERT(source);
  EXPECT_TEXT(*source, test_contents);
}

PERIMORTEM_UNIT_TEST(SystemFile, empty) {
  Bool written = File::write(View::Bytes(), test_output);
  ASSERT(written);

  auto source = File::read(test_output);
  ASSERT(source);
  EXPECT((*source).is_empty());
}

PERIMORTEM_UNIT_TEST(SystemFile, missing) {
  File::remove(test_output);

  auto source = File::read(test_output);
  EXPECT_NOT(source);

  Bool removed = File::remove(test_output);
  EXPECT_NOT(removed);
}

PERIMORTEM_UNIT_TEST(SystemFile, unreadable) {
  Bool written = File::write(test_contents, test_output);
  ASSERT(written);

  int restricted = chmod(test_output_path, 0);
  ASSERT_EQ(restricted, 0);

  auto source = File::read(test_output);

  int restored = chmod(test_output_path, S_IRUSR | S_IWUSR);
  EXPECT_EQ(restored, 0);
  EXPECT_NOT(source);
}

PERIMORTEM_UNIT_TEST(SystemFile, oversized) {
  int descriptor =
      open(test_output_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  ASSERT(descriptor >= 0);

  constexpr off_t oversized_file = (off_t(1) << 35) + 1;
  int truncated = ftruncate(descriptor, oversized_file);
  int closed = close(descriptor);
  ASSERT_EQ(truncated, 0);
  ASSERT_EQ(closed, 0);

  EXPECT_NOT(File::read(test_output));
}

PERIMORTEM_UNIT_TEST(SystemFile, non_regular) {
  EXPECT_NOT(File::read(".bin/bin/validation"_view));
}

PERIMORTEM_UNIT_TEST(SystemFile, short_read) {
  Bool written = File::write(test_contents, test_output);
  ASSERT(written);

  EXPECT(trace_read(
      test_output_path, View::Bytes(), False,
      TraceMutation::TruncateAfterMetadata));
}

PERIMORTEM_UNIT_TEST(SystemFile, same_opened_object) {
  Bool original_written = File::write(test_contents, test_output);
  ASSERT(original_written);

  Bool replacement_written =
      File::write(replacement_contents, test_replacement);
  ASSERT(replacement_written);

  EXPECT(trace_read(
      test_output_path, test_contents, True, TraceMutation::ReplacePath,
      test_replacement_path));
}

PERIMORTEM_UNIT_TEST(SystemFile, exists) {
  EXPECT(File::exists(test_file));
  EXPECT_NOT(File::exists("perimortem/"_view));
  EXPECT_NOT(File::exists("perimortem"_view));
}

PERIMORTEM_UNIT_TEST(SystemFile, remove) {
  Bool written = File::write(test_contents, test_output);
  ASSERT(written);

  Bool removed = File::remove(test_output);
  ASSERT(removed);
  EXPECT_NOT(File::exists(test_output));
}
