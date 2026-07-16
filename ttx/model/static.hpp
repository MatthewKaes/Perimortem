// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/callable.hpp"

namespace Ttx::Model {

// Static is the invocation distinction for a Callable selected through a Type
// or package context. Invocation does not add a receiver.
class Static : public Callable {};

}  // namespace Ttx::Model
