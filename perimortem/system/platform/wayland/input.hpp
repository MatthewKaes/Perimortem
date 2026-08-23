// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <stdint.h>
#include <wayland-client.h>

#include "perimortem/core/static/vector.hpp"

#include "perimortem/system/input.hpp"

namespace Perimortem::System::Platform::Wayland {

// Input owns the Wayland seat objects that feed one Window. Native callbacks
// retain only physical state and pointer motion. collect() then publishes one
// immutable System snapshot after applying the caller's virtual key mapping.
class Input {
 public:
  Input() = default;
  ~Input();
  Input(Input&&) = delete;
  auto operator=(Input&&) -> Input& = delete;
  Input(const Input&) = delete;
  auto operator=(const Input&) -> Input& = delete;

  auto attach(wl_surface* surface) -> Bool;
  auto register_global(
      wl_registry* registry,
      U32 name,
      const char* interface,
      U32 version) -> void;
  auto remove_global(U32 name) -> void;
  auto collect(const System::Input::Mapping& mapping) -> System::Input;
  auto destroy() -> void;

  static auto translate_keyboard(U32 code) -> System::Input::Key;
  static auto translate_button(U32 code) -> System::Input::Key;

 private:
  static constexpr Count word_bits = sizeof(U64) * 8;
  static constexpr Count word_count =
      (System::Input::key_count + word_bits - 1) / word_bits;
  using KeyBits = Core::Static::Vector<U64, word_count>;

  auto set_key(System::Input::Key key, Bool down) -> void;
  auto clear_keyboard() -> void;
  auto clear_pointer() -> void;
  auto release_devices() -> void;

  static auto on_seat_capabilities(void* data, wl_seat*, uint32_t capabilities)
      -> void;
  static auto on_seat_name(void*, wl_seat*, const char*) -> void;

  static auto on_keyboard_keymap(
      void*,
      wl_keyboard*,
      uint32_t,
      int32_t descriptor,
      uint32_t) -> void;
  static auto on_keyboard_enter(
      void* data,
      wl_keyboard*,
      uint32_t,
      wl_surface* surface,
      wl_array* keys) -> void;
  static auto
      on_keyboard_leave(void* data, wl_keyboard*, uint32_t, wl_surface* surface)
          -> void;
  static auto on_keyboard_key(
      void* data,
      wl_keyboard*,
      uint32_t,
      uint32_t,
      uint32_t key,
      uint32_t state) -> void;
  static auto on_keyboard_modifiers(
      void*,
      wl_keyboard*,
      uint32_t,
      uint32_t,
      uint32_t,
      uint32_t,
      uint32_t) -> void;
  static auto on_keyboard_repeat(void*, wl_keyboard*, int32_t, int32_t) -> void;

  static auto on_pointer_enter(
      void* data,
      wl_pointer*,
      uint32_t,
      wl_surface* surface,
      wl_fixed_t x,
      wl_fixed_t y) -> void;
  static auto
      on_pointer_leave(void* data, wl_pointer*, uint32_t, wl_surface* surface)
          -> void;
  static auto on_pointer_motion(
      void* data,
      wl_pointer*,
      uint32_t,
      wl_fixed_t x,
      wl_fixed_t y) -> void;
  static auto on_pointer_button(
      void* data,
      wl_pointer*,
      uint32_t,
      uint32_t,
      uint32_t button,
      uint32_t state) -> void;
  static auto on_pointer_axis(
      void* data,
      wl_pointer*,
      uint32_t,
      uint32_t axis,
      wl_fixed_t value) -> void;
  static auto on_pointer_frame(void*, wl_pointer*) -> void;
  static auto on_pointer_axis_source(void*, wl_pointer*, uint32_t) -> void;
  static auto on_pointer_axis_stop(void*, wl_pointer*, uint32_t, uint32_t)
      -> void;
  static auto on_pointer_axis_discrete(void*, wl_pointer*, uint32_t, int32_t)
      -> void;
  static auto on_pointer_axis_value120(void*, wl_pointer*, uint32_t, int32_t)
      -> void;
  static auto on_pointer_axis_direction(void*, wl_pointer*, uint32_t, uint32_t)
      -> void;

  static const wl_seat_listener seat_listener;
  static const wl_keyboard_listener keyboard_listener;
  static const wl_pointer_listener pointer_listener;

  wl_seat* seat = nullptr;
  wl_keyboard* keyboard = nullptr;
  wl_pointer* pointer = nullptr;
  wl_surface* surface = nullptr;
  U32 seat_name = 0;
  KeyBits current;
  KeyBits previous;
  R32 pointer_x = 0;
  R32 pointer_y = 0;
  R32 previous_pointer_x = 0;
  R32 previous_pointer_y = 0;
  R32 scroll_x = 0;
  R32 scroll_y = 0;
  Bool keyboard_focused = False;
  Bool pointer_focused = False;
  Bool previous_pointer_focused = False;
};

}  // namespace Perimortem::System::Platform::Wayland
