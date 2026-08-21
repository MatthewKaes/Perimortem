// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/app/language/monograph.hpp"
#include "tetrodotoxin/language/persistence/profile.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/documentation.hpp"

namespace Tetrodotoxin::App::Archive {

// Reader validates one App payload and reserves its real policy identities for
// the enclosing Workspace restoration barriers.
class Reader {
 public:
  constexpr Reader() = default;

  auto read(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes payload,
      Tetrodotoxin::Language::Persistence::Profile profile,
      const Ttx::Concept::Abstract& dialect,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context) const
      -> Perimortem::Core::Option<Tetrodotoxin::App::Language::Monograph&>;
};

}  // namespace Tetrodotoxin::App::Archive
