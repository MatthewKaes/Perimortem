// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Validation::Process::OracleFixture {

// Runs the reserved self-process child mode before the unit runner interprets
// its normal command line. The caller returns status when this reports true.
auto dispatch(
    Signed_32 argument_count,
    const char* const arguments[],
    Signed_32& status) -> Bool;

}  // namespace Validation::Process::OracleFixture
