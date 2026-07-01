// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/compiler/compiler.hpp"
#include "tetrodotoxin/resolution/record.hpp"
#include "tetrodotoxin/resolution/resolver.hpp"

namespace Tetrodotoxin::Compiler::Shader {

class CompiledProgram {
 public:
  Perimortem::Core::View::Bytes symbol_prefix;
  Perimortem::Core::View::Bytes vertex_spirv;
  Perimortem::Core::View::Bytes pixel_spirv;
};

auto add_program_symbols(
    Compiler& compiler,
    Resolution::Resolver& resolver,
    const Resolution::Record& record,
    const CompiledProgram& program) -> void;

auto write_program_header(
    Resolution::Resolver& resolver,
    const Resolution::Record& record,
    Perimortem::Core::View::Bytes symbol_prefix,
    Perimortem::Memory::Dynamic::Bytes& output) -> void;

}  // namespace Tetrodotoxin::Compiler::Shader
