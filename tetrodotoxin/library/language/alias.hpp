// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/alias.hpp"

namespace Tetrodotoxin::Library::Language {

// Alias is one authored Library Type namespace redirection. Parsing reserves
// its stable identity and TypeReference. Link binds one exact target only after
// every surrounding Type identity exists. TTX Alias remains opaque, so no
// consumer can inspect or operate on the stored target edge directly.
class Alias : public Ttx::Model::Alias {
 private:
  constexpr Alias(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      TypeReference target_reference)
      : Ttx::Model::Alias(
            definition.get_name(),
            definition.get_documentation()),
        definition(definition),
        domain(domain),
        target_reference(target_reference) {}

 public:
  TTX_CONTRACT(Alias, Ttx::Model::Alias);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      TypeReference target_reference) -> Alias&;

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      TypeReference target_reference) -> Alias&;

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

  constexpr auto get_definition() const
      -> const Tetrodotoxin::Language::Definition& {
    return definition;
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor {
    return definition.get_anchor();
  }

  // Source orders Alias completion across its whole declaration tree. Alias
  // itself resolves its retained route and binds the resulting TTX Type. A
  // value consumer separately proves the narrower Library Type protocol.
  auto link() -> Bool;
  auto report_unresolved(Ttx::Lexical::Cursor& cursor) const -> void;

  constexpr auto get_target_reference() const -> const TypeReference& {
    return target_reference;
  }

  constexpr auto is_linked() const -> Bool { return linked; }

 private:
  Tetrodotoxin::Language::Definition& definition;
  Perimortem::Memory::Allocator::Arena& domain;
  TypeReference target_reference;
  Perimortem::Core::Option<const Ttx::Concept::Documentation&> documentation;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language
