// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/map.hpp"

#include "perimortem/utility/range.hpp"

namespace Tetrodotoxin::Compiler::Context {

class Strings {
 public:
  auto add_string(
      Perimortem::Core::View::Bytes string_value,
      Count symbol_index) -> Perimortem::Utility::Range {
    Count start = data.get_size();
    data.concat(string_value);

    return {start, data.get_size()};
  }

  auto get_data() const -> Perimortem::Core::View::Bytes { return data; }

 private:
  Perimortem::Memory::Dynamic::Bytes data;
};

}  // namespace Tetrodotoxin::Compiler::Context
