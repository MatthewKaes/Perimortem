// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/types/structure.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Object is the managed nonnull reference specialization of Structure. Its
// category carries Library lifetime semantics while Structure owns every
// member, lookup, Layout, and completion rule.
class Object : public Structure {
 private:
  Object(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Documentation& documentation,
      Visibility visibility,
      Monograph& source,
      Materializations& materializations,
      const Structure& source_scope,
      Ttx::Lexical::Anchor anchor,
      Ttx::Lexical::Anchor name_anchor);

 public:
  using ClassCatagory = Object;
  static constexpr Perimortem::System::Uuid contract_id{
    0xed4871dfefaa4aee,
    0xa13f79099ea383ab,
  };

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Documentation& documentation,
      Visibility visibility,
      Monograph& source,
      Materializations& materializations,
      const Structure& source_scope,
      Ttx::Lexical::Anchor anchor,
      Ttx::Lexical::Anchor name_anchor) -> Object&;

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Structure::implements(requested);
  }
};

}  // namespace Tetrodotoxin::Library::Language::Types
