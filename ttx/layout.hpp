// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/type.hpp"

namespace Ttx {

// Layout is the shape-bearing half of the TTX model and represents a fluid data
// state used for describing value transformations without the need for a full
// named Type.
//
// It is a non-owning view over `Type::Member` entries. A layout can come from a
// source literal, from a function parameter list, from a return list, from a
// swizzle, or from projecting a type with `Layout(type)`. In every case it asks
// the same family of shape questions:
//
// * What names exist?
// * What order do entries appear in?
// * What types do entries refer to?
// * Is this shape equivalent to, or a subset of, another shape?
//
// Layouts are similar to anonymous structs in some languages, but differ in
// ways that enable TTX's data model. The object preserves whatever member view
// it is handed, including optional names and duplicates, but names are
// transient language facts. Source repacks such as grouping, slicing, and
// swizzling produce positional layouts unless the operator explicitly authors
// or preserves names. Names are acquired from authored named packs, member
// declarations, function parameter and return layouts, type layouts, or other
// receiving boundaries that own those names.
//
// If a layout view contains duplicate names, the leftmost duplicate is used for
// name access while every entry remains positionally valid. The owner that has
// source context should diagnose the shadowing when it is likely surprising.
// Layout keeps the shape representable because dialect and generated packages
// may still need to carry it far enough to report a useful error.
//
// A layout is not part of the formal type system and contains no nested types,
// functions, or identity for dispatch. That distinction is what makes `.` and
// `:[...]` layout operators while `::` and `->` are type queries.
//
// Types can project to layouts, but layouts do not become types. That keeps the
// model from needing a cycle where every type owns storage shape and every
// layout owns type identity. A type only answers who something is, has, and
// does. A layout answers what concrete shape an expression presents.
//
// Layout also does not own ABI storage facts. A layout can prove source shape,
// but lowering still owns the target specific answer for size, alignment, and
// offsets. This delays actual instantiation concepts till later enabling more
// reuse of the system across Tetrodotoxin layers.
//
// Keeping these rules local in particular benefits the resolution and dialect
// layers which all leverage layout and types as the shared "lingua franca"
// instead of having to create various interoping domain specific systems.
class Layout {
 public:
  constexpr Layout() = default;
  explicit Layout(const Type& type);
  explicit constexpr Layout(
      Perimortem::Core::View::Vector<Type::Member> members)
      : members(members) {}

  constexpr auto is_empty() const -> Bool { return members.is_empty(); }

  // Enumerates the member entries in this layout. Use `equivalent_to`, `fits`,
  // or `find_member` for semantic questions so name mapping, duplicate-name
  // access, and defaults stay inside Layout.
  constexpr auto get_members() const
      -> Perimortem::Core::View::Vector<Type::Member> {
    return members;
  }
  constexpr auto get_member_count() const -> Count {
    return members.get_size();
  }

  // Returns an empty member sentinel when the index is outside the layout.
  //
  // Layout algorithms use this as a safe bottom case while walking optional
  // suffixes and positional entries. Code that needs to prove a member exists
  // should check `get_member_count()` first or use `find_member()`.
  constexpr auto member_at(Count index) const -> Type::Member {
    return index < members.get_size() ? members[index] : Type::Member();
  }

  // Finds the first member with the requested name for layout `.` access.
  //
  // Layout preserves duplicate names when a producer supplies them, but the
  // leftmost duplicate is the only name-addressable member. Later duplicates
  // remain positionally valid and should be diagnosed by the source, package,
  // or dialect owner that knows whether the shadowing was intentional.
  constexpr auto find_member(Perimortem::Core::View::Bytes name) const
      -> const Type::Member* {
    for (Count member_index = 0; member_index < members.get_size();
         member_index++) {
      const Type::Member& member = members[member_index];
      if (member.get_name() == name) {
        return &member;
      }
    }

    return nullptr;
  }

  // Exact structural equality.
  //
  // Use this when the question is whether two layouts are the same shape.
  // `[.x Real_32, .y Real_32]` is equivalent to the same ordered fields.
  // `[.y Real_32, .x Real_32]` has the same names and types but not the same
  // ordered shape. `[Real_32, Real_32]` is equivalent to another positional two
  // entry layout with the same canonical member types in the same order.
  //
  // Names are part of ordered shape, but they do not create a nested record
  // identity. Duplicate names are preserved and compared in place. Defaults
  // and documentation do not make two layouts equivalent because they do not
  // change the exact entries that are present.
  //
  // This is the right query for proving a resolved shape still has the exact
  // same entries, or for comparing signatures after defaults have already been
  // materialized. It is the wrong query for initialization and calls because it
  // rejects valid construction where trailing defaulted target entries are
  // omitted by the source.
  auto equivalent_to(const Layout& other) const -> Bool;

  // Construction fit.
  //
  // Use this when a source layout is being assigned to, passed into, returned
  // as, or used to construct a target type layout. `source.fits(target)` asks
  // whether the source supplies every required target member and omits only
  // target members that have defaults at the back of the target layout.
  //
  // `[.x Real_32, .y Real_32]` fits `[.x Real_32, .y Real_32,
  // .z Real_32 = default]` because `.z` is optional and trails the required
  // members. `[Real_32, Real_32]` fits a positional target with the same first
  // two entries and a defaulted third entry. `[.x Real_32, .z Real_32]` does
  // not fit a named target that also requires `.y`.
  //
  // Defaults are only valid from back to front. A target shaped like
  // `[.x defaulted, .y required]` is malformed for construction because the
  // omitted source would create a gap before a required member. That mirrors
  // function parameter semantics and keeps positional construction simple.
  //
  // Use `equivalent_to` instead when default filling would hide a bug, such as
  // proving that two already resolved layouts are identical. Use `fits` instead
  // of `equivalent_to` when the user is authoring a construction, call, return,
  // or assignment and defaulted trailing members should be accepted.
  auto fits(const Layout& target) const -> Bool;

 private:
  // Returns an impossible count when a target has a defaulted member before a
  // required member.
  //
  // TTX only allows defaults from back to front so construction never has to
  // guess whether a missing positional entry was skipped or shifted.
  auto count_required_members() const -> Count;

  Perimortem::Core::View::Vector<Type::Member> members;
};

}  // namespace Ttx
