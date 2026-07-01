// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/dialect/shader/metadata.hpp"
#include "tetrodotoxin/resolution/record.hpp"
#include "tetrodotoxin/resolution/resolver.hpp"

namespace Tetrodotoxin::Compiler::Shader {

auto emit_stage_module(
    Resolution::Resolver& resolver,
    const Resolution::Record& record,
    const Dialect::Shader::Metadata::EntryPoint& entry,
    Perimortem::Memory::Dynamic::Bytes& output) -> Bool;

auto emit_program_modules(
    Resolution::Resolver& resolver,
    const Resolution::Record& record,
    Perimortem::Memory::Dynamic::Bytes& vertex,
    Perimortem::Memory::Dynamic::Bytes& pixel) -> Bool;

}  // namespace Tetrodotoxin::Compiler::Shader
