// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/types/real.hpp"

namespace Ttx::Model::Types {

// Real_64 is the standard 8 byte floating point representation.
// For ABI evaluation it can be used to represent C/C++'s `double`.
class Real_64 final : public Real {
 public:
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Real_64"_view;
  }

  constexpr auto get_documentation() const
      -> const Concept::Documentation& override {
    return documentation;
  }

  constexpr auto get_width() const -> Count override { return 64; }
  constexpr auto get_size() const -> Count override {
    return sizeof(::Real_64);
  }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Real_64);
  }

 private:
  static constexpr Documentations::Comment documentation{
    "Real_64 is stored as an 8 byte IEEE floating value."_view,
  };
};

}  // namespace Ttx::Model::Types
