// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/layout.hpp"

Ttx::Layout::Layout(const Type& source_type) {
  const Type* canonical_type = source_type.canonical();
  if (canonical_type) {
    members = canonical_type->get_members();
  }
}

auto Ttx::Layout::count_required_members() const -> Count {
  Count member_count = get_member_count();
  Count required_member_count = member_count;
  while (required_member_count > 0 &&
         member_at(required_member_count - 1).is_defaulted()) {
    required_member_count--;
  }

  for (Count member_index = 0; member_index < required_member_count;
       member_index++) {
    if (member_at(member_index).is_defaulted()) {
      return member_count + 1;
    }
  }

  return required_member_count;
}

auto Ttx::Layout::equivalent_to(const Layout& other) const -> Bool {
  if (get_member_count() != other.get_member_count()) {
    return False;
  }

  if (has_named_members() || other.has_named_members()) {
    for (Count member_index = 0; member_index < get_member_count();
         member_index++) {
      Type::Member member = member_at(member_index);
      if (!member.is_named()) {
        return False;
      }

      const Type::Member* mapped_member = other.find_member(member.get_name());
      if (!mapped_member) {
        return False;
      }

      if (!member.equivalent_to(*mapped_member)) {
        return False;
      }
    }

    return True;
  }

  for (Count member_index = 0; member_index < get_member_count();
       member_index++) {
    if (!member_at(member_index).equivalent_to(other.member_at(member_index))) {
      return False;
    }
  }

  return True;
}

auto Ttx::Layout::fits(const Layout& target) const -> Bool {
  if (target.is_empty()) {
    return equivalent_to(target);
  }

  Count target_member_count = target.get_member_count();
  Count required_target_members = target.count_required_members();
  if (required_target_members > target_member_count) {
    return False;
  }

  if (has_named_members()) {
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
