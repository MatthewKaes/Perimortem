// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language {

// Monograph is the semantic root produced in one source Arena. Workspace owns
// that Arena and retains it only after the complete source succeeds.
class Monograph : public Ttx::Concept::Abstract {
 public:
  TTX_CONTRACT(Monograph, Ttx::Concept::Abstract);

  virtual ~Monograph() = 0;

  Monograph(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Abstract& language,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context);

  Monograph(const Monograph&) = delete;
  Monograph(Monograph&&) = delete;
  auto operator=(const Monograph&) -> Monograph& = delete;
  auto operator=(Monograph&&) -> Monograph& = delete;

  TTX_DOCUMENTATION(documentation);

  constexpr auto get_language() const -> const Ttx::Concept::Abstract& {
    return language;
  }

  // Layer selection compares an installed Abstract context by exact live
  // identity. Contract type identities and authored names never participate.
  virtual auto get_layer(const Ttx::Concept::Abstract& requested) const
      -> Perimortem::Core::Option<const Monograph&>;

  // Linking may connect declarations only after every source in the enclosing
  // transaction has established its stable graph identities. Finalization then
  // validates those completed edges in the transaction's second barrier.
  virtual auto link(Ttx::Lexical::Cursor& cursor) -> Bool;
  virtual auto finalize(Ttx::Lexical::Cursor& cursor) -> Bool;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

 protected:
  // Concrete facts remain in the same lifetime domain as their Monograph so
  // graph edges never outlive their storage.
  Perimortem::Memory::Allocator::Arena& domain;

  // Parsed Documentation already belongs to the retained source Arena.
  const Ttx::Concept::Documentation& documentation;

  // The outer semantic context is borrowed for unresolved root queries. It is
  // neither transaction storage nor an Interpretation layer, and Monograph
  // never owns or mirrors its bindings.
  Ttx::Concept::Abstract& context;

 private:
  const Ttx::Concept::Abstract& language;
};

}  // namespace Tetrodotoxin::Language
