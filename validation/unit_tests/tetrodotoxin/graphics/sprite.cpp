// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/frame/sprite.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/graphics/image.hpp"
#include "perimortem/graphics/pixel.hpp"
#include "perimortem/graphics/sprite.hpp"
#include "tetrodotoxin/graphics/sprite_descriptor.hpp"
#include "tetrodotoxin/graphics/submission.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Graphics;
using namespace Validation;

static Harness GraphicsSprite = {
  .name = "Tetrodotoxin::Graphics::Sprite"_view,
};

alignas(U32) static constexpr U32 sprite_program[] = {0x07230203};

static auto configured_sprite() -> Perimortem::Graphics::Sprite {
  Dynamic::Vector<Perimortem::Graphics::Pixel> pixels;
  pixels.emplace(
      Perimortem::Graphics::Pixel::from_rgba(0xFF, 0x80, 0x40, 0xFF));
  Perimortem::Graphics::Image image(Data::take(pixels), 1, 1);
  Perimortem::Graphics::Sprite sprite;
  sprite.set_image(image);
  sprite.set_size_pixels({48, 32});
  Perimortem::Graphics::Transform2D transform;
  transform.translation = {4.0, 5.0};
  transform.scale_x = 2.0;
  transform.scale_y = 3.0;
  sprite.set_transform(transform);
  sprite.set_tone({0.25, 0.5, 0.75, 1.0});
  sprite.set_z_index(6);
  return sprite;
}

PERIMORTEM_UNIT_TEST(GraphicsSprite, unconfigured_sprite_submits_no_draw) {
  Perimortem::Graphics::Sprite sprite;
  SpriteDescriptor descriptor(
      Perimortem::Graphics::Frame::Program(
          Data::cast<const U8>(sprite_program)));
  auto submission =
      Submission::create(sprite.get_object(), descriptor.get_descriptor());
  ASSERT(submission);
  EXPECT(submission->get_batches().is_empty());
}

PERIMORTEM_UNIT_TEST(GraphicsSprite, freezes_reference_sprite) {
  Perimortem::Graphics::Sprite sprite = configured_sprite();
  SpriteDescriptor descriptor(
      Perimortem::Graphics::Frame::Program(
          Data::cast<const U8>(sprite_program)));
  auto submission =
      Submission::create(sprite.get_object(), descriptor.get_descriptor());
  ASSERT(submission);
  auto batches = submission->get_batches();
  ASSERT_EQ(batches.get_size(), Count(1));
  const Perimortem::Graphics::Frame::Batch& batch = batches.get_data()[0];
  EXPECT_EQ(batch.get_vertex_count(), Count(6));
  EXPECT_EQ(batch.get_z_index(), S64(6));
  EXPECT_EQ(batch.get_transform().get_x(), R64(4.0));
  EXPECT_EQ(batch.get_transform().get_y(), R64(5.0));
  ASSERT_EQ(batch.get_resources().get_size(), Count(1));
  EXPECT_EQ(batch.get_resources().get_data()[0].get_reservations(), Count(2));

  const auto* frame = Data::cast<const Perimortem::Graphics::Frame::Sprite>(
      batch.get_inputs().get_data());
  EXPECT_EQ(frame->size_pixels.width, U32(48));
  EXPECT_EQ(frame->size_pixels.height, U32(32));
  EXPECT_EQ(frame->tone.green, R64(0.5));

  sprite.set_size_pixels({1, 1});
  sprite.set_tone({1.0, 1.0, 1.0, 1.0});
  EXPECT_EQ(frame->size_pixels.width, U32(48));
  EXPECT_EQ(frame->tone.green, R64(0.5));
}
