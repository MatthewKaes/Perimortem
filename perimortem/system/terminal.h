// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef PERIMORTEM_SYSTEM_TERMINAL_H
#define PERIMORTEM_SYSTEM_TERMINAL_H

#include <stdio.h>

#include "perimortem/core/perimortem.h"

// Terminal borrows its streams. The default generated-code operations use
// stdin and stdout, while tests and embedding hosts may provide other streams
// with a lifetime that covers each call.
struct perimortem_terminal {
  FILE* input;
  FILE* output;
};

// A present empty line and an absent result are distinct. Present bytes own
// one Object reservation which transfers to the caller. The caller releases a
// nonnull data pointer with perimortem_core_object_release.
struct perimortem_terminal_line {
  struct perimortem_bytes value;
  perimortem_bool present;
};

PERIMORTEM_EXTERN_C struct perimortem_terminal_line
    perimortem_terminal_read_line(struct perimortem_terminal* terminal);
PERIMORTEM_EXTERN_C perimortem_bool perimortem_terminal_write_line(
    struct perimortem_terminal* terminal,
    struct perimortem_bytes data);

PERIMORTEM_EXTERN_C struct perimortem_terminal_line
    perimortem_system_terminal_read_line(void);
PERIMORTEM_EXTERN_C perimortem_bool
    perimortem_system_terminal_write_line(struct perimortem_bytes data);

#endif
