// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Ttx {

static constexpr Bits_32 shader_stage_vertex = 1;
static constexpr Bits_32 shader_stage_pixel = 2;

enum class ShaderStage : Bits_32 {
  Vertex = shader_stage_vertex,
  Pixel = shader_stage_pixel,
};

struct ShaderModule {
  ShaderStage stage;
  const Bits_32* spirv;
  const Count* spirv_size;
  const char* entry;
};

struct ShaderPushConstantRange {
  Count offset;
  Count size;
  Bits_32 stage_mask;
};

struct ShaderDescriptorBinding {
  const char* name;
  Count set;
  Count slot;
};

enum class ShaderObjectProperty : Bits_32 {
  Position,
  HalfSize,
  Alpha,
};

struct ShaderObjectPropertyBinding {
  ShaderObjectProperty property;
  Count offset;
  Count size;
};

struct ShaderProgramInfo {
  const ShaderModule* modules;
  Count module_count;
  const ShaderPushConstantRange* push_constants;
  Count push_constant_count;
  const ShaderDescriptorBinding* descriptors;
  Count descriptor_count;
  const ShaderObjectPropertyBinding* object_properties;
  Count object_property_count;
  Count vertex_count;
};

struct Shader {
  const ShaderProgramInfo* program;
};

}  // namespace Ttx
