// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/abi/runtime_symbols.h"

#define TTX_ABI_SYMBOL(text) {(const uint8_t*)(text), sizeof(text) - 1}

const struct perimortem_bytes tetrodotoxin_terminal_abi_object_allocate_symbol =
    TTX_ABI_SYMBOL("perimortem_core_object_allocate");
const struct perimortem_bytes tetrodotoxin_terminal_abi_object_retain_symbol =
    TTX_ABI_SYMBOL("perimortem_core_object_retain");
const struct perimortem_bytes tetrodotoxin_terminal_abi_object_release_symbol =
    TTX_ABI_SYMBOL("perimortem_core_object_release");
const struct perimortem_bytes tetrodotoxin_terminal_abi_object_capacity_symbol =
    TTX_ABI_SYMBOL("perimortem_core_object_capacity");
const struct perimortem_bytes tetrodotoxin_terminal_abi_object_clone_symbol =
    TTX_ABI_SYMBOL("perimortem_core_object_clone");
const struct perimortem_bytes
    tetrodotoxin_terminal_abi_object_reservations_symbol =
        TTX_ABI_SYMBOL("perimortem_core_object_reservations");
const struct perimortem_bytes tetrodotoxin_terminal_abi_object_reserve_symbol =
    TTX_ABI_SYMBOL("perimortem_core_object_reserve");
const struct perimortem_bytes
    tetrodotoxin_terminal_abi_object_finalize_trivial_symbol =
        TTX_ABI_SYMBOL("perimortem_core_object_finalize_trivial");
