// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"

namespace Tetrodotoxin::Scene::Language {

// Monograph is the one retained Scene root. Its fixed Library child keeps the
// real CPU facts while this outer owner controls phase order and publication.
class Monograph : public Tetrodotoxin::Language::Monograph {
 public:
  TTX_CONTRACT(Monograph, Tetrodotoxin::Language::Monograph);

  Monograph(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Concept::Abstract& language,
      Ttx::Concept::Abstract& context,
      Tetrodotoxin::Library::Language::Monograph& library);

  constexpr auto get_library() -> Tetrodotoxin::Library::Language::Monograph& {
    return library;
  }

  constexpr auto get_library() const
      -> const Tetrodotoxin::Library::Language::Monograph& {
    return library;
  }

  auto get_layer(const Ttx::Concept::Abstract& requested) const
      -> Perimortem::Core::Option<
          const Tetrodotoxin::Language::Monograph&> override;

  auto link(Ttx::Lexical::Cursor& cursor) -> Bool override;
  auto finalize(Ttx::Lexical::Cursor& cursor) -> Bool override;

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

 private:
  Tetrodotoxin::Library::Language::Monograph& library;
};

}  // namespace Tetrodotoxin::Scene::Language
