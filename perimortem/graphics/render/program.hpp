// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/graphics/render/descriptor_binding.hpp"
#include "perimortem/graphics/render/host_field.hpp"
#include "perimortem/graphics/render/host_input_range.hpp"
#include "perimortem/graphics/render/module.hpp"

namespace Perimortem::Graphics::Render {

// A borrowed description of one graphics pipeline. Program groups the shader
// modules with their host-input and resource layouts, but owns none of the
// referenced arrays or names. Those sources must remain alive only while a
// backend reads the description to construct its independently owned program.
//
// Per-draw values such as vertex counts and host-input bytes intentionally do
// not live here. Keeping those transactions separate prevents a reusable
// pipeline description from accumulating mutable submission state.
struct Program {
  Core::View::Vector<Module> modules;
  Core::View::Vector<HostInputRange> host_input_ranges;
  Core::View::Vector<DescriptorBinding> descriptors;
  Core::View::Vector<HostField> host_fields;
};

}  // namespace Perimortem::Graphics::Render
