// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/types/real.hpp"

namespace Ttx::Model::Types {

// Real_64 is the standard 4 byte floating point representation.
// For ABI evaluation it can be used to represent C/C++'s `float`.
class Real_32 : public Real {
 public:
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Real_32"_view;
  }

  constexpr auto get_documentation() const
      -> const Concept::Documentation& override {
    return documentation;
  }

  constexpr auto get_width() const -> Count override { return 32; }
  constexpr auto get_size() const -> Count override {
    return sizeof(::Real_32);
  }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Real_32);
  }

 private:
  static constexpr Documentations::Comment documentation{
    "Real_32 is stored as a 4 byte IEEE floating value."_view,
  };
};

}  // namespace Ttx::Model::Types
