// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/language/authored.hpp"
#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "tetrodotoxin/library/language/writability.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/addressable.hpp"

namespace Tetrodotoxin::Library::Language {

// Field is the exact Addressable binding retained by a Library Composite. Its
// authored Visibility decides readable lookup while Writability records who
// may mutate the reached value without widening the shared TTX Addressable
// contract. An initializer remains its real Pack: declared Fields receive the
// complete flow through Layout fitting, while inference accepts only one
// scalar output and retains that exact Type.
class Field : public Authored<Ttx::Model::Addressable> {
  using Base = Authored<Ttx::Model::Addressable>;

 private:
  constexpr Field(
      Tetrodotoxin::Language::Definition& definition,
      Writability writability,
      Perimortem::Core::Option<TypeReference> type_reference,
      Perimortem::Core::Option<Model::Pack&> initializer)
      : Base(definition),
        writability(writability),
        type_reference(type_reference),
        initializer(initializer),
        initializer_linked(!initializer) {}

 public:
  TTX_CONTRACT(Field, Base, 0xc6fc7cb2676b4bac, 0xa205fab02169b1ca);

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Field&>;

  auto link_type(Tetrodotoxin::Language::Monograph& source) -> Bool;

  Field(const Field&) = delete;
  Field(Field&&) = delete;
  auto operator=(const Field&) -> Field& = delete;
  auto operator=(Field&&) -> Field& = delete;

  auto link_initializer(Tetrodotoxin::Language::Monograph& source) -> Bool;

  // Const completion is a required link barrier. The initializer must reduce
  // to one exact constant Pack before any body can consume this Field.
  auto link_constant(Tetrodotoxin::Language::Monograph& source) const -> Bool;

  auto validate_publication(Tetrodotoxin::Language::Monograph& source) const
      -> Bool;

  // Finalization visits the real initializer Pack after linking has frozen
  // its output Layout. Field remains the declaration owner; no Expression
  // side inventory is required merely to cache constant producers.
  auto finalize() -> void;

  TTX_DOCUMENTATION(get_definition().get_documentation());

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  constexpr auto get_type() const -> const Ttx::Model::Type& override {
    return type->get();
  }

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

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

  constexpr auto is_inferred() const -> Bool { return !type_reference; }

  constexpr auto get_host() const -> const Ttx::Model::Type& {
    return static_cast<const Ttx::Model::Type&>(get_definition().get_host());
  }

  auto get_initializer() const -> Perimortem::Core::Option<const Model::Pack&>;

  // Const Fields are declaration-owned compile-time values. They never denote
  // per-instance storage, regardless of which valid receiver selects them.
  auto get_constant() const -> Perimortem::Core::Option<Model::Pack&>;

  constexpr auto is_linked() const -> Bool {
    return Bool(type) && initializer_linked;
  }

 private:
  enum class ConstantState : Unsigned_8 {
    Unresolved,
    Folding,
    Folded,
    Failed,
  };

  auto cache_constant() const -> Bool;

  Writability writability;
  Perimortem::Core::Option<TypeReference> type_reference;
  Perimortem::Core::Option<Model::Pack&> initializer;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      type;
  mutable Perimortem::Core::Option<Ttx::Concept::Reference<Model::Pack>>
      constant;
  mutable ConstantState constant_state = ConstantState::Unresolved;
  Bool initializer_linked;
};

}  // namespace Tetrodotoxin::Library::Language
