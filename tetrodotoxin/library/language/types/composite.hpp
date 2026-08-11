// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/visibility.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Composite owns the member inventories, lookup categories, Layout, and
// completion lifecycle shared by Source, Structure, and Object. Each concrete
// Type supplies its own presentation and authored semantics.
class Composite : public Ttx::Model::Type {
 protected:
  Composite(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Materializations& materializations,
      Perimortem::Core::Option<const Composite&> enclosing_scope = {});

  auto interpret_definition(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition) -> Bool;

  auto can_accept_definition() const -> Bool;

  auto can_bind_definition(const Ttx::Concept::Abstract& binding) const -> Bool;

  auto publish_binding(
      Ttx::Concept::Abstract& binding,
      Visibility binding_visibility) -> void;

  auto retain_definition_field(Field& field) -> Bool;

  virtual auto can_retain_binding(const Ttx::Concept::Abstract& binding) const
      -> Bool;

  virtual auto retain_binding(
      Ttx::Concept::Abstract& binding,
      Visibility binding_visibility) -> Bool;

  auto retain_authored_field(Field& field) -> Bool;

  virtual auto publish_linked_field(Field& field) -> void;

  virtual auto complete_field_layout() -> void;

  virtual auto validate_linked_callable(const Ttx::Model::Callable& callable)
      -> Bool;

  virtual auto grants_complete_access(
      const Ttx::Concept::Abstract& requester) const -> Bool;

  virtual auto resolve_internal_context(Perimortem::Core::View::Bytes route)
      const -> const Ttx::Concept::Abstract&;

  virtual auto resolve_internal_type_context(
      Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  virtual auto resolve_external_type_context(
      Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  auto resolve_internal_addressable_binding(Perimortem::Core::View::Bytes route)
      const -> const Ttx::Concept::Abstract&;

  auto resolve_external_addressable_binding(Perimortem::Core::View::Bytes route)
      const -> const Ttx::Concept::Abstract&;

  auto resolve_internal_type_binding(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  auto resolve_external_type_binding(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  auto owns(const Ttx::Model::Callable& requester) const -> Bool;

  auto owns(const Field& requester) const -> Bool;

  auto is_externally_reachable(const Ttx::Model::Type& type) const -> Bool;

  constexpr auto get_source_monograph() const -> const Monograph& {
    return source;
  }

  constexpr auto get_source_monograph() -> Monograph& { return source; }

 public:
  TTX_CONTRACT(
      Composite,
      Ttx::Model::Type,
      0x0282e7c7f8774a16,
      0x89d2776428d5f650);

  Composite(const Composite&) = delete;
  Composite(Composite&&) = delete;
  auto operator=(const Composite&) -> Composite& = delete;
  auto operator=(Composite&&) -> Composite& = delete;

  // Type and Callable declarations remain outside the instance Layout. Fields
  // enter it only after their exact Types settle.
  auto bind_member(
      Ttx::Concept::Abstract& binding,
      Visibility binding_visibility) -> Bool;

  auto can_bind_member(const Ttx::Concept::Abstract& binding) const -> Bool;

  // Type access receives the authenticated local and enclosing Composite
  // chain without exposing a general purpose internal lookup surface.
  auto resolve_type(const Access::Type& access) const
      -> const Ttx::Concept::Abstract&;

  // Publication checks begin with this Composite's exported Types and then
  // walk its enclosing exported Type chain before consulting intrinsics.
  auto resolve_exported_type(const Access::Type& access) const
      -> const Ttx::Concept::Abstract&;

  // Declaration Types settle recursively before any Composite in the same
  // closure may complete Field Type edges.
  auto link_types() -> Bool;
  auto link_fields() -> Bool;
  auto link_initializers() -> Bool;
  auto link_callable_signatures() -> Bool;
  auto link_callable_bodies() -> Bool;
  auto finalize() -> Bool;

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  // Contextless lookup exposes only externally readable Types and Fields. An
  // authenticated Callable receives private Type context, while Address access
  // selects Fields through the instance Layout and applies hosted readability.
  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  virtual auto resolve_context(
      Perimortem::Core::View::Bytes route,
      const Ttx::Model::Callable& requester) const
      -> const Ttx::Concept::Abstract&;

  virtual auto resolve_context(
      Perimortem::Core::View::Bytes route,
      const Field& requester) const -> const Ttx::Concept::Abstract&;

  auto get_layout() const -> const Ttx::Model::Layouts::Named& override;

  // Layout selection supplies the exact Field. Composite contributes only the
  // Library access policy that cannot live on the shared TTX Addressable.
  auto is_readable(const Field& field, const Ttx::Concept::Abstract& requester)
      const -> Bool;

  virtual constexpr auto get_anchor() const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor> = 0;

  // Authored and completed observations borrow the same retained Fields. An
  // incomplete Field remains present but resolves to Invalid until its Type
  // edge settles.
  auto get_fields() const
      -> Perimortem::Core::View::Vector<Ttx::Concept::Reference<const Field>>;

  auto get_public_fields() const
      -> Perimortem::Core::View::Vector<Ttx::Concept::Reference<const Field>>;

  auto get_callables() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Callable>>;

  auto get_public_callables() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Callable>>;

  auto get_callable_bindings() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>;

  auto get_callable_bindings(const Ttx::Concept::Abstract& requester) const
      -> Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<const Ttx::Concept::Abstract>>;

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
    TypesLinked,
    FieldsLinked,
    InitializersLinked,
    CallableSignaturesLinked,
    CallablesLinked,
    Finalized,
  };

  auto bind_definition(
      Ttx::Concept::Abstract& binding,
      Visibility binding_visibility) -> Bool;

  auto interpret_alias(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition) -> Bool;

  Perimortem::Memory::Allocator::Arena& domain;
  Monograph& source;
  Materializations& materializations;
  Perimortem::Core::Option<const Composite&> enclosing_scope;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Field>> fields;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<const Field>>
      field_observations;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<const Field>>
      public_fields;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      addressable_bindings;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      external_addressable_bindings;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
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
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      callable_bindings;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      external_callable_bindings;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      type_bindings;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      external_type_bindings;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      static_binding_order;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      external_static_binding_order;
  Perimortem::Core::Option<const Ttx::Model::Layouts::Named&> layout;
  Perimortem::Core::View::Vector<Ttx::Concept::Reference<Field>> linking_fields;
  Stage stage = Stage::Authored;
};

}  // namespace Tetrodotoxin::Library::Language::Types
