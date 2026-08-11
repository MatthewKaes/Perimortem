// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/selection.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/language/visibility.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/types/defined.hpp"
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
class Composite : public Defined {
 protected:
  Composite(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition);

  auto interpret_definition(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition) -> Bool;

  auto can_accept_definition() const -> Bool;

  auto can_bind_definition(const Ttx::Concept::Abstract& binding) const -> Bool;

  auto publish_binding(Ttx::Concept::Abstract& binding) -> void;

  virtual auto retain_binding(Ttx::Concept::Abstract& binding) -> Bool;

  virtual auto complete_field_layout() -> void;

  virtual auto validate_linked_callable(const Ttx::Model::Callable& callable)
      -> Bool;

  // Hosting is provenance and access authority rather than universal semantic
  // parentage. Following only the required Definition host chain reaches the
  // one Monograph that owns this Composite's completion transaction.
  auto get_monograph() -> Tetrodotoxin::Language::Monograph&;

  auto get_monograph() const -> const Tetrodotoxin::Language::Monograph&;

 public:
  TTX_CONTRACT(Composite, Defined, 0x0282e7c7f8774a16, 0x89d2776428d5f650);

  Composite(const Composite&) = delete;
  Composite(Composite&&) = delete;
  auto operator=(const Composite&) -> Composite& = delete;
  auto operator=(Composite&&) -> Composite& = delete;

  // A target grants private authority only to callers whose Definition host
  // chain contains this Type. The caller chain authenticates access without
  // becoming a semantic parent route or supplying an implicit receiver.
  auto grants_private_access(const Ttx::Model::Type& caller_scope) const
      -> Bool;

  // Addressable selection is local to this exact Composite category. The
  // caller scope controls publication filtering, while an incomplete inferred
  // Field remains unavailable until its declaration has established a Type.
  auto resolve_lexical_addressable(
      Perimortem::Core::View::Bytes route,
      const Ttx::Model::Type& caller_scope) const
      -> const Ttx::Concept::Abstract&;

  // A lexical root may climb only this Composite's Definition host chain. Each
  // visited Composite authenticates the unchanged caller before exposing a
  // private Type; the final source context and intrinsic lookup remain public
  // owner-directed fallbacks.
  auto resolve_type_root(
      Perimortem::Core::View::Bytes route,
      const Ttx::Model::Type& caller_scope) const
      -> const Ttx::Concept::Abstract&;

  // An explicit qualification segment selects only this Composite's local Type
  // category. Missing `receiver::name` never escapes into the receiver's host.
  auto resolve_type(
      Perimortem::Core::View::Bytes route,
      const Ttx::Model::Type& caller_scope) const
      -> const Ttx::Concept::Abstract&;

  // Authored Type access starts in this declaration scope and preserves that
  // exact caller authority through every qualified segment.
  auto resolve_type(const Access::Type& access) const
      -> const Ttx::Concept::Abstract&;

  // Publication checks begin with this Composite's exported Types and then
  // walk its enclosing exported Type chain before consulting intrinsics.
  auto resolve_exported_type(const Access::Type& access) const
      -> const Ttx::Concept::Abstract&;

  auto is_externally_reachable(const Ttx::Model::Type& type) const -> Bool;

  // Declaration Types settle recursively before any Composite in the same
  // closure may complete Field Type edges.
  auto link_types() -> Bool;
  auto link_fields() -> Bool;
  auto link_initializers() -> Bool;
  auto link_callable_signatures() -> Bool;
  auto link_callable_bodies() -> Bool;
  auto finalize() -> Bool;

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  // Contextless lookup exposes only Types. Address access selects Fields
  // through the instance Layout and applies publication plus host authority.
  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto get_layout() const -> const Ttx::Model::Layouts::Named& override;

  auto get_addressables(
      Tetrodotoxin::Language::Visibility visibility =
          Tetrodotoxin::Language::Visibility::Private) const {
    return Perimortem::Core::View::Selection(
        addressables.get_view(),
        [this, visibility](const auto& binding) -> Bool {
          return is_visible(binding.get(), visibility);
        });
  }

  auto get_callables(
      Tetrodotoxin::Language::Visibility visibility =
          Tetrodotoxin::Language::Visibility::Private) const {
    return Perimortem::Core::View::Selection(
        callables.get_view(), [this, visibility](const auto& binding) -> Bool {
          return is_visible(binding.get(), visibility);
        });
  }

  auto get_types(
      Tetrodotoxin::Language::Visibility visibility =
          Tetrodotoxin::Language::Visibility::Private) const {
    return Perimortem::Core::View::Selection(
        types.get_view(), [this, visibility](const auto& binding) -> Bool {
          return is_visible(binding.get(), visibility);
        });
  }

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

  auto is_visible(
      const Ttx::Concept::Abstract& binding,
      Tetrodotoxin::Language::Visibility visibility) const -> Bool;

  auto get_enclosing_scope() const
      -> Perimortem::Core::Option<const Composite&>;

  auto resolve_exported_type_root(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract&;

  Perimortem::Memory::Allocator::Arena& domain;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      addressables;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      callables;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      types;
  Perimortem::Core::Option<const Ttx::Model::Layouts::Named&> layout;
  Stage stage = Stage::Authored;
};

}  // namespace Tetrodotoxin::Library::Language::Types
