// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Perimortem::System {

// Stateless filesystem transactions. read returns an empty buffer for both an
// empty file and a failed read; use exists when that distinction matters.
class File {
 public:
  static auto read(Core::View::Bytes location) -> Memory::Dynamic::Bytes;
  static auto write(Core::View::Bytes data, Core::View::Bytes location) -> Bool;
  static auto remove(Core::View::Bytes location) -> Bool;
  static auto exists(Core::View::Bytes location) -> Bool;

 private:
  static constexpr Count max_path_size = 512;
  static auto create_path(Bits_8* output, Core::View::Bytes path)
      -> const Signed_8*;
};

}  // namespace Perimortem::System
