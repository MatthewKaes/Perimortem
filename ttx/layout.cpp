// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/layout.hpp"

// Layout is where TTX keeps shape policy. That is why this file contains both
// exact equivalence and construction fitting instead of sending those checks to
// a separate helper layer. The object that knows how names, positions, member
// types, and trailing defaults interact is the layout itself.
//
// The split from Type is deliberate. A Type can be projected into a Layout, but
// a repacked layout does not gain type identity, nested types, or functions.
// This breaks the cycle that appears if Type tries to own every storage shape
// question while Layout also needs member types. Layout can contain typed
// members as a view, and Type can stay focused on identity.
//
// The two public questions are intentionally different. `equivalent_to` proves
// two layouts have the same ordered entries. `fits` answers whether an authored
// source shape can construct a target shape where unique names may map entries
// and trailing target members may be filled by defaults. Keeping both words
// makes call, assignment, return, and repack code say which proof it actually
// needs.

static constexpr auto has_named_members(const Ttx::Layout& layout) -> Bool {
  Perimortem::Core::View::Vector<Ttx::Type::Member> members =
      layout.get_members();
  for (Count member_index = 0; member_index < members.get_size();
       member_index++) {
    if (members[member_index].is_named()) {
      return True;
    }
  }

  return False;
}

static constexpr auto has_duplicate_named_members(const Ttx::Layout& layout)
    -> Bool {
  Perimortem::Core::View::Vector<Ttx::Type::Member> members =
      layout.get_members();
  for (Count left_index = 0; left_index < members.get_size(); left_index++) {
    Ttx::Type::Member left = members[left_index];
    if (!left.is_named()) {
      continue;
    }

    for (Count right_index = left_index + 1;
         right_index < members.get_size(); right_index++) {
      Ttx::Type::Member right = members[right_index];
      if (right.is_named() && left.get_name() == right.get_name()) {
        return True;
      }
    }
  }

  return False;
}

static constexpr auto equivalent_ordered_member(
    Ttx::Type::Member left,
    Ttx::Type::Member right) -> Bool {
  return left.get_name() == right.get_name() && left.equivalent_to(right);
}

Ttx::Layout::Layout(const Type& source_type) {
  // Projecting a type to a layout uses canonical identity so aliases produce
  // the same member shape as their root type. Alias documentation stays on the
  // Type because prose is contextual, while the layout answers only shape.
  const Type* canonical_type = source_type.canonical();
  if (canonical_type) {
    members = canonical_type->get_members();
  }
}

auto Ttx::Layout::count_required_members() const -> Count {
  // Defaults are only legal as a trailing suffix. Walk backward to find the
  // required prefix, then make a second pass to reject any gap where a default
  // appears before a required member.
  Count member_count = get_member_count();
  Count required_member_count = member_count;
  while (required_member_count > 0 &&
         member_at(required_member_count - 1).is_defaulted()) {
    required_member_count--;
  }

  for (Count member_index = 0; member_index < required_member_count;
       member_index++) {
    if (member_at(member_index).is_defaulted()) {
      // Return a count larger than the layout so callers can treat a malformed
      // default gap the same way they treat an impossible construction.
      return member_count + 1;
    }
  }

  return required_member_count;
}

auto Ttx::Layout::equivalent_to(const Layout& other) const -> Bool {
  if (get_member_count() != other.get_member_count()) {
    return False;
  }

  // Equivalence is ordered shape. Names are preserved on entries, including
  // duplicate names, but they do not remap the comparison like struct fields.
  for (Count member_index = 0; member_index < get_member_count();
       member_index++) {
    if (!equivalent_ordered_member(
            member_at(member_index), other.member_at(member_index))) {
      return False;
    }
  }

  return True;
}

auto Ttx::Layout::fits(const Layout& target) const -> Bool {
  // An empty target can only be constructed by the exact same empty shape. This
  // keeps the empty layout as the bottom case rather than a wildcard.
  if (target.is_empty()) {
    return equivalent_to(target);
  }

  // The target owns default policy. A malformed target with a default gap is
  // rejected before comparing source members so construction never has to infer
  // skipped positional entries.
  Count target_member_count = target.get_member_count();
  Count required_target_members = target.count_required_members();
  if (required_target_members > target_member_count) {
    return False;
  }

  // When both sides have unique names, construction can use the names as a map.
  // Duplicate names fall back to ordered construction because only the leftmost
  // duplicate is addressable by name.
  if (has_named_members(*this) && has_named_members(target) &&
      !has_duplicate_named_members(*this) &&
      !has_duplicate_named_members(target)) {
    for (Count source_index = 0; source_index < get_member_count();
         source_index++) {
      Type::Member source_member = member_at(source_index);
      if (!source_member.is_named()) {
        return False;
      }

      const Type::Member* target_member =
          target.find_member(source_member.get_name());
      if (!target_member || !target_member->equivalent_to(source_member)) {
        return False;
      }
    }

    for (Count target_index = 0; target_index < target_member_count;
         target_index++) {
      Type::Member target_member = target.member_at(target_index);
      if (!target_member.is_named()) {
        return False;
      }

      if (!find_member(target_member.get_name()) &&
          target_index < required_target_members) {
        return False;
      }
    }

    return True;
  }

  // Ordered construction is the bottom case. The source must cover the required
  // prefix, may stop before the defaulted suffix, and cannot overrun the target
  // layout. The layout view may carry names, but construction here is asking
  // whether the typed entries can be packed in this order.
  if (get_member_count() > target_member_count ||
      get_member_count() < required_target_members) {
    return False;
  }

  for (Count source_index = 0; source_index < get_member_count();
       source_index++) {
    if (!target.member_at(source_index).equivalent_to(member_at(source_index))) {
      return False;
    }
  }

  return True;
}
