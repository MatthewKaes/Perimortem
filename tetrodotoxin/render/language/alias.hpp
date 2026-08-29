// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/type_reference.hpp"
#include "ttx/bootstrap/model/alias.hpp"

namespace Tetrodotoxin::Render::Language {

class Alias : public Ttx::Model::Alias {
 public:
  TTX_CONTRACT(Alias, Ttx::Model::Alias);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Tetrodotoxin::Language::TypeReference target) -> Alias&;

  auto link(Ttx::Lexical::Cursor& cursor, const Ttx::Concept::Abstract& context)
      -> Bool;

  auto link_restored(const Ttx::Concept::Abstract& context) -> Bool;

  constexpr auto get_definition() const
      -> const Tetrodotoxin::Language::Definition& {
    return definition;
  }

  constexpr auto get_target_reference() const
      -> const Tetrodotoxin::Language::TypeReference& {
    return target;
  }

 private:
  Alias(
      Tetrodotoxin::Language::Definition& definition,
      Tetrodotoxin::Language::TypeReference target)
      : Ttx::Model::Alias(
            definition.get_name(),
            definition.get_documentation()),
        definition(definition),
        target(target) {}

  Tetrodotoxin::Language::Definition& definition;
  Tetrodotoxin::Language::TypeReference target;
};

}  // namespace Tetrodotoxin::Render::Language
