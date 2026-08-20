// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/language/persistence/profile.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language {

// One Dialect is a stateless language protocol installed in an Environment
// Toolchain. Its immutable name and downward dependency edges are shared by
// every Workspace borrowing that Toolchain. A source Cursor exposes the
// transaction Arena used by every identity produced while reading that source.
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

  // Reads the one shared source envelope, selects an exact installed Dialect,
  // and returns that Dialect's sole parse result.
  static auto interpret_source(
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Dialect>>
          installed,
      Ttx::Lexical::Cursor& cursor,
      Ttx::Concept::Abstract& context) -> Perimortem::Core::Option<Monograph&>;

  // Encode only the durable facts owned by this Dialect. An engaged empty byte
  // value is a successful empty payload while no value reports unsupported or
  // failed encoding.
  virtual auto encode(
      const Ttx::Concept::Abstract& monograph,
      Persistence::Profile profile) const
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>;

  // Restore one opaque payload using the same transaction context as authored
  // interpretation. The explicit payload is reconstruction input rather than
  // a second contextual wrapper.
  virtual auto restore(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes payload,
      Persistence::Profile profile,
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
