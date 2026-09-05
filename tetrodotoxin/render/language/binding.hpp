// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/type_reference.hpp"
#include "ttx/concept/unknown.hpp"
#include "ttx/ffi/cpp/addressable.hpp"

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

  enum class Access : U8 {
    None,
    Read,
    Write,
    ReadWrite,
  };


  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Kind kind,
      Tetrodotoxin::Language::TypeReference type,
      Access access = Access::None) -> Binding&;

  static auto create_slot(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Domain& value_domain) -> Binding&;

  static auto create_restored_slot(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Language::TypeReference type) -> Binding&;

  auto link(Ttx::Lexical::Cursor& cursor, const Ttx::Concept::Abstract& context)
      -> Bool;

  auto link_restored(const Ttx::Concept::Abstract& context) -> Bool;

  TTX_NAME(name);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

  auto get_domain() const -> const Ttx::Concept::Abstract& override;

  constexpr auto is_linked() const -> Bool { return Bool(value_domain); }

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  constexpr auto get_kind() const -> Kind { return kind; }

  constexpr auto get_access() const -> Access { return access; }

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
      Access access,
      Perimortem::Core::Option<Tetrodotoxin::Language::TypeReference>
          type_reference,
      Perimortem::Core::Option<const Ttx::Model::Domain*> value_domain)
      : name(name),
        definition(definition),
        kind(kind),
        access(access),
        type_reference(type_reference),
        value_domain(value_domain) {}

  Perimortem::Core::View::Bytes name;
  Perimortem::Core::Option<Tetrodotoxin::Language::Definition&> definition;
  Kind kind;
  Access access;
  Perimortem::Core::Option<Tetrodotoxin::Language::TypeReference>
      type_reference;
  Perimortem::Core::Option<const Ttx::Model::Domain*> value_domain;
};

}  // namespace Tetrodotoxin::Render::Language
