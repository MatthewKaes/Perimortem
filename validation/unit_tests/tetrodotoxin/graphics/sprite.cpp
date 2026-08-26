// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/graphics/sprite.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/data.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/graphics/image.hpp"
#include "perimortem/graphics/pixel.hpp"
#include "perimortem/graphics/projection.hpp"
#include "perimortem/graphics/texture_2d.hpp"
#include "tetrodotoxin/graphics/runtime/sprite_drawable_2d.hpp"
#include "tetrodotoxin/graphics/runtime/sprite_placement_2d.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Graphics::Runtime;
using namespace Validation;

static Harness GraphicsSprite = {
  .name = "Tetrodotoxin::Graphics::Sprite"_view,
};

alignas(U32) static constexpr U32 sprite_program[] = {0x07230203};

struct SpriteParameters {
  R32 tone[4];
};

static auto finalize_parameters(U8*) -> void {}

static const Object<>::Descriptor parameter_descriptor(
    sizeof(SpriteParameters),
    alignof(SpriteParameters),
    finalize_parameters);

static const Perimortem::Graphics::Projection sprite_projection = {
  Data::cast<const U8>(sprite_program),
  0,
  sizeof(SpriteParameters),
};

static auto create_shader() -> Implementation {
  Object<> instance = Object<>::create(parameter_descriptor);
  auto* parameters = Data::cast<SpriteParameters>(instance.get_payload());
  parameters->tone[0] = 0.25f;
  parameters->tone[1] = 0.5f;
  parameters->tone[2] = 0.75f;
  parameters->tone[3] = 1.0f;
  auto implementation = Implementation::retain(instance, &sprite_projection);
  instance.release();
  return implementation ? static_cast<Implementation&&>(*implementation)
                        : Implementation();
}

static auto configured_sprite() -> Perimortem::Graphics::Sprite {
  Dynamic::Vector<Perimortem::Graphics::Pixel> pixels;
  pixels.emplace(
      Perimortem::Graphics::Pixel::from_rgba(0xFF, 0x80, 0x40, 0xFF));
  Perimortem::Graphics::Image image(Data::take(pixels), 1, 1);
  Perimortem::Graphics::Texture2D texture(image);
  Perimortem::Graphics::Sprite sprite;
  sprite.set_texture(texture);
  sprite.set_shader(create_shader());
  sprite.set_size_pixels({48, 32});
  Perimortem::Graphics::Transform2D transform;
  transform.translation = {4.0, 5.0};
  transform.scale_x = 2.0;
  transform.scale_y = 3.0;
  sprite.set_transform(transform);
  sprite.set_z_index(6);
  return sprite;
}

PERIMORTEM_UNIT_TEST(GraphicsSprite, unconfigured_sprite_submits_no_draw) {
  Perimortem::Graphics::Sprite sprite;
  const Drawable2D& drawable = SpriteDrawable2D::get_runtime();
  EXPECT_EQ(drawable.draw_count(sprite.get_object()), Count(0));
}

PERIMORTEM_UNIT_TEST(GraphicsSprite, projects_live_shader_parameters) {
  Perimortem::Graphics::Sprite sprite = configured_sprite();
  const Drawable2D& drawable = SpriteDrawable2D::get_runtime();
  const Placement2D& placement = SpritePlacement2D::get_runtime();
  ASSERT_EQ(drawable.draw_count(sprite.get_object()), Count(1));
  auto first = drawable.draw(sprite.get_object(), 0);
  auto selected_placement = placement.placement(sprite.get_object());
  ASSERT(first);
  ASSERT(selected_placement);
  EXPECT_EQ(first->get_program().get_locator(), sprite_projection.program);
  EXPECT_EQ(first->get_size_pixels().width, U32(48));
  EXPECT_EQ(first->get_size_pixels().height, U32(32));
  EXPECT_EQ(selected_placement->get_z_index(), S64(6));
  EXPECT_EQ(selected_placement->get_transform().get_x(), R64(4.0));
  EXPECT_EQ(selected_placement->get_transform().get_y(), R64(5.0));
  ASSERT_EQ(first->get_resources().get_size(), Count(1));
  EXPECT_EQ(first->get_resources().get_data()[0].get_reservations(), Count(2));
  const auto* first_parameters =
      Data::cast<const SpriteParameters>(first->get_inputs().get_data());
  EXPECT_EQ(first_parameters->tone[1], R32(0.5));

  Object<> instance = sprite.get_shader().get_object();
  auto* live = Data::cast<SpriteParameters>(instance.get_payload());
  live->tone[1] = 0.125f;
  auto second = drawable.draw(sprite.get_object(), 0);
  ASSERT(second);
  const auto* second_parameters =
      Data::cast<const SpriteParameters>(second->get_inputs().get_data());
  EXPECT_EQ(first_parameters->tone[1], R32(0.5));
  EXPECT_EQ(second_parameters->tone[1], R32(0.125));
}
