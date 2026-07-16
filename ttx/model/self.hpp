// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/callable.hpp"

namespace Ttx::Model {

// Self is the invocation distinction for a Callable selected through an
// addressable value. The receiver is entry zero of get_parameters(); no
// parser, compiler, reflector, or ABI layer may prepend it a second time.
class Self : public Callable {};

}  // namespace Ttx::Model
