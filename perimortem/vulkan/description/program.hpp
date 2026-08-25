// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/vulkan/description/descriptor_binding.hpp"
#include "perimortem/vulkan/description/host_field.hpp"
#include "perimortem/vulkan/description/host_input_range.hpp"
#include "perimortem/vulkan/description/module.hpp"
#include "perimortem/vulkan/description/vertex_input.hpp"

namespace Perimortem::Vulkan::Description {

// A borrowed description of one Vulkan pipeline derived from completed Render,
// Shader, and SPIR V products. Program groups target modules with their host
// input and resource layouts, but owns none of the referenced arrays or names.
// Those products remain alive while Vulkan creates its independently owned
// pipeline.
//
// Per draw values such as vertex counts and host input bytes intentionally do
// not live here. Keeping those transactions separate prevents this target
// description from becoming another frame or semantic model.
struct Program {
  Core::View::Vector<Module> modules;
  Core::View::Vector<HostInputRange> host_input_ranges;
  Core::View::Vector<DescriptorBinding> descriptors;
  Core::View::Vector<HostField> host_fields;
  Core::View::Vector<VertexInput> vertex_inputs;
};

}  // namespace Perimortem::Vulkan::Description
