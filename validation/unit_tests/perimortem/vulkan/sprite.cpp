// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/vulkan/programs/sprite.hpp"

#include "validation/process/child.hpp"
#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/file.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Vulkan;
using namespace Validation;

static Harness VulkanSprite = {
  .name = "Perimortem::Vulkan::Sprite"_view,
};

PERIMORTEM_UNIT_TEST(VulkanSprite, publishes_complete_program) {
  auto locator = Programs::Sprite::get_locator();
  auto description = Programs::Sprite::get_description();
  ASSERT(locator.is_valid());
  ASSERT_EQ(description.modules.get_size(), Count(2));
  ASSERT_EQ(description.host_input_ranges.get_size(), Count(1));
  ASSERT_EQ(description.descriptors.get_size(), Count(1));
  ASSERT_EQ(description.host_fields.get_size(), Count(3));
  EXPECT_EQ(
      locator.get_locator(),
      Data::cast<const U8>(description.modules.get_data()[0].words.get_data()));
  EXPECT(description.modules.get_data()[0].stage == Description::Stage::Vertex);
  EXPECT(description.modules.get_data()[1].stage == Description::Stage::Pixel);
  EXPECT_EQ(description.host_input_ranges.get_data()[0].size, Count(48));
  EXPECT_TEXT(description.descriptors.get_data()[0].name, "image"_view);
}

PERIMORTEM_UNIT_TEST(VulkanSprite, modules_pass_independent_validation) {
  auto modules = Programs::Sprite::get_description().modules;
  static constexpr Static::Vector<View::Bytes, 2> paths = {{
    ".bin/bin/validation/perimortem_sprite_vertex.spv"_view,
    ".bin/bin/validation/perimortem_sprite_fragment.spv"_view,
  }};
  for (Count index = 0; index < modules.get_size(); index++) {
    const Description::Module& module = modules.get_data()[index];
    View::Bytes words(
        Data::cast<const U8>(module.words.get_data()),
        module.words.get_size() * sizeof(U32));
    ASSERT(Perimortem::System::File::write(words, paths[index]));

    Static::Vector<View::Bytes, 3> arguments = {{
      "--target-env"_view,
      "vulkan1.0"_view,
      paths[index],
    }};
    Process::Request request = {
      .executable = "/usr/bin/spirv-val"_view,
      .arguments = arguments,
    };
    Process::Observation observation = Process::run(request);
    EXPECT(observation.launched);
    EXPECT_NOT(observation.timed_out);
    EXPECT_EQ(observation.exit_status, 0);
    EXPECT(observation.standard_output.is_empty());
    EXPECT(observation.standard_error.is_empty());
    EXPECT(observation.runner_error.is_empty());
    EXPECT(Perimortem::System::File::remove(paths[index]));
  }
}
