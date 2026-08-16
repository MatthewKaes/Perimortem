// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "ttx/model/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Signed_32 is the standard thirty two bit Signed Type.
class Signed_32 : public Model::Types::Signed {
 public:
  TTX_NAME("Signed_32"_view);

  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;
  constexpr auto get_width() const -> Count override { return 32; }
  constexpr auto get_size() const -> Count override {
    return sizeof(::Signed_32);
  }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Signed_32);
  }

 private:
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Signed_32 is stored as a 4 byte two's-complement integer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
