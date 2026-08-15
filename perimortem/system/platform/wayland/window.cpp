// Perimortem Engine
// Copyright © Matt Kaes

// Wayland's C headers can indirectly include libstdc++ <new> on this toolchain.
// Keep them before Perimortem headers so perimortem.hpp sees the standard-
// library guard and does not provide its placement-new stub.
// clang-format off
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <wayland-client.h>

#include "perimortem/system/platform/wayland/xdg_shell.hpp"
#include "perimortem/system/platform/wayland/window.hpp"
// clang-format on

using namespace Perimortem::System;

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
    Unsigned_32 width,
    Unsigned_32 height,
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

  if (!shell.create_toplevel(
          surface, title, "perimortem",
          {
            this,
            &Platform::Wayland::Window::on_shell_configure,
            &Platform::Wayland::Window::on_shell_close,
          })) {
    close_requested = True;
    return;
  }

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
    int events_read = wl_display_read_events(display);
    if (events_read < 0) {
      return False;
    }
  } else {
    wl_display_cancel_read(display);
  }

  int events_dispatched = wl_display_dispatch_pending(display);
  if (events_dispatched < 0) {
    return False;
  }

  return !close_requested;
}

auto Platform::Wayland::Window::get_logical_width() const -> Unsigned_32 {
  return logical_width;
}

auto Platform::Wayland::Window::get_logical_height() const -> Unsigned_32 {
  return logical_height;
}

auto Platform::Wayland::Window::get_scale() const -> Unsigned_32 {
  return scale;
}

auto Platform::Wayland::Window::get_needs_resize() const -> Bool {
  return needs_resize;
}

auto Platform::Wayland::Window::clear_resize() -> void {
  needs_resize = False;
}

auto Platform::Wayland::Window::get_display() const -> wl_display* {
  return display;
}

auto Platform::Wayland::Window::get_surface() const -> wl_surface* {
  return surface;
}

auto Platform::Wayland::Window::destroy() -> void {
  shell.destroy();

  if (surface) {
    wl_surface_destroy(surface);
    surface = nullptr;
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

auto Platform::Wayland::Window::on_shell_configure(
    void* data,
    int32_t width,
    int32_t height) -> void {
  auto* window = static_cast<Platform::Wayland::Window*>(data);
  auto new_width =
      width > 0 ? static_cast<Unsigned_32>(width) : window->initial_width;
  auto new_height =
      height > 0 ? static_cast<Unsigned_32>(height) : window->initial_height;
  if (new_width != window->logical_width ||
      new_height != window->logical_height) {
    window->logical_width = new_width;
    window->logical_height = new_height;
    window->needs_resize = True;
  }
}

auto Platform::Wayland::Window::on_shell_close(void* data) -> void {
  static_cast<Platform::Wayland::Window*>(data)->close_requested = True;
}

auto Platform::Wayland::Window::on_surface_enter(void*, wl_surface*, wl_output*)
    -> void {}

auto Platform::Wayland::Window::on_surface_leave(void*, wl_surface*, wl_output*)
    -> void {}

auto Platform::Wayland::Window::on_surface_preferred_buffer_scale(
    void* data,
    wl_surface*,
    int32_t factor) -> void {
  auto* window = static_cast<Platform::Wayland::Window*>(data);
  if (static_cast<Unsigned_32>(factor) != window->scale) {
    window->scale = static_cast<Unsigned_32>(factor);
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
  } else if (
      Platform::Wayland::XdgShell::recognizes(interface) &&
      !window->shell.bind(registry, name)) {
    window->close_requested = True;
  }
}

auto Platform::Wayland::Window::on_registry_global_remove(
    void*,
    wl_registry*,
    uint32_t) -> void {}
