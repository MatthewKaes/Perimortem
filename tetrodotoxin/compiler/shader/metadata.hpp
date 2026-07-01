// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/dialect/shader/metadata.hpp"

namespace Tetrodotoxin::Compiler::Shader {

auto write_host_input_metadata(
    const Dialect::Shader::Metadata::HostInputs& metadata,
    Perimortem::Memory::Dynamic::Bytes& output) -> void;

auto write_descriptor_metadata(
    Perimortem::Core::View::Vector<
        Dialect::Shader::Metadata::Binding>
        metadata,
    Perimortem::Memory::Dynamic::Bytes& output) -> void;

}  // namespace Tetrodotoxin::Compiler::Shader
