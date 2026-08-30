// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/runtime/application/session.hpp"

#include "validation/process/child.hpp"
#include "validation/unit_test.hpp"

#include "perimortem/core/object.hpp"

#include "tetrodotoxin/runtime/application/runner.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

static Validation::Harness ApplicationSession = {
  .name = "Tetrodotoxin::Runtime::Application::Session"_view,
};

static Validation::Harness ApplicationRunner = {
  .name = "Tetrodotoxin::Runtime::Application::Runner"_view,
};

static U8 replace_signal = 0;
static U8 exit_signal = 0;
static Count first_prepares = 0;
static Count first_updates = 0;
static Count first_releases = 0;
static Count second_prepares = 0;
static Count second_updates = 0;
static Count second_releases = 0;

static auto finalize_session_object(U8*) -> void {}

static const Core::Object<>::Descriptor
    session_object_descriptor{
        .size = sizeof(U8),
        .alignment = alignof(U8),
        .finalize = finalize_session_object,
    };

static auto construct_session_object() -> void* {
  return Core::Object<>::create(session_object_descriptor).get_payload();
}

static auto prepare_first(void**) -> void {
  first_prepares++;
}

static auto update_first(void** object, R64) -> void {
  first_updates++;
  tetrodotoxin_scene_emit(*object, &replace_signal);
}

static auto release_first(void**) -> void {
  first_releases++;
}

static auto prepare_second(void**) -> void {
  second_prepares++;
}

static auto update_second(void** object, R64) -> void {
  second_updates++;
  tetrodotoxin_scene_emit(*object, &exit_signal);
}

static auto release_second(void**) -> void {
  second_releases++;
}

PERIMORTEM_UNIT_TEST(ApplicationRunner, rejects_invalid_product) {
  Runtime::Application::Product product = {};
  EXPECT_EQ(tetrodotoxin_application_scene(nullptr), 1);
  EXPECT_EQ(tetrodotoxin_application_scene(&product), 1);
}

PERIMORTEM_UNIT_TEST(ApplicationRunner, reports_window_setup_failure) {
  static constexpr Core::View::Bytes arguments[] = {
    "-u"_view,
    "WAYLAND_DISPLAY"_view,
    "-u"_view,
    "DISPLAY"_view,
    "-u"_view,
    "XDG_RUNTIME_DIR"_view,
    ".bin/bin/apps/ttx/scene_lifetime/scene_lifetime"_view,
  };
  Validation::Process::Request request = {
    .executable = "/usr/bin/env"_view,
    .arguments = arguments,
  };
  Validation::Process::Observation observation =
      Validation::Process::run(request);
  EXPECT(observation.launched);
  EXPECT_NOT(observation.timed_out);
  EXPECT_EQ(observation.exit_status, 1);
  EXPECT(observation.runner_error.is_empty());
}

PERIMORTEM_UNIT_TEST(ApplicationSession, ordered_transitions) {
  first_prepares = 0;
  first_updates = 0;
  first_releases = 0;
  second_prepares = 0;
  second_updates = 0;
  second_releases = 0;

  const Runtime::Application::Scene scenes[] = {
    {
      construct_session_object,
      prepare_first,
      nullptr,
      nullptr,
      update_first,
      release_first,
      0,
      nullptr,
    },
    {
      construct_session_object,
      prepare_second,
      nullptr,
      nullptr,
      update_second,
      release_second,
      0,
      nullptr,
    },
  };
  const Runtime::Application::Transition transitions[] = {
    {0, &replace_signal, Runtime::Application::Action::Replace, 1},
    {1, &exit_signal, Runtime::Application::Action::Exit, Count(-1)},
  };
  U8 graphics = 0;
  const Runtime::Application::Product product = {
    &graphics, 1,       1,       scenes,  2, 0,       transitions,
    2,         nullptr, nullptr, nullptr, 0, nullptr, 0,
  };

  Runtime::Application::Session session(product);
  ASSERT(session.start());
  ASSERT(session.get_active_scene());
  EXPECT_EQ(&*session.get_active_scene(), scenes);
  EXPECT_EQ(first_prepares, Count(1));
  EXPECT(session.update(0.5));
  EXPECT(session.commit());
  EXPECT_EQ(first_updates, Count(1));
  EXPECT_EQ(first_releases, Count(1));
  EXPECT_EQ(second_prepares, Count(1));
  ASSERT(session.get_active_scene());
  EXPECT_EQ(&*session.get_active_scene(), scenes + 1);

  EXPECT(session.update(0.25));
  EXPECT_NOT(session.commit());
  EXPECT_EQ(second_updates, Count(1));
  session.stop();
  EXPECT_EQ(second_releases, Count(1));
}
