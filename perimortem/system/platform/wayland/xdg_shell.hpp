// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <stdint.h>
#include <wayland-client.h>

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::System::Platform::Wayland {

// Owns the XDG protocol proxies that turn one Wayland surface into a toplevel.
// Protocol objects are generic Wayland proxies at the ABI boundary; keeping
// those handles here avoids manufacturing incomplete C++ object types for each
// XML protocol name.
class XdgShell {
 public:
  struct Events {
    void* context = nullptr;
    void (*configure)(void*, int32_t, int32_t) = nullptr;
    void (*close)(void*) = nullptr;
  };

  XdgShell() = default;
  ~XdgShell();
  XdgShell(XdgShell&&) = delete;
  auto operator=(XdgShell&&) -> XdgShell& = delete;
  XdgShell(const XdgShell&) = delete;
  auto operator=(const XdgShell&) -> XdgShell& = delete;

  static auto recognizes(const char* interface) -> Bool;

  auto bind(wl_registry* registry, uint32_t name) -> Bool;
  auto create_toplevel(
      wl_surface* wayland_surface,
      const char* title,
      const char* application_id,
      Events events) -> Bool;
  auto destroy() -> void;

 private:
  using ListenerFunction = void (*)(void);

  static auto on_ping(void* data, wl_proxy*, uint32_t serial) -> void;
  static auto on_surface_configure(void* data, wl_proxy*, uint32_t serial)
      -> void;
  static auto on_toplevel_configure(
      void* data,
      wl_proxy*,
      int32_t width,
      int32_t height,
      wl_array*) -> void;
  static auto on_toplevel_close(void* data, wl_proxy*) -> void;
  static auto on_toplevel_configure_bounds(void*, wl_proxy*, int32_t, int32_t)
      -> void;
  static auto on_toplevel_wm_capabilities(void*, wl_proxy*, wl_array*) -> void;

  Events events;
  wl_proxy* wm_base = nullptr;
  wl_proxy* surface = nullptr;
  wl_proxy* toplevel = nullptr;
  wl_surface* wayland_surface = nullptr;
};

}  // namespace Perimortem::System::Platform::Wayland
