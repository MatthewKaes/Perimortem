// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "tetrodotoxin/library/language/types/interface.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Implemented is one concrete Object whose generated and authored members
// satisfy an authored Interface. Interface defaults materialize as real Fields
// owned by this Object, while additional members keep their ordinary Object
// meaning.
class Implemented : public Object {
 public:
  TTX_CONTRACT(Implemented, Object);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Tetrodotoxin::Library::Language::TypeReference requirement)
      -> Implemented&;

  static auto create_restored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Tetrodotoxin::Library::Language::TypeReference requirement)
      -> Implemented&;

  auto bind_authored_requirement(Ttx::Lexical::Cursor& cursor) -> Bool;

  auto complete_body() -> void override;

  auto link_fields(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto link_initializers(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto link_restored_fields() -> Bool override;

  auto link_restored_initializers() -> Bool override;

  auto finalize_restored() -> Bool override;

  auto satisfies(const Ttx::Concept::Abstract& requirement) const
      -> Bool override;

  constexpr auto get_requirement_reference() const
      -> const Tetrodotoxin::Library::Language::TypeReference& {
    return requirement_reference;
  }

  constexpr auto get_requirement() const
      -> Perimortem::Core::Option<const Interface&> {
    return requirement.visit(
        []() -> Perimortem::Core::Option<const Interface&> { return {}; },
        [](const Interface* selected)
            -> Perimortem::Core::Option<const Interface&> {
          return *selected;
        });
  }

 private:
  class GeneratedField {
   public:
    constexpr GeneratedField(const Field& requirement, Field& implementation)
        : requirement(&requirement), implementation(&implementation) {}

    const Field* requirement;
    Field* implementation;
  };

  Implemented(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Tetrodotoxin::Library::Language::TypeReference requirement,
      Bool provides_initialization = True)
      : Object(domain, definition, provides_initialization),
        requirement_reference(requirement),
        generated_fields(domain) {}

  auto resolve_restored_requirement() -> Bool;
  auto materialize_fields() -> Bool;

  Tetrodotoxin::Library::Language::TypeReference requirement_reference;
  Perimortem::Core::Option<const Interface*> requirement;
  Perimortem::Memory::Managed::Vector<GeneratedField> generated_fields;
  Bool body_complete = False;
  Bool fields_materialized = False;
};

}  // namespace Tetrodotoxin::Library::Language::Types
