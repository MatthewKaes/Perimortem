// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/data.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/system/window.hpp"

#include "perimortem/graphics/image.hpp"
#include "perimortem/graphics/pixel.hpp"
#include "perimortem/graphics/sprite.hpp"
#include "perimortem/vulkan/programs/sprite.hpp"
#include "perimortem/vulkan/renderer.hpp"
#include "perimortem/vulkan/sprite_renderer.hpp"
#include "tetrodotoxin/graphics/sprite_descriptor.hpp"
#include "tetrodotoxin/graphics/submission.hpp"

using namespace Perimortem;

static auto create_test_image() -> Graphics::Image {
  Memory::Dynamic::Vector<Graphics::Pixel> pixels;
  pixels.emplace(Graphics::Pixel::from_rgba(0xFF, 0x00, 0x00, 0xFF));
  pixels.emplace(Graphics::Pixel::from_rgba(0x00, 0xFF, 0x00, 0xFF));
  pixels.emplace(Graphics::Pixel::from_rgba(0x00, 0x00, 0xFF, 0xFF));
  pixels.emplace(Graphics::Pixel::from_rgba(0x00, 0x00, 0x00, 0x00));
  return Graphics::Image(Core::Data::take(pixels), 2, 2);
}

auto main() -> int {
  System::Window window(800, 600, "Tetrodotoxin Graphics");
  Vulkan::Renderer renderer(
      window.get_presentation(),
      window.get_logical_width() * window.get_scale(),
      window.get_logical_height() * window.get_scale());
  Vulkan::SpriteRenderer graphics(
      renderer.get_context(), renderer.get_swapchain().get_format(),
      Vulkan::Programs::Sprite::get_locator(),
      Vulkan::Programs::Sprite::get_description());

  Graphics::Image image = create_test_image();
  Graphics::Sprite sprite;
  sprite.set_image(image);
  sprite.set_size_pixels({320, 320});
  Graphics::Transform2D transform;
  transform.translation = {240.0, 140.0};
  sprite.set_transform(transform);
  Tetrodotoxin::Graphics::SpriteDescriptor descriptor(
      Vulkan::Programs::Sprite::get_locator());
  auto submission = Tetrodotoxin::Graphics::Submission::create(
      sprite.get_object(), descriptor.get_descriptor());
  if (!submission) {
    return 1;
  }

  while (window.poll_events()) {
    if (window.get_input().is_pressed(System::Input::Key::Escape)) {
      break;
    }

    const U32 width = window.get_logical_width() * window.get_scale();
    const U32 height = window.get_logical_height() * window.get_scale();
    if (window.get_needs_resize()) {
      if (renderer.resize(width, height)) {
        graphics.rebuild(renderer.get_swapchain().get_format());
      }
      window.clear_resize();
    }

    Vulkan::Renderer::Frame frame;
    if (!renderer.begin_frame(frame)) {
      renderer.resize(width, height);
      continue;
    }

    Bool recorded = graphics.record(
        frame.command_buffer, frame.width, frame.height,
        submission->get_batches());
    renderer.end_frame(frame);
    if (!recorded) {
      break;
    }
  }

  renderer.wait_idle();
  return 0;
}
