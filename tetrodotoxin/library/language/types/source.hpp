// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/types/composite.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Source is the synthetic root Composite for one Library Monograph. It owns
// fixed root presentation, source extension grammar, and Static publication
// while Composite keeps the shared inventories, lookup, and lifecycle.
class Source : public Composite {
 private:
  Source(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Monograph& source,
      Materializations& materializations);

  const Ttx::Concept::Documentation& documentation;
  const Ttx::Model::Layouts::Named& instance_layout;

 protected:
  auto can_retain_binding(const Ttx::Concept::Abstract& binding) const
      -> Bool override;

  auto retain_binding(
      Ttx::Concept::Abstract& binding,
      Visibility binding_visibility) -> Bool override;

  auto publish_linked_field(Field& field) -> void override;

  auto complete_field_layout() -> void override;

  auto validate_linked_callable(const Ttx::Model::Callable& callable)
      -> Bool override;

  auto grants_complete_access(const Ttx::Concept::Abstract& requester) const
      -> Bool override;

  auto resolve_internal_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_internal_type_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_external_type_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

 public:
  TTX_CONTRACT(Source, Composite, 0xa972070bd27746e0, 0x959dd29d7924aed4);

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Monograph& source,
      Materializations& materializations) -> Source&;

  Source(const Source&) = delete;
  Source(Source&&) = delete;
  auto operator=(const Source&) -> Source& = delete;
  auto operator=(Source&&) -> Source& = delete;

  // Source consumes its extension forms and delegates every ordinary Definition
  // to Composite. The root is therefore the only place that can admit using
  // without teaching Composite about source grammar.
  auto parse(Ttx::Lexical::Cursor& cursor) -> Bool;

  auto bind_static(
      Ttx::Concept::Abstract& binding,
      Visibility binding_visibility) -> Bool;

  auto can_bind_static(const Ttx::Concept::Abstract& binding) const -> Bool;

  TTX_NAME("source"_view);

  TTX_DOCUMENTATION(documentation);

  constexpr auto get_anchor() const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor> override {
    return {};
  }

  constexpr auto resolve() const -> const Ttx::Concept::Abstract& override {
    return *this;
  }

  auto get_layout() const -> const Ttx::Model::Layouts::Named& override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_context(
      Perimortem::Core::View::Bytes route,
      const Ttx::Model::Callable& requester) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_context(
      Perimortem::Core::View::Bytes route,
      const Field& requester) const -> const Ttx::Concept::Abstract& override;

  auto resolve_context(
      Perimortem::Core::View::Bytes route,
      const Monograph& requester) const -> const Ttx::Concept::Abstract&;
};

}  // namespace Tetrodotoxin::Library::Language::Types
