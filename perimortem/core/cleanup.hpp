// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/option.hpp"

namespace Perimortem::Core {

inline constexpr View::Bytes cleanup_register_symbol =
    "perimortem_core_cleanup_register"_view;

// Cleanup owns dynamically registered destructors whose data follows
// Bibliotheca lifetime. Each worker has one reverse ordered inventory so
// destruction unwinds dynamic initialization on that same worker.
//
// Registration and destruction are thread affine. A destructor runs on the
// worker that registered it, and its Object storage never moves to another
// worker. The worker's Bibliotheca is constructed before this inventory and is
// torn down only after every registered Object has been cleaned up. Reclaiming
// its slabs first would leave Object payloads and cleanup entries pointing into
// unmapped storage.
class Cleanup {
 public:
  using Destructor = void (*)();

  ~Cleanup();

  auto insert(Destructor destructor) -> void;

 private:
  class Entry {
   public:
    constexpr Entry(Destructor destructor, Option<Entry&> previous = {})
        : destructor(destructor), previous(previous) {}

    Destructor destructor;
    Option<Entry&> previous;
  };

  Option<Entry&> latest;
};

}  // namespace Perimortem::Core

extern "C" auto perimortem_core_cleanup_register(
    Perimortem::Core::Cleanup::Destructor destructor) -> void;
