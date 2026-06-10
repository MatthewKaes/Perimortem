// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/platform/wayland/window.hpp"

#include "perimortem/system/platform/wayland/wayland_api.h"

using namespace Perimortem::System;

auto Platform::Wayland::Window::create(
    Bits_32 width,
    Bits_32 height,
    const char* title) -> Window {
  Window win;
  win.handle = peri_wayland_create(width, height, title);
  return win;
}

Platform::Wayland::Window::~Window() {
  peri_wayland_destroy(static_cast<PeriWaylandWindow*>(handle));
  handle = nullptr;
}

Platform::Wayland::Window::Window(Window&& other) noexcept
    : handle(other.handle) {
  other.handle = nullptr;
}

auto Platform::Wayland::Window::operator=(Window&& other) noexcept -> Window& {
  if (this != &other) {
    this->~Window();
    handle = other.handle;
    other.handle = nullptr;
  }
  return *this;
}

auto Platform::Wayland::Window::poll_events() -> Bool {
  return Bool(peri_wayland_poll(static_cast<PeriWaylandWindow*>(handle)));
}

auto Platform::Wayland::Window::get_native_handles() const
    -> NativeWindowHandles {
  const auto raw =
      peri_wayland_get_handles(static_cast<const PeriWaylandWindow*>(handle));
  return NativeWindowHandles{.display = raw.display, .surface = raw.surface};
}

auto Platform::Wayland::Window::get_logical_width() const -> Bits_32 {
  return peri_wayland_get_width(static_cast<const PeriWaylandWindow*>(handle));
}

auto Platform::Wayland::Window::get_logical_height() const -> Bits_32 {
  return peri_wayland_get_height(static_cast<const PeriWaylandWindow*>(handle));
}

auto Platform::Wayland::Window::get_scale() const -> Bits_32 {
  return peri_wayland_get_scale(static_cast<const PeriWaylandWindow*>(handle));
}

auto Platform::Wayland::Window::get_needs_resize() const -> Bool {
  return Bool(
      peri_wayland_needs_resize(static_cast<const PeriWaylandWindow*>(handle)));
}

auto Platform::Wayland::Window::clear_resize() -> void {
  peri_wayland_clear_resize(static_cast<PeriWaylandWindow*>(handle));
}
