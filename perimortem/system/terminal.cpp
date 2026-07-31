// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/terminal.hpp"

#include <stdio.h>

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;

Terminal::Terminal() : Terminal(*stdin, *stdout) {}

Terminal::Terminal(FILE& input, FILE& output) : input(input), output(output) {}

auto Terminal::read_line() -> Option<Dynamic::Bytes> {
  Dynamic::Bytes line;
  Bool terminated = False;

  // A stream can return useful bytes before EOF, so keep the partial line
  // local until LF or a successful final EOF decides the complete result.
  while (true) {
    Signed_32 next;
    next = fgetc(&input);
    if (next == EOF) {
      if (ferror(&input) != 0 || line.is_empty()) {
        return {};
      }

      break;
    }

    if (next == '\n') {
      terminated = True;
      break;
    }

    line.append(Unsigned_8(next));
  }

  // CR belongs to the terminator only when LF completed this line. A final CR
  // at EOF remains ordinary authored input and is returned to the caller.
  if (terminated && !line.is_empty() && line[line.get_size() - 1] == '\r') {
    line.resize(line.get_size() - 1);
  }

  return Option<Dynamic::Bytes>(Data::take(line));
}

auto Terminal::write_line(View::Bytes data) -> Bool {
  Bool failed = False;

  // Each output stage still runs after an earlier failure so buffered stream
  // errors cannot hide whether the complete line reached its final flush.
  if (!data.is_empty()) {
    CppSize written;
    written = fwrite(data.get_data(), 1, data.get_size(), &output);
    failed |= written != data.get_size();
  }

  Signed_32 newline;
  newline = fputc('\n', &output);
  failed |= newline == EOF;

  Signed_32 flushed;
  flushed = fflush(&output);
  failed |= flushed != 0;

  return !failed;
}

// Foreign uses the C symbol namespace while generated native callers still
// share Perimortem value layouts. Limit Clang's C source warning to these two
// intentional language boundary definitions.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"

extern "C" auto perimortem_system_terminal_read_line()
    -> Option<Dynamic::Bytes> {
  Terminal terminal;
  return terminal.read_line();
}

extern "C" auto perimortem_system_terminal_write_line(View::Bytes data)
    -> Bool {
  Terminal terminal;
  return terminal.write_line(data);
}

#pragma clang diagnostic pop
