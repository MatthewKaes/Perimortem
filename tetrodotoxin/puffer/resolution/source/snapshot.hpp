// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Tetrodotoxin::Puffer::Resolution::Source {

// Source input retained while an invalidated consumer is replayed.
class Snapshot {
 public:
  Snapshot(
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source_text)
      : source_path(source_path), source_text(source_text) {}

  constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return source_path;
  }

  constexpr auto get_source_text() const -> Perimortem::Core::View::Bytes {
    return source_text;
  }

 private:
  Perimortem::Memory::Dynamic::Bytes source_path;
  Perimortem::Memory::Dynamic::Bytes source_text;
};

}  // namespace Tetrodotoxin::Puffer::Resolution::Source
