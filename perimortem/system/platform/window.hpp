// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::System::Platform {

// C API marshaling type; returned by value from get_native_handles and used
// only to forward opaque OS handles to the graphics API surface factory.
struct NativeWindowHandles {
  void* display;
  void* surface;
};

class Window {
 public:
  virtual ~Window() = default;

  virtual auto poll_events() -> Bool = 0;
  virtual auto get_native_handles() const -> NativeWindowHandles = 0;
  virtual auto get_logical_width() const -> Bits_32 = 0;
  virtual auto get_logical_height() const -> Bits_32 = 0;
  virtual auto get_scale() const -> Bits_32 = 0;
  virtual auto get_needs_resize() const -> Bool = 0;
  virtual auto clear_resize() -> void = 0;
};

}  // namespace Perimortem::System::Platform
