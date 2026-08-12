// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/authored.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/alias.hpp"

namespace Tetrodotoxin::Library::Language {

// Alias is one authored Library Type-space redirection. Interpretation reserves
// its stable identity and TypeReference; link binds one exact target only after
// every surrounding Type identity exists. TTX Alias remains opaque, so no
// consumer can inspect or operate on the stored target edge directly.
class Alias : public Authored<Ttx::Model::Alias> {
  using Base = Authored<Ttx::Model::Alias>;

 private:
  constexpr Alias(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      TypeReference target_reference)
      : Base(definition, definition.get_name(), definition.get_documentation()),
        domain(domain),
        target_reference(target_reference) {}

 public:
  TTX_CONTRACT(Alias, Base, 0xeb6c21e679a3443f, 0x9f8f327b34eba32d);

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Alias&>;

  auto link_target(Tetrodotoxin::Language::Monograph& source) -> Bool;

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

 private:
  enum class Stage : Unsigned_8 {
    Unlinked,
    Linking,
    Linked,
  };

  Perimortem::Memory::Allocator::Arena& domain;
  TypeReference target_reference;
  Perimortem::Core::Option<const Ttx::Concept::Documentation&> documentation;
  Stage stage = Stage::Unlinked;
};

}  // namespace Tetrodotoxin::Library::Language
