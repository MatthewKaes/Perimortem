// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/app/language/monograph.hpp"

namespace Tetrodotoxin::App::Archive {

// Writer emits one deterministic App policy payload without retaining source
// text, parser state, or a target product.
class Writer {
 public:
  constexpr Writer() = default;

  auto write(const Tetrodotoxin::App::Language::Monograph& monograph) const
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>;
};

}  // namespace Tetrodotoxin::App::Archive
