// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/import.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language {

// A Monograph is the lasting semantic root created for one source transaction.
// It shares that Arena with source bytes, presentation facts, and every graph
// identity established by interpretation. Workspace retains the whole
// transaction whenever this root exists, including an incomplete edit that is
// useful to editor tooling.
//
// Linking settles edges after neighboring roots exist. Finalization admits the
// completed semantic island to Terminal production. A fixed child Monograph is
// appropriate only when the source directly authors meaning owned by that
// child language, as Shader does for its executable Library body.
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

  // Environment binds source-local Import Aliases to these real roots. The
  // root is a Dialect semantic object, never the Monograph lifetime owner.
  virtual auto get_root() const -> const Ttx::Concept::Abstract&;

  virtual auto retain_import(
      const Import::Description& description,
      Perimortem::Core::Option<Ttx::Lexical::Associations&> associations = {})
      -> Bool;

  virtual constexpr auto get_imports() const
      -> Perimortem::Core::View::Vector<Ttx::Concept::Reference<Import>> {
    return imports.get_view();
  }

  // Linking begins after every source in the transaction has established stable
  // identities. Finalization follows as a second barrier where each Monograph
  // can validate edges that may cross into another source.
  virtual auto compose(Ttx::Lexical::Cursor& cursor) -> Bool;
  virtual auto link(Ttx::Lexical::Cursor& cursor) -> Bool;
  virtual auto finalize(Ttx::Lexical::Cursor& cursor) -> Bool;

  // Restored graphs cross the same Package barriers even though they have no
  // source Cursor. A persistent Dialect reports rejection through process
  // Diagnostics while these operations preserve the authored transaction order.
  virtual auto compose_restored() -> Bool;
  virtual auto link_restored() -> Bool;
  virtual auto finalize_restored() -> Bool;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

 protected:
  // Concrete source roots can compose their owned lookup surface with common
  // imports without falling through to the outer context. This keeps a fixed
  // child layer from recursing through its owning Monograph.
  auto resolve_import(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

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
  Perimortem::Memory::Managed::Map<Perimortem::Core::View::Bytes, Import&>
      import_lookup;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Import>> imports;
};

}  // namespace Tetrodotoxin::Language
