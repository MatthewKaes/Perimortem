// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

#include "perimortem/system/platform/window.hpp"

namespace Perimortem::System::Platform::Wayland {

class Window final : public Platform::Window {
 public:
  static auto create(Bits_32 width, Bits_32 height, const char* title)
      -> Window;

  Window() = default;
  ~Window() override;
  Window(Window&&) noexcept;
  auto operator=(Window&&) noexcept -> Window&;
  Window(const Window&) = delete;
  auto operator=(const Window&) = delete;

  auto poll_events() -> Bool override;
  auto get_native_handles() const -> NativeWindowHandles override;
  auto get_logical_width() const -> Bits_32 override;
  auto get_logical_height() const -> Bits_32 override;
  auto get_scale() const -> Bits_32 override;
  auto get_needs_resize() const -> Bool override;
  auto clear_resize() -> void override;

 private:
  void* handle = nullptr;
};

}  // namespace Perimortem::System::Platform::Wayland
