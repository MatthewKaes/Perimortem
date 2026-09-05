// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/native/toolchain.hpp"

using namespace Tetrodotoxin;

auto Terminal::Native::Toolchain::negotiate(ttx_abstract requirement) const
    -> ttx_interface_relation {
  return ttx_abstract_same(
             requirement, tetrodotoxin_native_toolchain_requirement())
             ? TTX_INTERFACE_SATISFIED
             : Ttx::Concept::Abstract::negotiate(requirement);
}
