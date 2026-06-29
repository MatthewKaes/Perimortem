// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

namespace Perimortem::Core::Writer {
class Textual;
}

namespace Perimortem::Serialization {

class EscapedText {
 public:
  enum class Style : Bits_8 {
    Json,
  };

  static auto decode(
      Memory::Allocator::Arena& arena,
      Core::View::Bytes source,
      Style style = Style::Json) -> Core::View::Bytes;

  static auto encoded_size(Core::View::Bytes source, Style style = Style::Json)
      -> Count;

  static auto encode(
      Core::Writer::Textual& output,
      Core::View::Bytes source,
      Style style = Style::Json) -> void;
};

}  // namespace Perimortem::Serialization
