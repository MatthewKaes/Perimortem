// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/null_terminated.hpp"

#include "ttx/concept/abstract.hpp"

namespace Ttx {

// Interface negotiation passes one exact Abstract rather than asking every
// participant to reconstruct a contract from text, a hash, or a C++ type
// address. A Requirement owns the bytes used to describe that Abstract so its
// C handle can lend them without allocation or another lifetime dependency.
// The bytes are initialized at compile time, while the Requirement's capability
// address keeps equal spellings from becoming equal identities.
template <Count size>
class Requirement final : public Abstract {
 public:
  constexpr explicit Requirement(Perimortem::Core::Static::Bytes<size> name)
      : value(name) {}

  constexpr auto name() const -> ttx_borrowed_bytes override {
    return {
      .data = value.get_data(),
      .size = value.get_size(),
    };
  }

 private:
  const Perimortem::Core::Static::Bytes<size> value;
};

template <Count size>
Requirement(Perimortem::Core::Static::Bytes<size>) -> Requirement<size>;

}  // namespace Ttx
