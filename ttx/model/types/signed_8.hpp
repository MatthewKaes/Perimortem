// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/types/signed.hpp"

namespace Ttx::Model::Types {

// Signed_8 is the standard eight-bit Signed Type.
class Signed_8 : public Signed {
 public:
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Signed_8"_view;
  }

  constexpr auto get_documentation() const
      -> const Concept::Documentation& override {
    return documentation;
  }

  constexpr auto get_width() const -> Count override { return 8; }
  constexpr auto get_size() const -> Count override {
    return sizeof(::Signed_8);
  }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Signed_8);
  }

 private:
  static constexpr Documentations::Comment documentation{
    "Signed_8 is stored as a 1 byte two's-complement integer."_view,
  };
};

}  // namespace Ttx::Model::Types
