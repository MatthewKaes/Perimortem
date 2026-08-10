// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/process/child.hpp"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/file.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Validation;

static constexpr View::Bytes event_output =
    "prepare\n"
    "submit top\n"
    "submit bottom\n"
    "release\n"_view;

static auto write_all(Signed_32 descriptor, View::Bytes bytes) -> Bool {
  Count offset = 0;
  while (offset < bytes.get_size()) {
    ssize_t count =
        write(descriptor, bytes.get_data() + offset, bytes.get_size() - offset);
    if (count > 0) {
      offset += Count(count);
      continue;
    }

    if (count < 0 && errno == EINTR) {
      continue;
    }

    return False;
  }

  return True;
}

static auto read_all(Signed_32 descriptor) -> Dynamic::Bytes {
  Dynamic::Bytes bytes;
  Static::Vector<Unsigned_8, 4096> buffer;
  while (true) {
    ssize_t count = read(descriptor, buffer.get_data(), buffer.get_size());
    if (count > 0) {
      bytes.concat(View::Bytes(buffer.get_data(), Count(count)));
      continue;
    }

    if (count < 0 && errno == EINTR) {
      continue;
    }

    return bytes;
  }
}

static auto child_mode(View::Bytes mode) -> Signed_32 {
  if (mode == "echo"_view) {
    Dynamic::Bytes input = read_all(STDIN_FILENO);
    if (!(input == "hello\nquit\n"_view)) {
      write_all(STDERR_FILENO, "unexpected stdin\n"_view);
      return 2;
    }

    write_all(STDOUT_FILENO, "Echo: hello\n"_view);
    return 0;
  }

  if (mode == "pass"_view) {
    write_all(STDOUT_FILENO, event_output);
    return 0;
  }

  if (mode == "missing"_view) {
    write_all(STDOUT_FILENO, "prepare\nsubmit top\nrelease\n"_view);
    return 0;
  }

  if (mode == "reordered"_view) {
    write_all(
        STDOUT_FILENO, "prepare\nsubmit bottom\nsubmit top\nrelease\n"_view);
    return 0;
  }

  if (mode == "duplicate"_view) {
    write_all(
        STDOUT_FILENO,
        "prepare\nsubmit top\nsubmit top\nsubmit bottom\nrelease\n"_view);
    return 0;
  }

  if (mode == "unexpected"_view) {
    write_all(
        STDOUT_FILENO,
        "prepare\nsubmit top\nsurprise\nsubmit bottom\nrelease\n"_view);
    return 0;
  }

  if (mode == "wrong_stream"_view) {
    write_all(STDERR_FILENO, event_output);
    return 0;
  }

  if (mode == "wrong_exit"_view) {
    write_all(STDOUT_FILENO, event_output);
    return 7;
  }

  if (mode == "timeout"_view) {
    while (true) {
      pause();
    }
  }

  write_all(STDERR_FILENO, "unknown child mode\n"_view);
  return 3;
}

static auto executable_path() -> Dynamic::Bytes {
  Static::Vector<Unsigned_8, 4096> path;
  ssize_t size = readlink(
      "/proc/self/exe", Data::cast<char>(path.get_data()), path.get_size());
  if (size <= 0 || Count(size) == path.get_size()) {
    return Dynamic::Bytes();
  }

  return Dynamic::Bytes(View::Bytes(path.get_data(), Count(size)));
}

static auto observe(
    View::Bytes executable,
    View::Bytes mode,
    View::Bytes input = View::Bytes(),
    Unsigned_64 timeout_nanoseconds = 1'000'000'000) -> Process::Observation {
  Static::Vector<View::Bytes, 2> arguments = {{"--child"_view, mode}};
  Process::Request request = {
    .executable = executable,
    .arguments = arguments,
    .standard_input = input,
    .timeout_nanoseconds = timeout_nanoseconds,
  };
  return Process::run(request);
}

static auto
    record(const char* name, Bool accepted, Signed_32& passed, Signed_32& total)
        -> void {
  total += 1;
  passed += accepted ? 1 : 0;
  printf("%s %s\n", accepted ? "PASS" : "FAIL", name);
}

auto main(Signed_32 argc, char** argv) -> Signed_32 {
  if (argc == 3 && NullTerminated::to_view(argv[1]) == "--child"_view) {
    return child_mode(NullTerminated::to_view(argv[2]));
  }

  Dynamic::Bytes executable = executable_path();
  if (executable.is_empty()) {
    fprintf(stderr, "unable to resolve oracle executable\n");
    return 1;
  }

  Signed_32 passed = 0;
  Signed_32 total = 0;
  auto echo_input = File::read("validation/data/ttx/oracles/echo.stdin"_view);
  auto echo_output = File::read("validation/data/ttx/oracles/echo.stdout"_view);
  auto echo_contract =
      File::read("validation/data/ttx/oracles/echo.contract"_view);
  if (!echo_input || !echo_output || !echo_contract) {
    fprintf(stderr, "unable to load process oracle data\n");
    return 1;
  }

  Process::Expectation echo_expectation = {
    .standard_input = *echo_input,
    .standard_output = *echo_output,
  };
  Process::Observation echo = observe(executable, "echo"_view, *echo_input);
  record(
      "echo exact streams and exit",
      *echo_contract == "stderr empty\nexit 0\ntimeout_ns 1000000000\n"_view &&
          Process::compare(echo, echo_expectation) == Process::Difference::None,
      passed, total);

  Process::Expectation events = {
    .standard_output = event_output,
  };
  record(
      "ordered observation accepted",
      Process::compare(observe(executable, "pass"_view), events) ==
          Process::Difference::None,
      passed, total);
  record(
      "missing observation rejected",
      Process::compare(observe(executable, "missing"_view), events) ==
          Process::Difference::StandardOutput,
      passed, total);
  record(
      "reordered observation rejected",
      Process::compare(observe(executable, "reordered"_view), events) ==
          Process::Difference::StandardOutput,
      passed, total);
  record(
      "duplicate observation rejected",
      Process::compare(observe(executable, "duplicate"_view), events) ==
          Process::Difference::StandardOutput,
      passed, total);
  record(
      "unexpected observation rejected",
      Process::compare(observe(executable, "unexpected"_view), events) ==
          Process::Difference::StandardOutput,
      passed, total);

  Process::Observation timed_out =
      observe(executable, "timeout"_view, View::Bytes(), 50'000'000);
  record(
      "timed out observation rejected",
      timed_out.timed_out &&
          Process::compare(timed_out, events) == Process::Difference::Timeout,
      passed, total);

  Process::Observation wrong_stream = observe(executable, "wrong_stream"_view);
  record(
      "wrong stream observation rejected",
      wrong_stream.standard_output.is_empty() &&
          wrong_stream.standard_error == event_output &&
          Process::compare(wrong_stream, events) != Process::Difference::None,
      passed, total);
  record(
      "wrong exit observation rejected",
      Process::compare(observe(executable, "wrong_exit"_view), events) ==
          Process::Difference::ExitStatus,
      passed, total);

  printf("%d / %d process oracle self tests passed\n", passed, total);
  return passed == total ? 0 : 1;
}
