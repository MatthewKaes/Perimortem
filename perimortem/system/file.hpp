// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/utility/option.hpp"

namespace Perimortem::System {

// Stateless filesystem transactions.
class File {
 public:
  static auto read(Core::View::Bytes location)
      -> Utility::Option<Memory::Dynamic::Bytes>;
  static auto write(Core::View::Bytes data, Core::View::Bytes location) -> Bool;
  static auto remove(Core::View::Bytes location) -> Bool;
  static auto exists(Core::View::Bytes location) -> Bool;
};

}  // namespace Perimortem::System
