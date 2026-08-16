// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "ttx/model/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Real_32 is the standard 4 byte floating point representation.
// For ABI evaluation it can be used to represent C/C++'s `float`.
class Real_32 : public Model::Types::Real {
 public:
  TTX_NAME("Real_32"_view);

  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;
  constexpr auto get_width() const -> Count override { return 32; }
  constexpr auto get_size() const -> Count override {
    return sizeof(::Real_32);
  }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Real_32);
  }

 private:
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Real_32 is stored as a 4 byte IEEE floating value."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
