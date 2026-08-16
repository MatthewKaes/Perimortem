// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/alias.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Enumeration is one authored Library Type whose cases are named immutable
// values. It keeps source facts private until one exact integer storage Type
// and every Alias backed Constant are complete.
class Enumeration : public Model::Type {
 private:
  struct SourceCase {
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Bytes value;
    const Ttx::Concept::Documentation& documentation;
    Ttx::Lexical::Anchor anchor;
    Ttx::Lexical::Anchor name_anchor;
    Ttx::Lexical::Anchor value_anchor;
  };

  Enumeration(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      TypeReference storage_reference);

 public:
  TTX_CONTRACT(Enumeration, Model::Type);

  static auto interpret(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Enumeration&>;

  Enumeration(const Enumeration&) = delete;
  Enumeration(Enumeration&&) = delete;
  auto operator=(const Enumeration&) -> Enumeration& = delete;
  auto operator=(Enumeration&&) -> Enumeration& = delete;

  constexpr auto get_definition() const
      -> const Tetrodotoxin::Language::Definition& {
    return definition;
  }

  constexpr auto get_host() -> Ttx::Concept::Abstract& {
    return definition.get_host();
  }

  constexpr auto get_host() const -> const Ttx::Concept::Abstract& {
    return definition.get_host();
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor {
    return definition.get_anchor();
  }

  TTX_NAME(definition.get_name());
  TTX_DOCUMENTATION(definition.get_documentation());

  auto link_types(Ttx::Lexical::Cursor& cursor) -> Bool override;
  auto finalize(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto get_storage_type() const -> Perimortem::Core::Option<const Model::Type&>;

  auto get_cases() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Alias>>;

 private:
  enum class Stage : ::Unsigned_8 {
    Authored,
    StorageLinked,
    Finalized,
  };

  Tetrodotoxin::Language::Definition& definition;
  Perimortem::Memory::Allocator::Arena& domain;
  TypeReference storage_reference;
  Perimortem::Memory::Managed::Vector<SourceCase> source_cases;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Model::Type>>
      storage_type;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Alias>>
      cases;
  Stage stage = Stage::Authored;
};

}  // namespace Tetrodotoxin::Library::Language::Types
