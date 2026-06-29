// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/dialect/shader/facts.hpp"

namespace Tetrodotoxin::Compiler::Shader {

auto write_push_constant_metadata(
    const Dialect::Shader::PushConstantMetadata& metadata,
    Perimortem::Memory::Dynamic::Bytes& out) -> void;

auto write_descriptor_metadata(
    Perimortem::Core::View::Vector<Dialect::Shader::DescriptorBinding> metadata,
    Perimortem::Memory::Dynamic::Bytes& out) -> void;

}  // namespace Tetrodotoxin::Compiler::Shader
