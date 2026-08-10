// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/types/signed.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Signed_8 is the standard eight-bit Signed Type.
class Signed_8 : public Ttx::Model::Types::Signed {
 public:
  TTX_NAME("Signed_8"_view);

  TTX_CONSTEXPR_DOCUMENTATION(documentation);

  constexpr auto get_width() const -> Count override { return 8; }
  constexpr auto get_size() const -> Count override {
    return sizeof(::Signed_8);
  }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Signed_8);
  }

 private:
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Signed_8 is stored as a 1 byte two's-complement integer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
