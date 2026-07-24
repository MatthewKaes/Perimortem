// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/types/signed.hpp"

namespace Ttx::Model::Types {

// Signed_32 is the standard thirty-two-bit Signed Type.
class Signed_32 : public Signed {
 public:
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Signed_32"_view;
  }

  constexpr auto get_documentation() const
      -> const Concept::Documentation& override {
    return documentation;
  }

  constexpr auto get_width() const -> Count override { return 32; }
  constexpr auto get_size() const -> Count override {
    return sizeof(::Signed_32);
  }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Signed_32);
  }

 private:
  static constexpr Documentations::Comment documentation{
    "Signed_32 is stored as a 4 byte two's-complement integer."_view,
  };
};

}  // namespace Ttx::Model::Types
