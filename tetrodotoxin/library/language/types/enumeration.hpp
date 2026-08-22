// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"
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

  static auto restore(
      Archive::Reader& reader,
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Concept::Abstract& host) -> Perimortem::Core::Option<Enumeration&>;

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

  constexpr auto get_declaration_anchor() const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor> override {
    return get_anchor();
  }

  TTX_NAME(definition.get_name());
  TTX_DOCUMENTATION(definition.get_documentation());

  auto link_types(Ttx::Lexical::Cursor& cursor) -> Bool override;
  auto finalize(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto link_restored_types() -> Bool override;

  auto finalize_restored() -> Bool override;

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_type_access(
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes route,
      Model::Type::Access access) const
      -> const Ttx::Concept::Abstract& override;

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto reserve(Llvm::Program& program) const -> Bool override;

  auto complete(Llvm::Program& program) const -> Bool override;

  auto persist(Archive::Writer& writer) const -> Bool override;

  auto accepts_iteration(const Ttx::Concept::Layout& bindings) const
      -> Bool override;

  auto begin_iteration(
      Llvm::Builder& body,
      const Ttx::Concept::Abstract& owner,
      const Ttx::Concept::Layout& bindings,
      const Ttx::Model::Pack& input) const -> Bool override;

  auto get_storage_type() const -> Perimortem::Core::Option<const Model::Type&>;

  auto get_cases() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Alias>>;

  constexpr auto get_case_count() const -> Count {
    return source_cases.get_size();
  }

  auto get_case_value(Count index) const -> Perimortem::Core::Option<U64>;

  auto get_case_name(Count index) const -> Perimortem::Core::View::Bytes;

  auto find_case_name(U64 value) const -> Perimortem::Core::View::Bytes;

 private:
  enum class Stage : ::U8 {
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
  Perimortem::Core::Option<Ttx::Concept::Reference<const Model::Addressable>>
      generated_size;
  Stage stage = Stage::Authored;
};

}  // namespace Tetrodotoxin::Library::Language::Types
