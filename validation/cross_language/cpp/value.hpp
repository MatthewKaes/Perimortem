// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_CROSS_LANGUAGE_CPP_VALUE_HPP
#define VALIDATION_CROSS_LANGUAGE_CPP_VALUE_HPP

#include "cross_language/bridge.h"
#include "cross_language/cpp/ttx.hpp"

namespace TtxTest {

auto create_value(
    ttx_abstract value,
    ttx_abstract view_bytes,
    ttx_abstract to_string,
    Route text) -> ttx_abstract;

}  // namespace TtxTest

#endif
