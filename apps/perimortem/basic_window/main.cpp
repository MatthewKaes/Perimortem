// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/system/window.hpp"

#include "perimortem/vulkan/renderer.hpp"

using namespace Perimortem;

auto main() -> int {
  System::Window window(800, 600, "Perimortem Basic Window");
  System::Window::EventStatus event_status = window.get_event_status();
  if (event_status != System::Window::EventStatus::Ready) {
    return event_status == System::Window::EventStatus::Failed ? 1 : 0;
  }
  Vulkan::Renderer renderer(
      window.get_presentation(),
      window.get_logical_width() * window.get_scale(),
      window.get_logical_height() * window.get_scale());
  while ((event_status = window.poll_events()) ==
         System::Window::EventStatus::Ready) {
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
  return event_status == System::Window::EventStatus::Failed ? 1 : 0;
}
