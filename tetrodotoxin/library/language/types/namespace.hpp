// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/types/composite.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Namespace is a contextual Type that publishes only nested Types and Aliases,
// leaving value Layout, state, construction, and Callable behavior to the
// declarations reached through those routes.
class Namespace : public Composite {
 public:

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition) -> Namespace&;

  static auto create_restored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition) -> Namespace&;

  auto complete_body() -> void override;

 protected:
  auto retain_binding(
      Ttx::Concept::Abstract& binding,
      Tetrodotoxin::Language::Definition& definition,
      Category category,
      Ttx::Lexical::Cursor& cursor) -> Bool override;

 private:
  Namespace(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition)
      : Composite(domain, definition) {}
};

}  // namespace Tetrodotoxin::Library::Language::Types
