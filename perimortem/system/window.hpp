// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#ifdef PERI_LINUX
#include "perimortem/system/platform/wayland/input.hpp"
#include "perimortem/system/platform/wayland/xdg_shell.hpp"
#else
#error Perimortem does not have a window implementation for this platform.
#endif

#include "perimortem/system/input.hpp"
#include "perimortem/system/presentation.hpp"

namespace Perimortem::System {

// Window owns one platform toplevel and turns host events into stable System
// values. Its public shape remains the same as platform implementations grow,
// while Presentation carries the narrow native handoff selected by a renderer.
class Window {
 public:
  // EventStatus keeps an ordinary compositor close separate from a host
  // failure so an application can preserve its process outcome.
  enum class EventStatus : U8 {
    Ready,
    Closed,
    Failed,
  };

  Window() = default;
  Window(U32 width, U32 height, const char* title);
  ~Window();
  Window(Window&&) = delete;
  auto operator=(Window&&) -> Window& = delete;
  Window(const Window&) = delete;
  auto operator=(const Window&) -> Window& = delete;

  auto poll_events() -> EventStatus;
  auto get_event_status() const -> EventStatus;

  auto get_logical_width() const -> U32;
  auto get_logical_height() const -> U32;
  auto get_scale() const -> U32;
  auto get_needs_resize() const -> Bool;
  auto clear_resize() -> void;

  // poll_events publishes exactly one immutable input snapshot. Applications
  // can update the mapping between frames without exposing host keycodes.
  auto get_input() const -> const Input&;
  auto get_input_mapping() -> Input::Mapping&;
  auto get_input_mapping() const -> const Input::Mapping&;

  auto get_presentation() const -> Presentation;

 private:
  auto destroy() -> void;

  static auto on_shell_configure(void* data, S32 width, S32 height) -> void;
  static auto on_shell_close(void* data) -> void;
  static auto on_registry_global(
      void* data,
      void* registry,
      U32 name,
      const char* interface,
      U32 version) -> void;
  static auto on_registry_global_remove(void* data, U32 name) -> void;
  static auto on_surface_scale(void* data, S32 factor) -> void;

#ifdef PERI_LINUX
  Platform::Wayland::XdgShell shell;
  Platform::Wayland::Input input_collector;
#endif
  Input::Mapping input_mapping;
  Input input_snapshot;
  void* display = nullptr;
  void* registry = nullptr;
  void* compositor = nullptr;
  void* surface = nullptr;
  U32 logical_width = 0;
  U32 logical_height = 0;
  U32 initial_width = 0;
  U32 initial_height = 0;
  U32 scale = 1;
  Bool close_requested = False;
  Bool failed = False;
  Bool needs_resize = False;
};

}  // namespace Perimortem::System
