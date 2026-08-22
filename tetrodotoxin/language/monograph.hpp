// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language {

// A Monograph is the lasting semantic result of one source. It shares the
// source transaction Arena with every identity created during interpretation,
// and Workspace retains that complete lifetime after the source succeeds.
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

  // Some Dialects build one fixed child language layer into their result. Exact
  // installed Dialect identity selects that child, which keeps the relationship
  // consistent with the Toolchain that interpreted the source.
  virtual auto get_layer(const Ttx::Concept::Abstract& requested) const
      -> Perimortem::Core::Option<const Monograph&>;

  // Linking begins after every source in the transaction has established stable
  // identities. Finalization follows as a second barrier where each Monograph
  // can validate edges that may cross into another source.
  virtual auto link(Ttx::Lexical::Cursor& cursor) -> Bool;
  virtual auto finalize(Ttx::Lexical::Cursor& cursor) -> Bool;

  // Restored graphs cross the same Package barriers even though they have no
  // source Cursor. A persistent Dialect reports rejection through process
  // Diagnostics while these operations preserve the authored transaction order.
  virtual auto link_restored() -> Bool;
  virtual auto finalize_restored() -> Bool;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

 protected:
  // Keeping concrete facts in the Monograph's lifetime domain lets graph edges
  // remain valid for as long as Workspace exposes the source result.
  Perimortem::Memory::Allocator::Arena& domain;

  // Parsed Documentation shares the retained source Arena.
  const Ttx::Concept::Documentation& documentation;

  // The outer semantic context answers unresolved root queries. Borrowing it
  // keeps those bindings with their real owner while the Monograph retains only
  // its own source graph.
  Ttx::Concept::Abstract& context;

 private:
  const Ttx::Concept::Abstract& language;
};

}  // namespace Tetrodotoxin::Language
