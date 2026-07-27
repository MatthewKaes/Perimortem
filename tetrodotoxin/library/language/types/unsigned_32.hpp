// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/types/unsigned.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Unsigned_32 is the standard thirty-two-bit Type implementing the Unsigned
// domain.
class Unsigned_32 : public Ttx::Model::Types::Unsigned {
 public:
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Unsigned_32"_view;
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }

  constexpr auto get_width() const -> Count override { return 32; }
  constexpr auto get_size() const -> Count override {
    return sizeof(::Unsigned_32);
  }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Unsigned_32);
  }

 private:
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Unsigned_32 is stored as a 4 byte unsigned integer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
