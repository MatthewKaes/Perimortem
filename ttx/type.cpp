// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/type.hpp"

auto Ttx::Type::Member::equivalent_to(const Member& other) const -> Bool {
  if (type && other.type) {
    return type->equivalent_to(*other.type);
  }

  return False;
}

auto Ttx::Type::canonical() const -> const Type* {
  const Type* slow = this;
  const Type* fast = this;
  while (fast && fast->alias_parent) {
    slow = slow ? slow->alias_parent : nullptr;
    fast = fast->alias_parent;
    if (fast && fast->alias_parent) {
      fast = fast->alias_parent;
    }

    if (slow && slow == fast) {
      return nullptr;
    }
  }

  const Type* canonical_type = this;
  while (canonical_type && canonical_type->alias_parent) {
    canonical_type = canonical_type->alias_parent;
  }

  return canonical_type;
}

auto Ttx::Type::equivalent_to(const Type& other) const -> Bool {
  const Type* left = canonical();
  const Type* right = other.canonical();
  return left && right && left == right;
}

auto Ttx::Type::find_member(Perimortem::Core::View::Bytes name) const
    -> const Member* {
  const Type* type = canonical();
  if (!type) {
    return nullptr;
  }

  for (Count member_index = 0; member_index < type->members.get_size();
       member_index++) {
    if (type->members[member_index].get_name() == name) {
      return &type->members[member_index];
    }
  }

  return nullptr;
}

auto Ttx::Type::find_type(Perimortem::Core::View::Bytes name) const
    -> const Type* {
  const Type* type = canonical();
  if (!type) {
    return nullptr;
  }

  for (Count type_index = 0; type_index < type->types.get_size();
       type_index++) {
    const Type* nested_type = type->types[type_index];
    if (nested_type && nested_type->get_name() == name) {
      return nested_type;
    }
  }

  return nullptr;
}

auto Ttx::Type::find_function(Perimortem::Core::View::Bytes name) const
    -> const Function* {
  const Type* type = canonical();
  if (!type) {
    return nullptr;
  }

  for (Count function_index = 0; function_index < type->functions.get_size();
       function_index++) {
    if (type->functions[function_index].get_name() == name) {
      return &type->functions[function_index];
    }
  }

  return nullptr;
}
