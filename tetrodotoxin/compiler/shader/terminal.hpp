// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/compiler/compiler.hpp"
#include "tetrodotoxin/resolution/record.hpp"

namespace Tetrodotoxin::Compiler::Shader {

class V1Terminal {
 public:
  Perimortem::Core::View::Bytes symbol_prefix;
  Perimortem::Core::View::Bytes vertex_spirv;
  Perimortem::Core::View::Bytes pixel_spirv;
};

auto add_v1_terminal_symbols(
    Compiler& compiler,
    const Resolution::Record& record,
    const V1Terminal& terminal) -> void;

auto write_v1_terminal_header(
    const Resolution::Record& record,
    Perimortem::Core::View::Bytes symbol_prefix,
    Perimortem::Memory::Dynamic::Bytes& out) -> void;

}  // namespace Tetrodotoxin::Compiler::Shader
