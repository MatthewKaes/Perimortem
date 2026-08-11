// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/types/structure.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Object is the managed nonnull reference specialization of Structure. It
// retains the mandatory authored Definition through Structure while Composite
// owns every member, lookup, Layout, and completion rule.
class Object : public Structure {
 private:
  Object(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition);

 public:
  TTX_CONTRACT(Object, Structure, 0xed4871dfefaa4aee, 0xa13f79099ea383ab);

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Object&>;
};

}  // namespace Tetrodotoxin::Library::Language::Types
