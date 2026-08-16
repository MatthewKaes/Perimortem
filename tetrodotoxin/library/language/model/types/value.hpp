// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/concept/invalid.hpp"

namespace Tetrodotoxin::Library::Language::Model::Types {

// Value is the Library scalar representation protocol. Width is semantic bit
// precision, while size and alignment are target storage facts. Keeping them
// distinct prevents category eligibility from implying promotion or packing.
class Value : public Model::Type {
 public:
  TTX_CONTRACT(Value, Model::Type);

  TTX_INVALID_CONTEXT;

  // Folding may retain the selected Type while producing an incompatible
  // Constant carrier. The scalar domain owns that proof so every consumer can
  // reject malformed folded values without enumerating Constant subclasses.
  virtual auto accepts_constant(const Ttx::Concept::Abstract&) const
      -> Bool = 0;

  virtual constexpr auto get_width() const -> Count = 0;
  virtual constexpr auto get_size() const -> Count = 0;
  virtual constexpr auto get_alignment() const -> Count = 0;
};

}  // namespace Tetrodotoxin::Library::Language::Model::Types
