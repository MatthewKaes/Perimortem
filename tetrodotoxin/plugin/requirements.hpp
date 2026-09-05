// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <optional>

#include "tetrodotoxin/plugin/abi.h"

namespace Tetrodotoxin::Plugin {

auto resolve_requirement(
    ttx_host_requirements host,
    const ttx_requirement_descriptor& descriptor)
    -> std::optional<ttx_abstract>;

}  // namespace Tetrodotoxin::Plugin
