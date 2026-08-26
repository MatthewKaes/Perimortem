// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/field.hpp"
#include "ttx/concept/interface.hpp"

namespace Tetrodotoxin::Library::Language::Interfaces {

// Structure compares one Library Structure requirement with one real Object.
// Public state names and Types form the useful structural promise here. The
// Object keeps its identity and any additional state, while callers can reuse
// the same selected Fields when they later derive a physical Projection.
class Structure : public Ttx::Concept::Interface {
 public:
  auto negotiate(
      const Ttx::Concept::Abstract& requirement,
      const Ttx::Concept::Abstract& candidate) const -> Relation override;

  auto select_field(
      const Tetrodotoxin::Library::Language::Field& requirement,
      const Ttx::Concept::Abstract& candidate) const
      -> Perimortem::Core::Option<
          const Tetrodotoxin::Library::Language::Field&>;

 private:
  static auto compatible_type(
      const Tetrodotoxin::Library::Language::Model::Type& required,
      const Tetrodotoxin::Library::Language::Model::Type& supplied) -> Bool;
};

}  // namespace Tetrodotoxin::Library::Language::Interfaces
