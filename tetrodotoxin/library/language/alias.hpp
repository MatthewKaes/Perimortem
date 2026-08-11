// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/authored.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/alias.hpp"

namespace Tetrodotoxin::Library::Language {

// Alias is one authored Library redirection. The retained Definition owns its
// local source facts while the shared TTX Alias keeps the exact target edge.
class Alias : public Authored<Ttx::Model::Alias> {
  using Base = Authored<Ttx::Model::Alias>;

 private:
  constexpr Alias(
      Tetrodotoxin::Language::Definition& definition,
      const Ttx::Concept::Abstract& target,
      const Ttx::Concept::Documentation& documentation)
      : Base(definition, definition.get_name(), target, documentation) {}

 public:
  TTX_CONTRACT(Alias, Base, 0xeb6c21e679a3443f, 0x9f8f327b34eba32d);

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Alias&>;
};

}  // namespace Tetrodotoxin::Library::Language
