// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <stdint.h>
#include <wayland-client.h>

#include "perimortem/core/perimortem.hpp"

#include "perimortem/system/platform/wayland/xdg_shell.hpp"

namespace Perimortem::System::Platform::Wayland {

// Owns one Wayland toplevel and translates compositor callbacks into compact
// window state for the application loop. Logical size and compositor scale are
// retained separately because render backends require physical pixel extent.
// The class does not select a renderer or assign graphics meaning to its native
// handles.
class Window {
 public:
  Window() = default;
  Window(U32 width, U32 height, const char* title);
  ~Window();
  Window(Window&&) = delete;
  auto operator=(Window&&) -> Window& = delete;
  Window(const Window&) = delete;
  auto operator=(const Window&) -> Window& = delete;

  auto poll_events() -> Bool;

  auto get_logical_width() const -> U32;
  auto get_logical_height() const -> U32;
  auto get_scale() const -> U32;
  auto get_needs_resize() const -> Bool;
  auto clear_resize() -> void;

  // Native presentation handles are exposed without assigning them graphics
  // meaning. The application selects a renderer and supplies these handles.
  auto get_display() const -> wl_display*;
  auto get_surface() const -> wl_surface*;

 private:
  auto destroy() -> void;

  static auto on_shell_configure(void* data, int32_t width, int32_t height)
      -> void;
  static auto on_shell_close(void* data) -> void;
  static auto on_surface_enter(void*, wl_surface*, wl_output*) -> void;
  static auto on_surface_leave(void*, wl_surface*, wl_output*) -> void;
  static auto on_surface_preferred_buffer_scale(
      void* data,
      wl_surface*,
      int32_t factor) -> void;
  static auto on_surface_preferred_buffer_transform(
      void*,
      wl_surface*,
      uint32_t) -> void;
  static auto on_registry_global(
      void* data,
      wl_registry* registry,
      uint32_t name,
      const char* interface,
      uint32_t) -> void;
  static auto on_registry_global_remove(void*, wl_registry*, uint32_t) -> void;

  static const wl_surface_listener surface_listener;
  static const wl_registry_listener registry_listener;

  wl_display* display = nullptr;
  wl_registry* registry = nullptr;
  wl_compositor* compositor = nullptr;
  wl_surface* surface = nullptr;
  XdgShell shell;
  U32 logical_width = 0;
  U32 logical_height = 0;
  U32 initial_width = 0;
  U32 initial_height = 0;
  U32 scale = 1;
  Bool close_requested = False;
  Bool needs_resize = False;
};

}  // namespace Perimortem::System::Platform::Wayland
