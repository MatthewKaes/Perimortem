// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/model/type.hpp"

namespace Ttx::Model::Layouts {

// Reports whether expanding one Type Layout reaches only terminal Type leaves.
// The query follows the real Type and Addressable edges already retained by the
// Layout. This keeps recursive shape validation independent from any language
// operation that later constructs or lowers a value.
auto is_terminating(const Type& type) -> Bool;

}  // namespace Ttx::Model::Layouts
