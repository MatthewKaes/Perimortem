// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/attribute.hpp"
#include "tetrodotoxin/library/language/type_reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/concept/constant.hpp"
#include "ttx/reference/concept/layout.hpp"
#include "ttx/ffi/cpp/addressable.hpp"
#include "ttx/ffi/cpp/domain.hpp"

namespace Tetrodotoxin::Library::Language::Model {

// Layout is one authored Library descriptor and its eventual TTX Layout. Each
// source slot retains its TypeReference and exactly one delayed semantic edge.
// Parameter linking installs a real Layout-owned Addressable, while ordinary
// descriptor linking installs the exact Type. There is no parallel resolved
// inventory or optional semantic Layout to drift from those canonical slots.
//
// A nonempty authored Layout becomes observable through the TTX Layout
// contract only after every slot links. Empty `[]` is complete immediately.
// Function resolution enforces that boundary. Registration before link derives
// receiver role from declares_self() without pretending an unresolved shape is
// empty value flow.
class Layout final : public Ttx::Concept::Layout {
 public:
  // Slot is the retained source description for one Layout entry. Its Type
  // route and Anchor are retained source metadata, while interpretation decides
  // how punctuation produces this value.
  class Slot {
   public:
    constexpr Slot(
        Perimortem::Core::Option<TypeReference> type_reference,
        Ttx::Lexical::Anchor anchor,
        Perimortem::Core::View::Bytes name,
        Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
            attributes = {})
        : type_reference(type_reference),
          anchor(anchor),
          name(name),
          attributes(attributes) {}

    constexpr auto get_type_anchor() const -> Ttx::Lexical::Anchor {
      return type_reference.visit(
          [&]() { return anchor; },
          [](const TypeReference& reference) {
            return reference.get_anchor();
          });
    }

    constexpr auto has_type_reference() const -> Bool {
      return Bool(type_reference);
    }

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }

    // Attributes remain uninterpreted source metadata on the exact Layout slot.
    // Library execution ignores keys it does not own, while an embedding
    // Dialect can use the same slot to express a richer interface contract.
    constexpr auto get_attributes() const
        -> Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute> {
      return attributes;
    }

   private:
    friend class Layout;

    Perimortem::Core::Option<TypeReference> type_reference;
    Ttx::Lexical::Anchor anchor;
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
        attributes;
    Perimortem::Core::Option<const Ttx::Concept::Abstract*> edge;
  };

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Memory::Managed::Vector<Slot> slots,
      Ttx::Lexical::Anchor anchor,
      Bool parameters) -> Layout&;

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Memory::Managed::Vector<Slot> slots,
      Ttx::Lexical::Anchor anchor,
      Bool parameters) -> Layout&;

  // Shader inherits Stage signatures after neighboring Render contracts have
  // composed. The real Function already owns this Layout, so adding generated
  // slots here completes that one descriptor without retaining parser state or
  // creating a second signature model.
  auto retain_generated_slot(
      TypeReference type_reference,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
          attributes = {}) -> Bool;

  auto retain_generated_slot(
      const Type& type,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>
          attributes = {}) -> Bool;

  auto retain_generated_edge(Count index, const Type& type) -> Bool;

  auto link_restored(
      const Ttx::Concept::Abstract& host,
      Bool parameters,
      Perimortem::Core::Option<const Ttx::Model::Addressable&> self = {})
      -> Bool;

  Layout(const Layout&) = delete;
  Layout(Layout&&) = delete;
  auto operator=(const Layout&) -> Layout& = delete;
  auto operator=(Layout&&) -> Layout& = delete;

  // Both paths resolve the same authored TypeReference relationships. Parameter
  // slots materialize real Layout-owned Addressables, results retain selected
  // Types, and the reserved scalar result `self` retains parameter entry zero
  // itself. Every authored Type slot must provide a value. Only `[]` carries an
  // empty descriptor.
  auto link_parameters(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& host) -> Bool;

  auto link_types(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::Option<const Ttx::Model::Addressable&> self = {})
      -> Bool;

  // Named lookup returns the exact semantic entry retained by this Layout.
  // Positional, incomplete, or missing selections resolve Unknown.
  auto resolve_named(
      Perimortem::Core::View::Bytes route,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> host = {}) const
      -> const Ttx::Concept::Abstract&;

  // Publication remains beside the authored routes and final edges it checks.
  auto validate_publication(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& host) const -> Bool;

  auto declares_self() const -> Bool;
  auto is_linked() const -> Bool;

  auto get_size() const -> Count override;

  auto get_abstract(Count index) const
      -> Perimortem::Core::Option<const Ttx::Concept::Abstract&> override;

  auto get_name(Count index) const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> override;

  auto get_slot_anchor(Count index) const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor>;

  auto get_slot_attributes(Count index) const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Language::Attribute>;

  auto get_type_reference(Count index) const
      -> Perimortem::Core::Option<const TypeReference&>;

  auto get_declared_name(Count index) const -> Perimortem::Core::View::Bytes;

  auto fits_entry(
      const Ttx::Concept::Layout& target,
      Count source_index,
      Count target_index) const -> Bool override;

  auto fits_at(const Ttx::Concept::Layout& target, Count target_offset) const
      -> Bool override;

  auto get_fitted_at(
      const Ttx::Concept::Layout& target,
      Count target_offset,
      Count target_index) const
      -> Perimortem::Utility::Result<
          const Ttx::Concept::Abstract&,
          Ttx::Concept::Layout::Errors> override;

  void named(ttx_named_result result) const override;

 private:
  class RouteIdentity final : public Ttx::Concept::Constant {
   public:
    explicit RouteIdentity(Perimortem::Core::View::Bytes bytes);

    TTX_NAME(bytes);
    TTX_EMPTY_DOCUMENTATION();

    void route(ttx_abstract self, ttx_route_result result) const override;

   protected:
    auto negotiate(ttx_abstract requirement) const
        -> ttx_interface_relation override;

   private:
    struct Binding {
      ttx_route_ops operations;
    };

    static auto select(ttx_route self) -> const RouteIdentity&;
    static auto TTX_CALL candidate(ttx_route self) -> ttx_abstract;
    static auto TTX_CALL route_bytes(ttx_route self) -> ttx_borrowed_bytes;

    Perimortem::Core::View::Bytes bytes;
    Binding binding;
  };

  class RouteLayout final : public Ttx::Concept::Layout {
   public:
    explicit RouteLayout(
        const Tetrodotoxin::Library::Language::Model::Layout& owner)
        : owner(owner) {}

    auto get_size() const -> Count override;
    auto get_abstract(Count index) const
        -> Perimortem::Core::Option<const Ttx::Concept::Abstract&> override;
    auto fits_entry(
        const Ttx::Concept::Layout& target,
        Count source_index,
        Count target_index) const -> Bool override;
    auto fits_at(const Ttx::Concept::Layout& target, Count target_offset) const
        -> Bool override;
    auto get_fitted_at(
        const Ttx::Concept::Layout& target,
        Count target_offset,
        Count target_index) const
        -> Perimortem::Utility::Result<
            const Ttx::Concept::Abstract&,
            Ttx::Concept::Layout::Errors> override;

   private:
    const Tetrodotoxin::Library::Language::Model::Layout& owner;
  };

  struct NamedBinding {
    ttx_named_ops operations;
  };

  Layout(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Memory::Managed::Vector<Slot> slots,
      Ttx::Lexical::Anchor anchor,
      Bool parameters);

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& host,
      Bool parameters,
      Perimortem::Core::Option<const Ttx::Model::Addressable&> self) -> Bool;

  auto is_named() const -> Bool;
  auto get_slot(Count index) const -> Perimortem::Core::Option<const Slot&>;
  auto fits_value(
      const Ttx::Concept::Layout& target,
      Count source_index,
      Count target_index) const -> Bool;
  auto has_unique_names() const -> Bool;
  void retain_route(Perimortem::Core::View::Bytes route);

  static auto select(ttx_named self) -> const Layout&;
  static auto TTX_CALL named_candidate(ttx_named self) -> ttx_layout;
  static auto TTX_CALL named_source(ttx_named self) -> ttx_layout;
  static auto TTX_CALL named_routes(ttx_named self) -> ttx_layout;
  static void TTX_CALL
      visit_named_routes(ttx_named self, ttx_named_route_sink result);
  static void TTX_CALL select_named_route(
      ttx_named self,
      ttx_borrowed_bytes route,
      ttx_named_selection_result result);

  Perimortem::Memory::Allocator::Arena& domain;
  Perimortem::Memory::Managed::Vector<Slot> slots;
  Perimortem::Memory::Managed::Vector<RouteIdentity*> route_identities;
  RouteLayout route_layout;
  NamedBinding named_binding;
  Ttx::Lexical::Anchor anchor;
  Bool parameters;
};

}  // namespace Tetrodotoxin::Library::Language::Model
