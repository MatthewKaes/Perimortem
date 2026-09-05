// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/runtime/application/runner.hpp"

static auto no_placement()
    -> const Tetrodotoxin::Graphics::Runtime::Placement2D* {
  return nullptr;
}

static auto no_children()
    -> const Tetrodotoxin::Graphics::Runtime::Children2D* {
  return nullptr;
}

static auto no_drawable()
    -> const Tetrodotoxin::Graphics::Runtime::Drawable2D* {
  return nullptr;
}

int main() {
  static constexpr U8 title[] = "Window failure fixture";
  static constexpr Tetrodotoxin::Runtime::Application::Scene scene = {};
  static constexpr Tetrodotoxin::Runtime::Application::PlacementProvider
      placements[] = {no_placement};
  static constexpr Tetrodotoxin::Runtime::Application::ChildrenProvider
      children[] = {no_children};
  static constexpr Tetrodotoxin::Runtime::Application::DrawableProvider
      drawables[] = {no_drawable};
  static constexpr Perimortem::Vulkan::Description::Program program = {};
  static constexpr Tetrodotoxin::Runtime::Application::Product product = {
    .title = title,
    .width = 1,
    .height = 1,
    .scenes = &scene,
    .scene_count = 1,
    .initial_scene = 0,
    .transitions = nullptr,
    .transition_count = 0,
    .graphics_placements = placements,
    .graphics_children = children,
    .graphics_drawables = drawables,
    .graphics_type_count = 1,
    .programs = &program,
    .program_count = 1,
  };
  return Tetrodotoxin::Runtime::Application::Runner::run(product);
}
