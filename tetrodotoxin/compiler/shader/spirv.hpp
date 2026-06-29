// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/dialect/shader/facts.hpp"
#include "tetrodotoxin/resolution/record.hpp"

namespace Tetrodotoxin::Compiler::Shader {

auto emit_v1_stage_module(
    const Resolution::Record& record,
    const Dialect::Shader::EntryPoint& entry,
    Perimortem::Memory::Dynamic::Bytes& out) -> Bool;

auto emit_v1_modules(
    const Resolution::Record& record,
    Perimortem::Memory::Dynamic::Bytes& vertex,
    Perimortem::Memory::Dynamic::Bytes& pixel) -> Bool;

}  // namespace Tetrodotoxin::Compiler::Shader
