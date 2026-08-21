// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::App::Language {

// Runtime owns the selected application startup profile. The first App slice
// accepts only the Terminal profile and no profile settings.
class Runtime : public Ttx::Concept::Abstract {
 public:
  TTX_CONTRACT(Runtime, Ttx::Concept::Abstract);

  static auto parse(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation)
      -> Perimortem::Core::Option<Runtime&>;

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Documentation& documentation) -> Runtime&;

  TTX_NAME("Terminal"_view);
  TTX_DOCUMENTATION(documentation);

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

  auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Ttx::Concept::Abstract& override;

 private:
  constexpr Runtime(
      const Ttx::Concept::Documentation& documentation,
      Ttx::Lexical::Anchor anchor)
      : documentation(documentation), anchor(anchor) {}

  const Ttx::Concept::Documentation& documentation;
  Ttx::Lexical::Anchor anchor;
};

}  // namespace Tetrodotoxin::App::Language
