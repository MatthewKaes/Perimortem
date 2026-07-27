// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/types/signed.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Signed_64 is the standard sixty-four-bit Signed Type.
class Signed_64 : public Ttx::Model::Types::Signed {
 public:
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Signed_64"_view;
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }

  constexpr auto get_width() const -> Count override { return 64; }
  constexpr auto get_size() const -> Count override {
    return sizeof(::Signed_64);
  }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Signed_64);
  }

 private:
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Signed_64 is stored as an 8 byte two's-complement integer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
