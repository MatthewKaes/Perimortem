// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/types/signed.hpp"

namespace Ttx::Model::Types {

// Signed_16 is the standard sixteen-bit Signed Type.
class Signed_16 final : public Signed {
 public:
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Signed_16"_view;
  }

  constexpr auto get_documentation() const
      -> const Concept::Documentation& override {
    return documentation;
  }

  constexpr auto get_width() const -> Count override { return 16; }
  constexpr auto get_size() const -> Count override {
    return sizeof(::Signed_16);
  }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Signed_16);
  }

 private:
  static constexpr Documentations::Comment documentation{
    "Signed_16 is stored as a 2 byte two's-complement integer."_view,
  };
};

}  // namespace Ttx::Model::Types
