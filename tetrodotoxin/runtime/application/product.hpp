// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/graphics/descriptor.hpp"
#include "tetrodotoxin/runtime/application/scene.hpp"
#include "tetrodotoxin/runtime/application/transition.hpp"

namespace Tetrodotoxin::Runtime::Application {

// Product is the immutable handoff from the App Terminal to the runtime. The
// arrays and title bytes live in the generated entry object for the complete
// process lifetime.
struct Product {
  using GraphicsDescriptor = const Tetrodotoxin::Graphics::Descriptor* (*)();

  const U8* title;
  U32 width;
  U32 height;
  const Scene* scenes;
  Count scene_count;
  Count initial_scene;
  const Transition* transitions;
  Count transition_count;
  const GraphicsDescriptor* graphics_descriptors;
  Count graphics_descriptor_count;
  const U8* graphics;
};

static_assert(__is_trivial(Product));

}  // namespace Tetrodotoxin::Runtime::Application
