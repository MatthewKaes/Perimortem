// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_CROSS_LANGUAGE_CPP_ECHO_HPP
#define VALIDATION_CROSS_LANGUAGE_CPP_ECHO_HPP

#include "cross_language/cpp/ttx.hpp"

namespace TtxTest {

auto create_echo_simulacrum(
    ttx_abstract echo,
    ttx_abstract operation,
    ttx_abstract child) -> ttx_abstract;

}  // namespace TtxTest

#endif
