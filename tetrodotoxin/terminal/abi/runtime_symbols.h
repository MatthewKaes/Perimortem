// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_TERMINAL_ABI_RUNTIME_SYMBOLS_H
#define TETRODOTOXIN_TERMINAL_ABI_RUNTIME_SYMBOLS_H

#include "perimortem/core/perimortem.h"

// A generated native product and the runtime must agree on exact external
// spellings. The ABI Terminal owns that agreement because it authors the
// product boundary. Perimortem exports ordinary C functions and does not need
// a parallel symbol catalogue merely to describe itself to one producer.
PERIMORTEM_EXTERN_C const struct perimortem_bytes
    tetrodotoxin_terminal_abi_object_allocate_symbol;
PERIMORTEM_EXTERN_C const struct perimortem_bytes
    tetrodotoxin_terminal_abi_object_retain_symbol;
PERIMORTEM_EXTERN_C const struct perimortem_bytes
    tetrodotoxin_terminal_abi_object_release_symbol;
PERIMORTEM_EXTERN_C const struct perimortem_bytes
    tetrodotoxin_terminal_abi_object_capacity_symbol;
PERIMORTEM_EXTERN_C const struct perimortem_bytes
    tetrodotoxin_terminal_abi_object_clone_symbol;
PERIMORTEM_EXTERN_C const struct perimortem_bytes
    tetrodotoxin_terminal_abi_object_reservations_symbol;
PERIMORTEM_EXTERN_C const struct perimortem_bytes
    tetrodotoxin_terminal_abi_object_reserve_symbol;
PERIMORTEM_EXTERN_C const struct perimortem_bytes
    tetrodotoxin_terminal_abi_object_finalize_trivial_symbol;

#endif
