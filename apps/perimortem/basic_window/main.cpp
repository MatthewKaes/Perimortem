// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/system/window.hpp"

#include "perimortem/vulkan/renderer.hpp"

using namespace Perimortem;

auto main() -> int {
  System::Window window(800, 600, "Perimortem Basic Window");
  Vulkan::Renderer renderer(
      window.get_display(), window.get_surface(),
      window.get_logical_width() * window.get_scale(),
      window.get_logical_height() * window.get_scale());
  while (window.poll_events()) {
    if (window.get_input().is_pressed(System::Input::Key::Escape)) {
      break;
    }

    const U32 width = window.get_logical_width() * window.get_scale();
    const U32 height = window.get_logical_height() * window.get_scale();
    if (window.get_needs_resize()) {
      renderer.resize(width, height);
      window.clear_resize();
    }

    Vulkan::Renderer::Frame frame;
    if (!renderer.begin_frame(frame)) {
      renderer.resize(width, height);
      continue;
    }

    renderer.end_frame(frame);
  }

  renderer.wait_idle();
  return 0;
}
