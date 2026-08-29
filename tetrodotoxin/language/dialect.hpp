// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language {

// A Dialect is one language installed in a Tetrodotoxin Toolchain. It owns the
// grammar and semantic construction for that language while remaining reusable
// across every Workspace that borrows the Toolchain. Each source Cursor lends
// the Arena where that interpretation creates its Monograph and semantic
// identities.
//
// Cursor supplies an already classified Code stream. The frontend that produces
// that stream owns tokenization, preprocessing, and macro policy, so a Dialect
// can focus on the meaning of the source form it recognizes.
class Dialect : public Ttx::Concept::Abstract {
 public:
  TTX_CONTRACT(Dialect, Ttx::Concept::Abstract);

  Dialect(Perimortem::Core::View::Bytes name);
  virtual ~Dialect() = 0;

  virtual auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Monograph&> = 0;

  static auto find_installed(
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Dialect>>
          installed,
      Perimortem::Core::View::Bytes name) -> Perimortem::Core::Option<Dialect&>;

  // Every Tetrodotoxin source begins with the same documentation and Dialect
  // envelope. Reading it here gives the selected language one consistent entry
  // point and one Monograph result.
  static auto interpret_source(
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Dialect>>
          installed,
      Ttx::Lexical::Cursor& cursor,
      Ttx::Concept::Abstract& context) -> Perimortem::Core::Option<Monograph&>;

  // A persistent Dialect retains the complete facts needed to rebuild its own
  // Monograph. An engaged empty value is a valid payload.
  virtual auto encode(const Ttx::Concept::Abstract& monograph) const
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>;

  // Restoration receives the same Arena, Documentation, and outer context as
  // authored interpretation. The payload replaces source reading while the
  // language keeps its ordinary construction and completion rules.
  virtual auto restore(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes payload,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context) -> Perimortem::Core::Option<Monograph&>;

  // A completed source asks its selected Dialect for the default external
  // product. The graph is supplied only through Abstract concepts, keeping
  // Puffer and concrete Dialects independent from Workspace representation.
  virtual auto produce(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& graph,
      const Monograph& monograph) const
      -> Perimortem::Core::Option<const Ttx::Concept::Pack&>;

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return Ttx::Concept::Documentation::get_empty();
  }

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

 private:
  Perimortem::Core::View::Bytes name;
};

}  // namespace Tetrodotoxin::Language
