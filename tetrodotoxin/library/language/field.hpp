// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "tetrodotoxin/library/language/writability.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language {

// Field is the exact Addressable binding retained by a Library Composite. Its
// authored Visibility decides readable lookup while Writability records its
// Static, state, or compile time evaluation policy without widening the shared
// TTX Addressable contract. An initializer remains its real Pack: declared
// Fields receive the complete flow through Layout fitting, while inference
// accepts only one scalar output and retains that exact Type.
class Field : public Model::Addressable {
 private:
  constexpr Field(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Writability writability,
      Perimortem::Core::Option<TypeReference> type_reference,
      Perimortem::Core::Option<Model::Pack&> initializer)
      : definition(definition),
        domain(domain),
        writability(writability),
        type_reference(type_reference),
        initializer(initializer),
        initializer_linked(!initializer) {}

 public:
  TTX_CONTRACT(Field, Model::Addressable);

  static auto interpret(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Field&>;

  static auto restore(
      Archive::Reader& reader,
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Concept::Abstract& host) -> Perimortem::Core::Option<Field&>;

  static auto restore_slot(
      Archive::Reader& reader,
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Concept::Abstract& host,
      Count ordinal) -> Perimortem::Core::Option<Field&>;

  auto link_declaration_type(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto link_inferred_declaration_type(Ttx::Lexical::Cursor& cursor)
      -> Bool override;

  Field(const Field&) = delete;
  Field(Field&&) = delete;
  auto operator=(const Field&) -> Field& = delete;
  auto operator=(Field&&) -> Field& = delete;

  auto link_declaration_initializer(Ttx::Lexical::Cursor& cursor)
      -> Bool override;

  auto link_restored_declaration_type() -> Bool override;

  auto link_restored_declaration_initializer() -> Bool override;

  // Const completion is a required link barrier. The initializer must reduce
  // to one exact constant Pack before any body can consume this Field.
  auto link_declaration_constant(Ttx::Lexical::Cursor& cursor) const
      -> Bool override;

  // Finalization visits the real initializer Pack after linking has frozen
  // its output Layout. Field remains the declaration owner. No Expression
  // side inventory is required merely to cache constant producers.
  auto finalize_declaration(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto reserve_declaration(Llvm::Program& program) const -> Bool override;

  auto complete_declaration(Llvm::Program& program) const -> Bool override;

  auto lower_declaration(Llvm::Program& program) const -> Bool override;

  auto persist(Archive::Writer& writer) const -> Bool override;

  auto persist_slot(Archive::Writer& writer, Count ordinal) const -> Bool;

  constexpr auto contributes_to_instance_layout() const -> Bool override {
    return writability == Writability::Internal;
  }

  constexpr auto supports_access(Model::Type::Access access) const
      -> Bool override {
    if (writability == Writability::Constant) {
      return True;
    }
    return Bool(
        (access == Model::Type::Access::Static &&
         writability == Writability::Full) ||
        (access == Model::Type::Access::Self &&
         writability == Writability::Internal));
  }

  constexpr auto permits_write_from(const Model::Type& access_scope) const
      -> Bool override {
    switch (writability) {
    case Writability::Full:
      return True;
    case Writability::Internal:
      return Bool(
          get_definition().get_visibility() ==
              Tetrodotoxin::Language::Visibility::Public ||
          access_scope.has_private_access_to(get_host()));
    case Writability::Constant:
      return False;
    }
    return False;
  }

  TTX_DOCUMENTATION(get_definition().get_documentation());
  TTX_NAME(definition.get_name());

  constexpr auto get_definition() const
      -> const Tetrodotoxin::Language::Definition& {
    return definition;
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor {
    return definition.get_anchor();
  }

  constexpr auto get_declaration_anchor() const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor> override {
    return get_anchor();
  }

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  constexpr auto get_type() const -> const Model::Type& override {
    return type->get();
  }

  auto get_type_reference() const
      -> Perimortem::Core::Option<const TypeReference&> {
    return type_reference.visit(
        []() -> Perimortem::Core::Option<const TypeReference&> { return {}; },
        [](const TypeReference& selected)
            -> Perimortem::Core::Option<const TypeReference&> {
          return selected;
        });
  }

  constexpr auto get_writability() const -> Writability { return writability; }

  auto get_type_anchor() const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor> {
    return type_reference.visit(
        []() -> Perimortem::Core::Option<Ttx::Lexical::Anchor> { return {}; },
        [](const TypeReference& selected)
            -> Perimortem::Core::Option<Ttx::Lexical::Anchor> {
          return selected.get_anchor();
        });
  }

  constexpr auto get_host() const -> const Model::Type& {
    return static_cast<const Model::Type&>(get_definition().get_host());
  }

  auto get_initializer() const -> Perimortem::Core::Option<const Model::Pack&>;

  // Const Fields are declaration owned compile time values. They never denote
  // per instance storage, regardless of which valid receiver selects them.
  auto get_constant() const -> Perimortem::Core::Option<Model::Pack&> override;

  constexpr auto is_linked() const -> Bool {
    return Bool(type) && initializer_linked;
  }

 private:
  enum class ConstantState : U8 {
    Unresolved,
    Folding,
    Folded,
    Failed,
  };

  auto cache_constant() const -> Bool;

  auto validate_publication(Ttx::Lexical::Cursor& cursor) const -> Bool;

  Tetrodotoxin::Language::Definition& definition;
  Perimortem::Memory::Allocator::Arena& domain;
  Writability writability;
  Perimortem::Core::Option<TypeReference> type_reference;
  Perimortem::Core::Option<Model::Pack&> initializer;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Model::Type>> type;
  mutable Perimortem::Core::Option<Ttx::Concept::Reference<Model::Pack>>
      constant;
  mutable ConstantState constant_state = ConstantState::Unresolved;
  Bool initializer_linked;
};

}  // namespace Tetrodotoxin::Library::Language
