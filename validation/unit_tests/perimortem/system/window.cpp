// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/system/window.hpp"

#include "validation/unit_test.hpp"

using namespace Perimortem;

static Validation::Harness SystemWindow = {
  .name = "Perimortem::System::Window"_view,
};

PERIMORTEM_UNIT_TEST(SystemWindow, empty_window_fails) {
  System::Window window;
  EXPECT(window.get_event_status() == System::Window::EventStatus::Failed);
  EXPECT(window.poll_events() == System::Window::EventStatus::Failed);
}
