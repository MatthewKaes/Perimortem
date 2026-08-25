// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

#include "perimortem/system/input.hpp"

namespace Perimortem::Abi::System {

// Input is the trivial carrier generated TTX code sees at the native boundary.
// It mirrors one immutable Perimortem snapshot without exposing the C++ class
// or the platform events that produced it.
struct Input {
  U64 current[3];
  U64 changed[3];
  R32 pointer_x;
  R32 pointer_y;
  R32 pointer_delta_x;
  R32 pointer_delta_y;
  R32 scroll_x;
  R32 scroll_y;
  bool pointer_active;
};

static_assert(sizeof(Input) == sizeof(Perimortem::System::Input));
static_assert(alignof(Input) == alignof(Perimortem::System::Input));
static_assert(__is_trivial(Input));
static_assert(__is_standard_layout(Input));

// The application runtime publishes once after collecting a frame. TTX reads
// that value through the C boundary, so every query during the frame observes
// the same snapshot even when native events continue arriving.
auto create_input(const Perimortem::System::Input& input) -> Input;
auto is_current(const Input& input, U8 key) -> Bool;
auto is_changed(const Input& input, U8 key) -> Bool;
auto publish_input(const Perimortem::System::Input& input) -> void;

}  // namespace Perimortem::Abi::System

extern "C" auto perimortem_system_input_snapshot()
    -> Perimortem::Abi::System::Input;
extern "C" auto perimortem_system_input_held(
    Perimortem::Abi::System::Input input,
    U8 key) -> bool;
extern "C" auto perimortem_system_input_pressed(
    Perimortem::Abi::System::Input input,
    U8 key) -> bool;
extern "C" auto perimortem_system_input_released(
    Perimortem::Abi::System::Input input,
    U8 key) -> bool;
