// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/graphics/render/program.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Graphics;
using namespace Validation;

static Harness GraphicsRender = {
  .name = "Graphics::Render"_view,
};

PERIMORTEM_UNIT_TEST(GraphicsRender, pipeline_layout) {
  const Unsigned_32 words[] = {0x07230203, 0x00010000, 0, 1};
  const Render::Stage stages[] = {
    Render::Stage::Vertex,
    Render::Stage::Pixel,
  };
  const Render::Module modules[] = {{
    .stage = Render::Stage::Vertex,
    .words = words,
    .entry = "main"_view,
  }};
  const Render::HostInputRange host_input_ranges[] = {{
    .offset = 16,
    .size = 32,
    .stages = stages,
  }};
  const Render::DescriptorBinding descriptors[] = {{
    .name = "texture"_view,
    .set = 1,
    .slot = 2,
  }};
  const Render::HostField host_fields[] = {{
    .name = "transform"_view,
    .offset = 16,
    .size = 32,
  }};
  const Render::Program program = {
    .modules = modules,
    .host_input_ranges = host_input_ranges,
    .descriptors = descriptors,
    .host_fields = host_fields,
  };
  EXPECT_EQ(program.modules.get_size(), Count(1));
  EXPECT_EQ(program.modules[0].words.get_size(), Count(4));
  EXPECT_EQ(program.modules[0].entry, "main"_view);
  EXPECT_EQ(program.host_input_ranges[0].offset, Count(16));
  EXPECT_EQ(program.host_input_ranges[0].stages.get_size(), Count(2));
  EXPECT_EQ(program.descriptors[0].name, "texture"_view);
  EXPECT_EQ(program.descriptors[0].set, Count(1));
  EXPECT_EQ(program.descriptors[0].slot, Count(2));
  EXPECT_EQ(program.host_fields[0].name, "transform"_view);
}
