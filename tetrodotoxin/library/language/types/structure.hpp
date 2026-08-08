// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/visibility.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/layouts/structured.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Structure owns Library static and instance lookup, real Field edges, hosted
// Callables, and one Structured instance Layout. Synthetic source and Object
// specialize this same anatomy instead of reproducing it.
class Structure : public Ttx::Model::Type {
 protected:
  Structure(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Documentation& documentation,
      Visibility visibility,
      Monograph& source,
      Materializations& materializations,
      Perimortem::Utility::Option<
          Ttx::Concept::Reference<const Ttx::Model::Type>> source_scope,
      Perimortem::Utility::Option<Ttx::Lexical::Anchor> anchor,
      Perimortem::Utility::Option<Ttx::Lexical::Anchor> name_anchor);

  auto complete_declaration(Ttx::Lexical::Anchor complete_anchor) -> void;

 public:
  using ClassCatagory = Structure;
  static constexpr Perimortem::System::Uuid contract_id{
    0xe3773c0325224200,
    0xaeb9a3131139c16f,
  };

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Monograph& source,
      Materializations& materializations) -> Structure&;

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      Monograph& source,
      Materializations& materializations)
      -> Perimortem::Utility::Option<Structure&>;

  Structure(const Structure&) = delete;
  Structure(Structure&&) = delete;
  auto operator=(const Structure&) -> Structure& = delete;
  auto operator=(Structure&&) -> Structure& = delete;

  // Static bindings never enter the instance Layout. Only the synthetic source
  // admits this mutation while its authored binding phase remains open.
  auto bind_static(
      Ttx::Concept::Abstract& binding,
      Visibility binding_visibility) -> Bool;

  auto can_bind_static(Perimortem::Core::View::Bytes name) const -> Bool;

  auto link_fields() -> Bool;
  auto link_initializers() -> Bool;
  auto link_callable_signatures() -> Bool;
  auto link_callable_bodies() -> Bool;
  auto finalize() -> Bool;

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Ttx::Model::Type::implements(requested);
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {
    return name;
  }

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  // Contextless lookup exposes only externally readable identities. A hosted
  // Callable receives the complete view after exact identity authentication.
  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_context(
      Perimortem::Core::View::Bytes route,
      const Ttx::Model::Callable& requester) const
      -> const Ttx::Concept::Abstract&;

  auto resolve_context(
      Perimortem::Core::View::Bytes route,
      const Field& requester) const -> const Ttx::Concept::Abstract&;

  // The exact source Monograph uses the same complete lookup during semantic
  // link barriers that run before a hosted Callable can ask for context.
  auto resolve_context(
      Perimortem::Core::View::Bytes route,
      const Monograph& requester) const -> const Ttx::Concept::Abstract&;

  auto get_layout() const -> const Ttx::Concept::Layout& override;

  constexpr auto get_visibility() const -> Visibility { return visibility; }

  constexpr auto get_anchor() const
      -> const Perimortem::Utility::Option<Ttx::Lexical::Anchor>& {
    return anchor;
  }

  constexpr auto get_name_anchor() const
      -> const Perimortem::Utility::Option<Ttx::Lexical::Anchor>& {
    return name_anchor;
  }

  constexpr auto is_source() const -> Bool { return !source_scope; }

  auto get_fields() const
      -> Perimortem::Core::View::Vector<Ttx::Concept::Reference<const Field>>;

  auto get_public_fields() const
      -> Perimortem::Core::View::Vector<Ttx::Concept::Reference<const Field>>;

  auto get_field_sources() const
      -> Perimortem::Core::View::Vector<Field::Source>;

  auto get_callables() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Callable>>;

  auto get_public_callables() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Callable>>;

  auto get_static_bindings() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>;

  auto get_external_static_bindings() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>;

  constexpr auto is_linked() const -> Bool {
    return stage >= Stage::FieldsLinked;
  }

  constexpr auto is_finalized() const -> Bool {
    return stage == Stage::Finalized;
  }

 private:
  enum class Stage : Unsigned_8 {
    Authored,
    FieldsLinked,
    InitializersLinked,
    CallableSignaturesLinked,
    CallablesLinked,
    Finalized,
  };

  auto bind_callable(
      Ttx::Model::Callable& callable,
      Visibility callable_visibility) -> Bool;

  auto bind(Ttx::Concept::Abstract& binding, Visibility binding_visibility)
      -> Bool;

  auto can_bind(Perimortem::Core::View::Bytes name) const -> Bool;

  auto owns(const Ttx::Model::Callable& requester) const -> Bool;

  auto owns(const Field& requester) const -> Bool;

  auto get_external_source_context() const -> const Ttx::Concept::Abstract&;

  auto resolve_external_static_context(Perimortem::Core::View::Bytes route)
      const -> const Ttx::Concept::Abstract&;

  auto resolve_external_member_context(Perimortem::Core::View::Bytes route)
      const -> const Ttx::Concept::Abstract&;

  auto resolve_internal_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  Perimortem::Memory::Allocator::Arena& domain;
  Perimortem::Core::View::Bytes name;
  const Ttx::Concept::Documentation& documentation;
  Visibility visibility;
  Monograph& source;
  Materializations& materializations;
  Perimortem::Utility::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      source_scope;
  Perimortem::Utility::Option<Ttx::Lexical::Anchor> anchor;
  Perimortem::Utility::Option<Ttx::Lexical::Anchor> name_anchor;
  Perimortem::Memory::Managed::Vector<Field::Source> field_sources;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Field>> fields;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<const Field>>
      field_observations;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<const Field>>
      public_fields;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Addressable>>
      layout_fields;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Model::Callable>>
      callables;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Callable>>
      callable_observations;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Callable>>
      public_callables;
  Perimortem::Memory::Managed::Map<
      Perimortem::Core::View::Bytes,
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      static_bindings;
  Perimortem::Memory::Managed::Map<
      Perimortem::Core::View::Bytes,
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      external_static_bindings;
  Perimortem::Memory::Managed::Map<
      Perimortem::Core::View::Bytes,
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      member_bindings;
  Perimortem::Memory::Managed::Map<
      Perimortem::Core::View::Bytes,
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      external_member_bindings;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      static_binding_order;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      external_static_binding_order;
  Perimortem::Utility::Option<const Ttx::Model::Layouts::Structured&> layout;
  Stage stage = Stage::Authored;
};

}  // namespace Tetrodotoxin::Library::Language::Types
