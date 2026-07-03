// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <vulkan/vulkan.h>

#include "perimortem/core/perimortem.hpp"

struct wl_array;
struct wl_compositor;
struct wl_display;
struct wl_output;
struct wl_registry;
struct wl_registry_listener;
struct wl_surface;
struct wl_surface_listener;
struct xdg_surface;
struct xdg_surface_listener;
struct xdg_toplevel;
struct xdg_toplevel_listener;
struct xdg_wm_base;
struct xdg_wm_base_listener;

namespace Perimortem::System::Platform::Wayland {

class Window {
 public:
  Window() = default;
  Window(Bits_32 width, Bits_32 height, const char* title);
  ~Window();
  Window(Window&&) = delete;
  auto operator=(Window&&) -> Window& = delete;
  Window(const Window&) = delete;
  auto operator=(const Window&) -> Window& = delete;

  auto poll_events() -> Bool;
  auto create_vulkan_surface(VkInstance instance) const -> VkSurfaceKHR;

  auto get_logical_width() const -> Bits_32;
  auto get_logical_height() const -> Bits_32;
  auto get_scale() const -> Bits_32;
  auto get_needs_resize() const -> Bool;
  auto clear_resize() -> void;

 private:
  auto destroy() -> void;

  static auto on_wm_base_ping(void*, xdg_wm_base* wm_base, uint32_t serial)
      -> void;
  static auto on_xdg_surface_configure(
      void* data,
      xdg_surface* surface,
      uint32_t serial) -> void;
  static auto on_toplevel_configure(
      void* data,
      xdg_toplevel*,
      int32_t width,
      int32_t height,
      wl_array*) -> void;
  static auto on_toplevel_close(void* data, xdg_toplevel*) -> void;
  static auto
      on_toplevel_configure_bounds(void*, xdg_toplevel*, int32_t, int32_t)
          -> void;
  static auto on_toplevel_wm_capabilities(void*, xdg_toplevel*, wl_array*)
      -> void;
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

  static const xdg_wm_base_listener wm_base_listener;
  static const xdg_surface_listener xdg_surface_events;
  static const xdg_toplevel_listener toplevel_listener;
  static const wl_surface_listener surface_listener;
  static const wl_registry_listener registry_listener;

  wl_display* display = nullptr;
  wl_registry* registry = nullptr;
  wl_compositor* compositor = nullptr;
  wl_surface* surface = nullptr;
  xdg_wm_base* wm_base = nullptr;
  xdg_surface* shell_surface = nullptr;
  xdg_toplevel* toplevel = nullptr;
  Bits_32 logical_width = 0;
  Bits_32 logical_height = 0;
  Bits_32 initial_width = 0;
  Bits_32 initial_height = 0;
  Bits_32 scale = 1;
  Bool close_requested = False;
  Bool needs_resize = False;
};

}  // namespace Perimortem::System::Platform::Wayland
