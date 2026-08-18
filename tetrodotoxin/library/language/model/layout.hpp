// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/type_reference.hpp"
#include "ttx/concept/layout.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Model {

// Layout is one authored Library descriptor and its eventual TTX Layout. Each
// source slot retains its TypeReference and exactly one delayed semantic edge.
// Parameter linking installs the real Parameter Addressable, while ordinary
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
  // Function parameters require an empty or Named Layout and alone admit a
  // leading reserved `self` entry. Results and other descriptor consumers use
  // the general positional or Named form.
  static auto interpret_parameters(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& host) -> Perimortem::Core::Option<Layout&>;

  static auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& host) -> Perimortem::Core::Option<Layout&>;

  Layout(const Layout&) = delete;
  Layout(Layout&&) = delete;
  auto operator=(const Layout&) -> Layout& = delete;
  auto operator=(Layout&&) -> Layout& = delete;

  // Both paths resolve the same authored TypeReference facts. Parameters
  // materialize a real Addressable while results retain the selected Type
  // directly. Every authored Type slot must provide a value. Only `[]` carries
  // an empty descriptor.
  auto link_parameters(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& host) -> Bool;

  auto link_types(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& host) -> Bool;

  // Named lookup returns the exact semantic entry retained by this Layout.
  // Positional, incomplete, or missing selections resolve Invalid.
  auto resolve_named(Perimortem::Core::View::Bytes route) const
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
  class Slot {
   public:
    constexpr Slot(
        Perimortem::Core::Option<TypeReference> type_reference,
        Ttx::Lexical::Anchor anchor,
        Perimortem::Core::View::Bytes name)
        : type_reference(type_reference), anchor(anchor), name(name) {}

    constexpr auto get_type_anchor() const -> Ttx::Lexical::Anchor {
      return type_reference.visit(
          [&]() { return anchor; },
          [](const TypeReference& reference) {
            return reference.get_anchor();
          });
    }

    Perimortem::Core::Option<TypeReference> type_reference;
    Ttx::Lexical::Anchor anchor;
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::Option<
        Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
        edge;
  };

  Layout(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Memory::Managed::Vector<Slot> slots,
      Ttx::Lexical::Anchor anchor)
      : domain(domain), slots(slots), anchor(anchor) {}

  static auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& host,
      Bool parameters) -> Perimortem::Core::Option<Layout&>;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& host,
      Bool parameters) -> Bool;

  auto is_named() const -> Bool;
  auto get_slot(Count index) const -> Perimortem::Core::Option<const Slot&>;
  auto fits_value(
      const Ttx::Concept::Layout& target,
      Count source_index,
      Count target_index) const -> Bool;
  auto has_unique_names() const -> Bool;

  Perimortem::Memory::Allocator::Arena& domain;
  Perimortem::Memory::Managed::Vector<Slot> slots;
  Ttx::Lexical::Anchor anchor;
};

}  // namespace Tetrodotoxin::Library::Language::Model
