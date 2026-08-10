// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/types/signed.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Signed_16 is the standard sixteen-bit Signed Type.
class Signed_16 : public Ttx::Model::Types::Signed {
 public:
  TTX_NAME("Signed_16"_view);

  TTX_CONSTEXPR_DOCUMENTATION(documentation);

  constexpr auto get_width() const -> Count override { return 16; }
  constexpr auto get_size() const -> Count override {
    return sizeof(::Signed_16);
  }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Signed_16);
  }

 private:
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Signed_16 is stored as a 2 byte two's-complement integer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
