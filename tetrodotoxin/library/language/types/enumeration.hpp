// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/library/language/model/completion.hpp"
#include "tetrodotoxin/library/language/model/initialization.hpp"
#include "tetrodotoxin/library/language/model/iteration.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "tetrodotoxin/language/binding.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Enumeration is one authored Library Type whose cases are named immutable
// values. It keeps the authored cases private until one integer storage Type
// and every Alias backed Constant are complete.
class Enumeration : public Model::Type, public Model::Completion {
 public:
  // Case retains the authored spelling and Documentation until the storage
  // relationship settles and can create the immutable value. Keeping that
  // input with Enumeration avoids a parallel parser record whose lifetime
  // would have to be synchronized with the declaration.
  struct Case {
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Bytes value;
    const Ttx::Concept::Documentation& documentation;
    Ttx::Lexical::Anchor anchor;
    Ttx::Lexical::Anchor name_anchor;
    Ttx::Lexical::Anchor value_anchor;
  };

 private:
  Enumeration(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      TypeReference storage_reference);

 public:

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      TypeReference storage_reference,
      Perimortem::Core::View::Vector<Case> cases) -> Enumeration&;

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      TypeReference storage_reference,
      Perimortem::Core::View::Vector<Case> cases) -> Enumeration&;

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
    return definition.get_declaration_anchor();
  }

  TTX_NAME(definition.get_name());
  TTX_DOCUMENTATION(definition.get_documentation());

  auto link_types(Ttx::Lexical::Cursor& cursor) -> Bool override;
  auto finalize(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto link_restored_types() -> Bool override;

  auto finalize_restored() -> Bool override;

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;
  void visit_concepts(ttx_named_abstract_callable* visitor) const override;

  auto initialize_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&>;

  auto accepts_binding(const Ttx::Concept::Layout& bindings) const -> Bool;

  auto get_storage_type() const -> Perimortem::Core::Option<const Model::Type&>;

  constexpr auto get_storage_reference() const -> const TypeReference& {
    return storage_reference;
  }

  auto get_cases() const
      -> Perimortem::Core::View::Vector<const Tetrodotoxin::Language::Binding*>;

  constexpr auto get_case_count() const -> Count {
    return source_cases.get_size();
  }

  auto get_case_value(Count index) const -> Perimortem::Core::Option<U64>;

  auto get_case_name(Count index) const -> Perimortem::Core::View::Bytes;

  auto retain_restored_case(
      Perimortem::Core::View::Bytes name,
      U64 value,
      const Ttx::Concept::Documentation& documentation) -> Bool;

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
  Perimortem::Memory::Managed::Vector<Case> source_cases;
  Perimortem::Core::Option<const Model::Type*> storage_type;
  Perimortem::Memory::Managed::Vector<const Tetrodotoxin::Language::Binding*> cases;
  Perimortem::Core::Option<const Model::Addressable*> generated_size;
  class Iteration final : public Model::Iteration {
   public:
    explicit Iteration(const Enumeration& owner) : owner(owner) {}

    auto accepts_binding(const Ttx::Concept::Layout& bindings) const
        -> Bool override {
      return owner.accepts_binding(bindings);
    }

   private:
    const Enumeration& owner;
  };
  Iteration iteration;
  Model::OwnedInitialization<Enumeration> initialization;
  Stage stage = Stage::Authored;
};

}  // namespace Tetrodotoxin::Library::Language::Types
