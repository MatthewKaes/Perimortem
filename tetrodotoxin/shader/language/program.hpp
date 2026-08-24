// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/type_reference.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/render/language/structure.hpp"
#include "tetrodotoxin/shader/language/binding.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Shader::Language {

// Program is the Library Composite authored inside one Shader definition. Its
// Functions, Fields, Types, expressions, and Flow owners are the same semantic
// objects used by Library source. Program adds only the selected Render
// contract and Shader binding relationships needed to validate GPU use.
class Program : public Tetrodotoxin::Library::Language::Types::Structure {
 public:
  TTX_CONTRACT(Program, Tetrodotoxin::Library::Language::Types::Structure);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Tetrodotoxin::Language::TypeReference contract,
      Ttx::Concept::Abstract& context) -> Program&;

  auto retain_shader_binding(
      Tetrodotoxin::Library::Language::Field& field,
      Tetrodotoxin::Render::Language::Binding::Kind kind) -> void;

  auto validate_contract(Ttx::Lexical::Cursor& cursor) -> Bool;

  constexpr auto get_contract() const -> Perimortem::Core::Option<
      const Tetrodotoxin::Render::Language::Structure&> {
    return contract_type.visit(
        []() -> Perimortem::Core::Option<
                 const Tetrodotoxin::Render::Language::Structure&> {
          return {};
        },
        [](const Ttx::Concept::Reference<
            const Tetrodotoxin::Render::Language::Structure>& selected)
            -> Perimortem::Core::Option<
                const Tetrodotoxin::Render::Language::Structure&> {
          return selected.get();
        });
  }

  constexpr auto get_bindings() const
      -> Perimortem::Core::View::Vector<Binding> {
    return bindings;
  }

 private:
  Program(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Tetrodotoxin::Language::TypeReference contract,
      Ttx::Concept::Abstract& context)
      : Tetrodotoxin::Library::Language::Types::Structure(
            domain,
            definition,
            False),
        contract(contract),
        context(context),
        bindings(domain) {}

  Tetrodotoxin::Language::TypeReference contract;
  Ttx::Concept::Abstract& context;
  Perimortem::Memory::Managed::Vector<Binding> bindings;
  Perimortem::Core::Option<
      Ttx::Concept::Reference<const Tetrodotoxin::Render::Language::Structure>>
      contract_type;
};

}  // namespace Tetrodotoxin::Shader::Language
