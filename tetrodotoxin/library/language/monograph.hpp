// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/managed/map.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"

namespace Tetrodotoxin::Library::Language {

// Monograph retains one Library source transaction and its intrinsic root
// vocabulary. The synthetic Source owns source grammar and graph completion.
class Monograph : public Tetrodotoxin::Language::Monograph {
 public:
  TTX_CONTRACT(Monograph, Tetrodotoxin::Language::Monograph);

  static auto create_authored(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      const Ttx::Concept::Abstract& language,
      Ttx::Concept::Abstract& context) -> Monograph&;

  auto parse(Ttx::Lexical::Cursor& cursor) -> Bool;

  auto link(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto lower(Llvm::Program& program) const
      -> Perimortem::Core::Option<Llvm::Program&>;

  auto get_name() const -> Perimortem::Core::View::Bytes override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_access(
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_call(
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_source() -> Types::Source& { return source; }

  constexpr auto get_source() const -> const Types::Source& { return source; }

 private:
  Monograph(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      const Ttx::Concept::Abstract& language,
      Ttx::Concept::Abstract& context);

  auto resolve_root_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Ttx::Concept::Abstract&>
          vocabulary;
  Types::Source& source;
};

}  // namespace Tetrodotoxin::Library::Language
