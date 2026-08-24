// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/type_reference.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/addressable.hpp"

namespace Tetrodotoxin::Render::Language {

// Binding names one value required by a Render contract. It carries interface
// meaning only. Executable initialization and mutation belong to the language
// that supplies the value.
class Binding : public Ttx::Model::Addressable {
 public:
  enum class Kind : U8 {
    Value,
    Constant,
    Push,
    Resource,
    Parameter,
  };

  TTX_CONTRACT(Binding, Ttx::Model::Addressable);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Kind kind,
      Tetrodotoxin::Language::TypeReference type) -> Binding&;

  static auto create_slot(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& type) -> Binding&;

  static auto create_restored_slot(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Language::TypeReference type) -> Binding&;

  auto link(Ttx::Lexical::Cursor& cursor, const Ttx::Concept::Abstract& context)
      -> Bool;

  auto link_restored(const Ttx::Concept::Abstract& context) -> Bool;

  TTX_NAME(name);
  TTX_INVALID_CONTEXT;

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

  auto get_type() const -> const Ttx::Model::Type& override;

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  constexpr auto get_kind() const -> Kind { return kind; }

  constexpr auto get_type_reference() const -> Perimortem::Core::Option<
      const Tetrodotoxin::Language::TypeReference&> {
    return type_reference.visit(
        []() -> Perimortem::Core::Option<
                 const Tetrodotoxin::Language::TypeReference&> { return {}; },
        [](const Tetrodotoxin::Language::TypeReference& selected)
            -> Perimortem::Core::Option<
                const Tetrodotoxin::Language::TypeReference&> {
          return selected;
        });
  }

  constexpr auto get_definition() const
      -> Perimortem::Core::Option<const Tetrodotoxin::Language::Definition&> {
    return definition.visit(
        []() -> Perimortem::Core::Option<
                 const Tetrodotoxin::Language::Definition&> { return {}; },
        [](const Tetrodotoxin::Language::Definition& selected)
            -> Perimortem::Core::Option<
                const Tetrodotoxin::Language::Definition&> {
          return selected;
        });
  }

 private:
  Binding(
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::Option<Tetrodotoxin::Language::Definition&> definition,
      Kind kind,
      Perimortem::Core::Option<Tetrodotoxin::Language::TypeReference>
          type_reference,
      Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
          type)
      : name(name),
        definition(definition),
        kind(kind),
        type_reference(type_reference),
        type(type) {}

  Perimortem::Core::View::Bytes name;
  Perimortem::Core::Option<Tetrodotoxin::Language::Definition&> definition;
  Kind kind;
  Perimortem::Core::Option<Tetrodotoxin::Language::TypeReference>
      type_reference;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      type;
};

}  // namespace Tetrodotoxin::Render::Language
