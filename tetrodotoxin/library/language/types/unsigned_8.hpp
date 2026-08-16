// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "ttx/model/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Unsigned_8 is the standard eight bit Type implementing the Unsigned domain.
// Its name and representation match the Perimortem primitive exactly.
class Unsigned_8 : public Model::Types::Unsigned {
 public:
  TTX_NAME("Unsigned_8"_view);

  TTX_DOCUMENTATION(documentation);

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;
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
