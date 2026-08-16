// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "ttx/model/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Unsigned_16 is the standard sixteen bit Type implementing the Unsigned
// domain.
class Unsigned_16 : public Model::Types::Unsigned {
 public:
  TTX_NAME("Unsigned_16"_view);

  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;
  constexpr auto get_width() const -> Count override { return 16; }
  constexpr auto get_size() const -> Count override {
    return sizeof(::Unsigned_16);
  }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Unsigned_16);
  }

 private:
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Unsigned_16 is stored as a 2 byte unsigned integer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
