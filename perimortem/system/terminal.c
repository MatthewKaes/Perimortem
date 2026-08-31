// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/system/terminal.h"

#include "perimortem/core/object.h"

#include "perimortem/memory/buffer.h"

static const struct perimortem_object_descriptor byte_descriptor = {
  .size = 1,
  .alignment = 1,
  .finalize = perimortem_core_object_finalize_trivial,
};

static struct perimortem_terminal_line absent_line(void) {
  const struct perimortem_terminal_line result = {
    .value = {.data = 0, .size = 0},
    .present = PERIMORTEM_FALSE,
  };
  return result;
}

static struct perimortem_terminal_line present_line(
    uint8_t* data,
    perimortem_count size) {
  const struct perimortem_terminal_line result = {
    .value = {.data = data, .size = size},
    .present = PERIMORTEM_TRUE,
  };
  return result;
}

struct perimortem_terminal_line perimortem_terminal_read_line(
    struct perimortem_terminal* terminal) {
  uint8_t* data = 0;
  perimortem_count size = 0;
  perimortem_bool terminated = PERIMORTEM_FALSE;
  const uint8_t empty = 0;

  if (terminal == 0 || terminal->input == 0) {
    return absent_line();
  }

  // A stream can return useful bytes before EOF, so keep the partial line
  // local until LF or a successful final EOF decides the complete result.
  for (;;) {
    const int next = fgetc(terminal->input);
    if (next == EOF) {
      if (ferror(terminal->input) != 0 || size == 0) {
        perimortem_core_object_release(data);
        return absent_line();
      }

      break;
    }

    if (next == '\n') {
      terminated = PERIMORTEM_TRUE;
      break;
    }

    data = perimortem_core_object_reserve(
        data, &byte_descriptor, size + 1, 1, &empty);
    data[size] = (uint8_t)next;
    ++size;
  }

  // CR belongs to the terminator only when LF completed this line. A final CR
  // at EOF remains ordinary authored input and is returned to the caller.
  if (terminated && size != 0 && data[size - 1] == '\r') {
    --size;
  }

  return present_line(data, size);
}

perimortem_bool perimortem_terminal_write_line(
    struct perimortem_terminal* terminal,
    struct perimortem_bytes data) {
  perimortem_bool failed = PERIMORTEM_FALSE;

  if (terminal == 0 || terminal->output == 0 ||
      (data.size != 0 && data.data == 0)) {
    return PERIMORTEM_FALSE;
  }

  // Each output stage still runs after an earlier failure so buffered stream
  // errors cannot hide whether the complete line reached its final flush.
  if (data.size != 0) {
    const size_t written = fwrite(data.data, 1, data.size, terminal->output);
    if (written != data.size) {
      failed = PERIMORTEM_TRUE;
    }
  }

  if (fputc('\n', terminal->output) == EOF) {
    failed = PERIMORTEM_TRUE;
  }

  if (fflush(terminal->output) != 0) {
    failed = PERIMORTEM_TRUE;
  }

  return failed ? PERIMORTEM_FALSE : PERIMORTEM_TRUE;
}

struct perimortem_terminal_line perimortem_system_terminal_read_line(void) {
  struct perimortem_terminal terminal = {
    .input = stdin,
    .output = stdout,
  };
  return perimortem_terminal_read_line(&terminal);
}

perimortem_bool perimortem_system_terminal_write_line(
    struct perimortem_bytes data) {
  struct perimortem_terminal terminal = {
    .input = stdin,
    .output = stdout,
  };
  return perimortem_terminal_write_line(&terminal, data);
}
