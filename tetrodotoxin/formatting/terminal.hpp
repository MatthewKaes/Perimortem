// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "ttx/lexical/tokenizer.hpp"

namespace Tetrodotoxin::Formatting {

// Terminal formats source whose Monograph completed every semantic barrier.
// That handoff allows canonical presentation to normalize documentation while
// the lexical recovery path remains conservative for an unfinished edit.
class Terminal {
 public:
  Terminal() = delete;

  static auto format(
      const Tetrodotoxin::Language::Monograph& monograph,
      const Ttx::Lexical::Tokenizer& tokenizer)
      -> Perimortem::Memory::Dynamic::Bytes;
};

}  // namespace Tetrodotoxin::Formatting
