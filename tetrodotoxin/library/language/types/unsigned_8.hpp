// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/types/unsigned.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Unsigned_8 is the standard eight-bit Type implementing the Unsigned domain.
// Its name and representation match the Perimortem primitive exactly.
class Unsigned_8 : public Ttx::Model::Types::Unsigned {
 public:
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Unsigned_8"_view;
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }

  constexpr auto get_width() const -> Count override { return 8; }
  constexpr auto get_size() const -> Count override {
    return sizeof(::Unsigned_8);
  }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Unsigned_8);
  }

 private:
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Unsigned_8 is stored as a 1 byte unsigned integer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
