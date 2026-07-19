// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/types/flag.hpp"

namespace Ttx::Model::Types {

// Boolean is the standard one-bit Flag Type with byte-addressable storage.
class Boolean final : public Flag {
 public:
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return "Bool"_view;
  }

  constexpr auto get_documentation() const
      -> const Concept::Documentation& override {
    return documentation;
  }

  constexpr auto get_width() const -> Count override { return 1; }
  constexpr auto get_size() const -> Count override { return sizeof(::Bool); }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Bool);
  }

 private:
  static constexpr Documentations::Comment documentation{
    "Bool is stored as a 1 byte logical value."_view,
  };
};

}  // namespace Ttx::Model::Types
