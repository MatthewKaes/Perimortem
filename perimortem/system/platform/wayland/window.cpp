// Perimortem Engine
// Copyright © Matt Kaes

// Wayland's generated C headers can indirectly include libstdc++ <new> on this
// toolchain. Keep them before Perimortem headers so perimortem.hpp sees the
// standard-library guard and does not provide its placement-new stub.
// clang-format off
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>
#include <wayland-client.h>

#include "perimortem/system/platform/wayland/xdg_shell.hpp"
#include "perimortem/system/platform/wayland/window.hpp"
// clang-format on

using namespace Perimortem::System;

const xdg_wm_base_listener Platform::Wayland::Window::wm_base_listener = {
  Platform::Wayland::Window::on_wm_base_ping};

const xdg_surface_listener Platform::Wayland::Window::xdg_surface_events = {
  Platform::Wayland::Window::on_xdg_surface_configure,
};

const xdg_toplevel_listener Platform::Wayland::Window::toplevel_listener = {
  Platform::Wayland::Window::on_toplevel_configure,
  Platform::Wayland::Window::on_toplevel_close,
  Platform::Wayland::Window::on_toplevel_configure_bounds,
  Platform::Wayland::Window::on_toplevel_wm_capabilities,
};

const wl_surface_listener Platform::Wayland::Window::surface_listener = {
  Platform::Wayland::Window::on_surface_enter,
  Platform::Wayland::Window::on_surface_leave,
  Platform::Wayland::Window::on_surface_preferred_buffer_scale,
  Platform::Wayland::Window::on_surface_preferred_buffer_transform,
};

const wl_registry_listener Platform::Wayland::Window::registry_listener = {
  Platform::Wayland::Window::on_registry_global,
  Platform::Wayland::Window::on_registry_global_remove,
};

Platform::Wayland::Window::Window(
    Bits_32 width,
    Bits_32 height,
    const char* title) {
  initial_width = width;
  initial_height = height;
  logical_width = width;
  logical_height = height;

  display = wl_display_connect(nullptr);
  registry = wl_display_get_registry(display);
  wl_registry_add_listener(registry, &registry_listener, this);

  wl_display_roundtrip(display);

  surface = wl_compositor_create_surface(compositor);
  wl_surface_add_listener(surface, &surface_listener, this);

  shell_surface = xdg_wm_base_get_xdg_surface(wm_base, surface);
  xdg_surface_add_listener(shell_surface, &xdg_surface_events, this);

  toplevel = xdg_surface_get_toplevel(shell_surface);
  xdg_toplevel_add_listener(toplevel, &toplevel_listener, this);
  xdg_toplevel_set_title(toplevel, title);
  xdg_toplevel_set_app_id(toplevel, "perimortem");

  wl_surface_commit(surface);
  wl_display_roundtrip(display);
}

Platform::Wayland::Window::~Window() {
  destroy();
}

auto Platform::Wayland::Window::poll_events() -> Bool {
  if (!display || close_requested) {
    return False;
  }

  if (wl_display_dispatch_pending(display) < 0) {
    return False;
  }
  if (close_requested) {
    return False;
  }

  while (wl_display_prepare_read(display) != 0) {
    if (wl_display_dispatch_pending(display) < 0) {
      return False;
    }
    if (close_requested) {
      return False;
    }
  }

  if (wl_display_flush(display) < 0) {
    wl_display_cancel_read(display);
    return False;
  }

  pollfd display_fd = {
    wl_display_get_fd(display),
    POLLIN,
    0,
  };

  auto poll_result = 0;
  do {
    poll_result = poll(&display_fd, 1, 0);
  } while (poll_result < 0 && errno == EINTR);

  if (poll_result < 0) {
    wl_display_cancel_read(display);
    return False;
  }

  if (poll_result > 0 && (display_fd.revents & POLLIN) != 0) {
    if (wl_display_read_events(display) < 0) {
      return False;
    }
  } else {
    wl_display_cancel_read(display);
  }

  if (wl_display_dispatch_pending(display) < 0) {
    return False;
  }
  return !close_requested;
}

auto Platform::Wayland::Window::create_vulkan_surface(VkInstance instance) const
    -> VkSurfaceKHR {
  if (!display || !surface) {
    return VK_NULL_HANDLE;
  }

  VkWaylandSurfaceCreateInfoKHR surface_info = {
    VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
  surface_info.display = display;
  surface_info.surface = surface;

  VkSurfaceKHR surface = VK_NULL_HANDLE;
  if (vkCreateWaylandSurfaceKHR(instance, &surface_info, nullptr, &surface) !=
      VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }
  return surface;
}

auto Platform::Wayland::Window::get_logical_width() const -> Bits_32 {
  return logical_width;
}

auto Platform::Wayland::Window::get_logical_height() const -> Bits_32 {
  return logical_height;
}

auto Platform::Wayland::Window::get_scale() const -> Bits_32 {
  return scale;
}

auto Platform::Wayland::Window::get_needs_resize() const -> Bool {
  return needs_resize;
}

auto Platform::Wayland::Window::clear_resize() -> void {
  needs_resize = False;
}

auto Platform::Wayland::Window::destroy() -> void {
  if (toplevel) {
    xdg_toplevel_destroy(toplevel);
    toplevel = nullptr;
  }
  if (shell_surface) {
    xdg_surface_destroy(shell_surface);
    shell_surface = nullptr;
  }
  if (surface) {
    wl_surface_destroy(surface);
    surface = nullptr;
  }
  if (wm_base) {
    xdg_wm_base_destroy(wm_base);
    wm_base = nullptr;
  }
  if (compositor) {
    wl_compositor_destroy(compositor);
    compositor = nullptr;
  }
  if (registry) {
    wl_registry_destroy(registry);
    registry = nullptr;
  }
  if (display) {
    wl_display_disconnect(display);
    display = nullptr;
  }
}

auto Platform::Wayland::Window::on_wm_base_ping(
    void*,
    xdg_wm_base* wm_base,
    uint32_t serial) -> void {
  xdg_wm_base_pong(wm_base, serial);
}

auto Platform::Wayland::Window::on_xdg_surface_configure(
    void* data,
    xdg_surface* surface,
    uint32_t serial) -> void {
  auto* window = static_cast<Platform::Wayland::Window*>(data);
  xdg_surface_ack_configure(surface, serial);
  wl_surface_commit(window->surface);
}

auto Platform::Wayland::Window::on_toplevel_configure(
    void* data,
    xdg_toplevel*,
    int32_t width,
    int32_t height,
    wl_array*) -> void {
  auto* window = static_cast<Platform::Wayland::Window*>(data);
  auto new_width =
      width > 0 ? static_cast<Bits_32>(width) : window->initial_width;
  auto new_height =
      height > 0 ? static_cast<Bits_32>(height) : window->initial_height;

  if (new_width != window->logical_width ||
      new_height != window->logical_height) {
    window->logical_width = new_width;
    window->logical_height = new_height;
    window->needs_resize = True;
  }
}

auto Platform::Wayland::Window::on_toplevel_close(void* data, xdg_toplevel*)
    -> void {
  static_cast<Platform::Wayland::Window*>(data)->close_requested = True;
}

auto Platform::Wayland::Window::on_toplevel_configure_bounds(
    void*,
    xdg_toplevel*,
    int32_t,
    int32_t) -> void {}

auto Platform::Wayland::Window::on_toplevel_wm_capabilities(
    void*,
    xdg_toplevel*,
    wl_array*) -> void {}

auto Platform::Wayland::Window::on_surface_enter(void*, wl_surface*, wl_output*)
    -> void {}

auto Platform::Wayland::Window::on_surface_leave(void*, wl_surface*, wl_output*)
    -> void {}

auto Platform::Wayland::Window::on_surface_preferred_buffer_scale(
    void* data,
    wl_surface*,
    int32_t factor) -> void {
  auto* window = static_cast<Platform::Wayland::Window*>(data);
  if (static_cast<Bits_32>(factor) != window->scale) {
    window->scale = static_cast<Bits_32>(factor);
    window->needs_resize = True;
  }
}

auto Platform::Wayland::Window::on_surface_preferred_buffer_transform(
    void*,
    wl_surface*,
    uint32_t) -> void {}

auto Platform::Wayland::Window::on_registry_global(
    void* data,
    wl_registry* registry,
    uint32_t name,
    const char* interface,
    uint32_t) -> void {
  auto* window = static_cast<Platform::Wayland::Window*>(data);
  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    window->compositor = static_cast<wl_compositor*>(
        wl_registry_bind(registry, name, &wl_compositor_interface, 6));
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    window->wm_base = static_cast<xdg_wm_base*>(
        wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
    xdg_wm_base_add_listener(
        window->wm_base, &Platform::Wayland::Window::wm_base_listener, window);
  }
}

auto Platform::Wayland::Window::on_registry_global_remove(
    void*,
    wl_registry*,
    uint32_t) -> void {}
