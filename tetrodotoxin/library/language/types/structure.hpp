// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/managed/vector.hpp"

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

// Structure owns Library Type, Callable, and member lookup, real Field edges,
// and one Named instance Layout. Source and Object specialize this same anatomy
// without reproducing its inventories.
class Structure : public Ttx::Model::Type {
 protected:
  Structure(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Documentation& documentation,
      Visibility visibility,
      Monograph& source,
      Materializations& materializations,
      Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
          enclosing_scope,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> name_anchor);

  auto can_accept_declaration() const -> Bool;

  auto can_bind_declaration(const Ttx::Concept::Abstract& binding) const
      -> Bool;

  auto publish_binding(
      Ttx::Concept::Abstract& binding,
      Visibility binding_visibility) -> void;

  auto retain_declaration_field(Field::Source field) -> Bool;

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

  constexpr auto get_source_monograph() const -> const Monograph& {
    return source;
  }

  constexpr auto get_source_monograph() -> Monograph& { return source; }

 public:
  using ClassCatagory = Structure;
  static constexpr Perimortem::System::Uuid contract_id{
    0xe3773c0325224200,
    0xaeb9a3131139c16f,
  };

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      Monograph& source,
      Materializations& materializations,
      const Structure& enclosing_scope) -> Perimortem::Core::Option<Structure&>;

  Structure(const Structure&) = delete;
  Structure(Structure&&) = delete;
  auto operator=(const Structure&) -> Structure& = delete;
  auto operator=(Structure&&) -> Structure& = delete;

  // Type and Callable declarations remain outside the instance Layout. Fields
  // enter it only after their exact Types settle.
  auto bind_member(
      Ttx::Concept::Abstract& binding,
      Visibility binding_visibility) -> Bool;

  auto can_bind_member(const Ttx::Concept::Abstract& binding) const -> Bool;

  // Structure retains complete authored facts until its declaration barrier
  // can construct and publish every exact Field identity together.
  auto retain_field(Field::Source field) -> Bool;

  // Type access receives the authenticated local and enclosing Structure
  // chain without exposing a general purpose internal lookup surface.
  auto resolve_type(const Access::Type& access) const
      -> const Ttx::Concept::Abstract&;

  // Publication checks begin with this Structure's exported Types and then
  // walk its enclosing exported Type chain before consulting intrinsics.
  auto resolve_exported_type(const Access::Type& access) const
      -> const Ttx::Concept::Abstract&;

  // Declaration Types settle recursively before any Structure in the same
  // closure may construct Fields.
  auto link_types() -> Bool;
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

  // Layout selection supplies the exact Field. Structure contributes only the
  // Library access policy that cannot live on the shared TTX Addressable.
  auto is_readable(const Field& field, const Ttx::Concept::Abstract& requester)
      const -> Bool;

  constexpr auto get_visibility() const -> Visibility { return visibility; }

  constexpr auto get_anchor() const
      -> const Perimortem::Core::Option<Ttx::Lexical::Anchor>& {
    return anchor;
  }

  constexpr auto get_name_anchor() const
      -> const Perimortem::Core::Option<Ttx::Lexical::Anchor>& {
    return name_anchor;
  }

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

  auto complete_declaration(Ttx::Lexical::Anchor complete_anchor) -> void;

  auto bind_declaration(
      Ttx::Concept::Abstract& binding,
      Visibility binding_visibility) -> Bool;

  Perimortem::Memory::Allocator::Arena& domain;
  Perimortem::Core::View::Bytes name;
  const Ttx::Concept::Documentation& documentation;
  Visibility visibility;
  Monograph& source;
  Materializations& materializations;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      enclosing_scope;
  Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor;
  Perimortem::Core::Option<Ttx::Lexical::Anchor> name_anchor;
  Perimortem::Memory::Managed::Vector<Field::Source> field_sources;
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
  Stage stage = Stage::Authored;
};

}  // namespace Tetrodotoxin::Library::Language::Types
