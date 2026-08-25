// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/runtime/application/runner.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/time.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/system/input.hpp"
#include "perimortem/system/window.hpp"

#include "perimortem/abi/system/input.hpp"
#include "perimortem/vulkan/description/program.hpp"
#include "perimortem/vulkan/renderer.hpp"
#include "perimortem/vulkan/sprite_renderer.hpp"
#include "tetrodotoxin/graphics/compiled_descriptor.hpp"
#include "tetrodotoxin/graphics/submission.hpp"
#include "tetrodotoxin/runtime/application/session.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

auto Runtime::Application::Runner::run(const Product& product) -> int {
  if (product.title == nullptr || product.width == 0 || product.height == 0 ||
      product.initial_scene >= product.scene_count ||
      product.graphics_descriptor_count == 0 ||
      product.graphics_descriptors == nullptr || product.graphics == nullptr) {
    return 1;
  }

  System::Window window(
      product.width, product.height,
      reinterpret_cast<const char*>(product.title));
  const auto& graphics_program =
      *Core::Data::cast<const Vulkan::Description::Program>(product.graphics);
  if (graphics_program.modules.is_empty()) {
    return 1;
  }

  Perimortem::Graphics::Frame::Program frame_program(
      Core::Data::cast<const U8>(
          graphics_program.modules.get_data()[0].words.get_data()));
  Memory::Dynamic::Vector<Tetrodotoxin::Graphics::Descriptor>
      graphics_descriptors(product.graphics_descriptor_count);
  for (Count index = 0; index < product.graphics_descriptor_count; index++) {
    auto factory = product.graphics_descriptors[index];
    const Tetrodotoxin::Graphics::Descriptor* descriptor =
        factory ? factory() : nullptr;
    if (descriptor == nullptr) {
      return 1;
    }
    graphics_descriptors.insert(descriptor->bind(frame_program.get_locator()));
  }
  Memory::Dynamic::Vector<const Tetrodotoxin::Graphics::Descriptor*>
      graphics_types(product.graphics_descriptor_count);
  for (Count index = 0; index < graphics_descriptors.get_size(); index++) {
    graphics_types.insert(&graphics_descriptors[index]);
  }
  U32 physical_width = window.get_logical_width() * window.get_scale();
  U32 physical_height = window.get_logical_height() * window.get_scale();
  Vulkan::Renderer renderer(
      window.get_presentation(), physical_width, physical_height);
  Vulkan::SpriteRenderer sprite_renderer(
      renderer.get_context(), renderer.get_swapchain().get_format(),
      frame_program, graphics_program);
  Runtime::Application::Session session(product);
  if (!session.start()) {
    return 1;
  }

  // The runtime keeps authored lifecycle ahead of graphics extraction. A Scene
  // can update its real hosted Objects first, then the descriptor reads that
  // completed state for the frame without another retained Scene inventory.
  Core::Time previous = Core::Time::now();
  while (session.is_running() && window.poll_events()) {
    Abi::System::publish_input(window.get_input());
    if (window.get_input().is_pressed(System::Input::Key::Escape)) {
      session.stop();
      break;
    }

    Core::Time current = Core::Time::now();
    R64 delta = previous.measure(current).convert_to_seconds();
    previous = current;
    if (!session.update(delta)) {
      break;
    }

    if (window.get_needs_resize()) {
      physical_width = window.get_logical_width() * window.get_scale();
      physical_height = window.get_logical_height() * window.get_scale();
      if (physical_width != 0 && physical_height != 0 &&
          renderer.resize(physical_width, physical_height)) {
        sprite_renderer.rebuild(renderer.get_swapchain().get_format());
      }
      window.clear_resize();
    }

    auto active_scene = session.get_active_scene();
    if (!active_scene) {
      break;
    }
    Tetrodotoxin::Graphics::CompiledDescriptor scene_descriptor(
        active_scene->graphics_child_count, active_scene->graphics_children,
        graphics_types.get_view());
    auto submission = Tetrodotoxin::Graphics::Submission::create(
        session.get_active_object(), scene_descriptor.get_descriptor());
    Vulkan::Renderer::Frame frame;
    Bool frame_complete = submission && renderer.begin_frame(frame);
    if (frame_complete) {
      frame_complete = sprite_renderer.record(
          frame.command_buffer, frame.width, frame.height,
          submission->get_batches());
      renderer.end_frame(frame);
    }
    if (!frame_complete) {
      session.stop();
      break;
    }
    session.commit();
  }

  session.stop();
  return 0;
}

extern "C" auto tetrodotoxin_application_scene(
    const Runtime::Application::Product* product) -> int {
  return product ? Runtime::Application::Runner::run(*product) : 1;
}
