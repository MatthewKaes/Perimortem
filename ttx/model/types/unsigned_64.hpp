// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/types/unsigned.hpp"

namespace Ttx::Model::Types {

// Unsigned_64 is the standard sixty-four-bit Type implementing the Unsigned
// domain.
class Unsigned_64 final : public Unsigned {
 public:
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Unsigned_64"_view;
  }

  constexpr auto get_documentation() const
      -> const Concept::Documentation& override {
    return documentation;
  }

  constexpr auto get_width() const -> Count override { return 64; }
  constexpr auto get_size() const -> Count override {
    return sizeof(::Unsigned_64);
  }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Unsigned_64);
  }

 private:
  static constexpr Documentations::Comment documentation{
    "Unsigned_64 is stored as an 8 byte unsigned integer."_view,
  };
};

}  // namespace Ttx::Model::Types
