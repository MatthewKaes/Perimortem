// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/types/flag.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Boolean is the standard one-bit Flag Type with byte-addressable storage.
class Boolean : public Ttx::Model::Types::Flag {
 public:
  TTX_NAME("Bool"_view);

  TTX_DOCUMENTATION(documentation);

  constexpr auto get_width() const -> Count override { return 1; }
  constexpr auto get_size() const -> Count override { return sizeof(::Bool); }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Bool);
  }

 private:
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Bool is stored as a 1 byte logical value."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
