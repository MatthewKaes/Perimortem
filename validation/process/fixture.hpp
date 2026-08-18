// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Validation::Process::Fixture {

auto dispatch(
    Signed_32 argument_count,
    const char* const arguments[],
    Signed_32& status) -> Bool;

}  // namespace Validation::Process::Fixture
