// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "ttx/model/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Signed_64 is the standard sixty four bit Signed Type.
class Signed_64 : public Model::Types::Signed {
 public:
  TTX_NAME("Signed_64"_view);

  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;
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
