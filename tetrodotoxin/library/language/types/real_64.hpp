// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/types/real.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Real_64 is the standard 8 byte floating point representation.
// For ABI evaluation it can be used to represent C/C++'s `double`.
class Real_64 : public Ttx::Model::Types::Real {
 public:
  TTX_NAME("Real_64"_view);

  TTX_CONSTEXPR_DOCUMENTATION(documentation);

  constexpr auto get_width() const -> Count override { return 64; }
  constexpr auto get_size() const -> Count override {
    return sizeof(::Real_64);
  }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Real_64);
  }

 private:
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Real_64 is stored as an 8 byte IEEE floating value."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
