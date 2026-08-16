// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language {

// One Dialect is the installed Abstract language context for a Workspace. It
// owns only Workspace lifetime language state and downward dependency edges. A
// source Cursor exposes the transaction Arena used by every identity produced
// while reading that source.
class Dialect : public Ttx::Concept::Abstract {
 public:
  Dialect(Perimortem::Core::View::Bytes name);
  virtual ~Dialect() = 0;

  TTX_CONTRACT(Dialect, Ttx::Concept::Abstract);

  virtual auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Monograph&> = 0;

  static auto find_installed(
      Perimortem::Core::View::Vector<Dialect*> installed,
      Perimortem::Core::View::Bytes name) -> Perimortem::Core::Option<Dialect&>;

  // Reads the one shared source envelope, selects an exact installed Dialect,
  // and returns that Dialect's sole parse result.
  static auto interpret_source(
      Perimortem::Core::View::Vector<Dialect*> installed,
      Ttx::Lexical::Cursor& cursor,
      Ttx::Concept::Abstract& context) -> Perimortem::Core::Option<Monograph&>;

  // Encode only the durable facts owned by this Dialect. An engaged empty byte
  // value is a successful empty payload while no value reports unsupported or
  // failed encoding.
  virtual auto encode(const Ttx::Concept::Abstract& monograph) const
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>;

  // Restore one opaque payload using the same transaction context as authored
  // interpretation. The explicit payload is reconstruction input rather than
  // a second contextual wrapper.
  virtual auto restore(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes payload,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context) -> Perimortem::Core::Option<Monograph&>;

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return Ttx::Concept::Documentation::get_empty();
  }

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

 private:
  Perimortem::Core::View::Bytes name;
};

}  // namespace Tetrodotoxin::Language
