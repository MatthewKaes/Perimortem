// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/system/input.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;

auto Input::set(KeyBits& keys, Key key) -> void {
  Count index = Count(key);
  if (key == Key::None || index >= key_count) {
    return;
  }
  keys[index / word_bits] |= U64(1) << (index % word_bits);
}

auto Input::contains(const KeyBits& keys, Key key) -> Bool {
  Count index = Count(key);
  if (key == Key::None || index >= key_count) {
    return False;
  }
  return Bool((keys[index / word_bits] & (U64(1) << (index % word_bits))) != 0);
}

auto Input::add_modifier_groups(KeyBits& keys) -> void {
  if (contains(keys, Key::LeftShift) || contains(keys, Key::RightShift)) {
    set(keys, Key::Shift);
  }
  if (contains(keys, Key::LeftControl) || contains(keys, Key::RightControl)) {
    set(keys, Key::Control);
  }
  if (contains(keys, Key::LeftAlt) || contains(keys, Key::RightAlt)) {
    set(keys, Key::Alt);
  }
  if (contains(keys, Key::LeftSuper) || contains(keys, Key::RightSuper)) {
    set(keys, Key::Super);
  }
}

auto Input::collect(View::Vector<Key> keys, const Mapping& mapping) -> KeyBits {
  KeyBits output;
  for (Key physical : keys) {
    set(output, mapping.resolve(physical));
  }
  add_modifier_groups(output);
  return output;
}

auto Input::create(View::Vector<Key> current, View::Vector<Key> previous)
    -> Input {
  return create(current, previous, Mapping(), Pointer());
}

auto Input::create(
    View::Vector<Key> current,
    View::Vector<Key> previous,
    Pointer pointer) -> Input {
  return create(current, previous, Mapping(), pointer);
}

auto Input::create(
    View::Vector<Key> current,
    View::Vector<Key> previous,
    const Mapping& mapping) -> Input {
  return create(current, previous, mapping, Pointer());
}

auto Input::create(
    View::Vector<Key> current_keys,
    View::Vector<Key> previous_keys,
    const Mapping& mapping,
    Pointer pointer) -> Input {
  KeyBits current = collect(current_keys, mapping);
  KeyBits previous = collect(previous_keys, mapping);
  KeyBits changed;
  for (Count index = 0; index < word_count; index++) {
    changed[index] = current[index] ^ previous[index];
  }
  return Input(current, changed, pointer);
}

auto Input::is_current(Key key) const -> Bool {
  return contains(current, key);
}

auto Input::is_pressed(Key key) const -> Bool {
  return contains(current, key) && contains(changed, key);
}

auto Input::is_released(Key key) const -> Bool {
  return !contains(current, key) && contains(changed, key);
}

static thread_local perimortem_system_input current_input;

auto Perimortem::System::create_input(const Input& input)
    -> perimortem_system_input {
  perimortem_system_input output = {};
  for (Count index = 0; index < 3; index++) {
    output.current[index] = input.get_current_word(index);
    output.changed[index] = input.get_changed_word(index);
  }
  const Input::Pointer& pointer = input.get_pointer();
  output.pointer[0] = pointer.x;
  output.pointer[1] = pointer.y;
  output.pointer_delta[0] = pointer.delta_x;
  output.pointer_delta[1] = pointer.delta_y;
  output.scroll[0] = pointer.scroll_x;
  output.scroll[1] = pointer.scroll_y;
  output.pointer_active = pointer.active.value;
  return output;
}

static auto snapshot_is_current(perimortem_system_input input, U8 key)
    -> Bool {
  return key != 0 && key < U8(Input::Key::Count) &&
         Bool((input.current[key / 64] & (U64(1) << (key % 64))) != 0);
}

static auto snapshot_is_changed(perimortem_system_input input, U8 key)
    -> Bool {
  return key != 0 && key < U8(Input::Key::Count) &&
         Bool((input.changed[key / 64] & (U64(1) << (key % 64))) != 0);
}

auto Perimortem::System::publish_input(const Input& input) -> void {
  current_input = create_input(input);
}

extern "C" auto perimortem_system_input_snapshot()
    -> perimortem_system_input {
  return current_input;
}

extern "C" auto perimortem_system_input_held(
    perimortem_system_input input,
    U8 key) -> perimortem_bool {
  return snapshot_is_current(input, key).value;
}

extern "C" auto perimortem_system_input_pressed(
    perimortem_system_input input,
    U8 key) -> perimortem_bool {
  return snapshot_is_current(input, key) && snapshot_is_changed(input, key)
             ? PERIMORTEM_TRUE
             : PERIMORTEM_FALSE;
}

extern "C" auto perimortem_system_input_released(
    perimortem_system_input input,
    U8 key) -> perimortem_bool {
  return !snapshot_is_current(input, key) && snapshot_is_changed(input, key)
             ? PERIMORTEM_TRUE
             : PERIMORTEM_FALSE;
}
