// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/type.hpp"

// The type implementation intentionally has very little policy. TTX originally
// wants to make many things feel type shaped because types are the names users
// write most often, but that path quickly turns Type into a duplicate program
// model. The useful boundary is narrower. Type owns identity and dispatch.
// Layout owns shape. Resolution owns loaded source records and imported names.
// Lowering owns ABI facts.
//
// Alias handling is the pressure point that makes this split visible. An alias
// should behave like its root type for member, nested type, and function
// lookup, while its documentation remains local to the authored alias name.
// Canonicalization therefore answers identity only. Presentation tools can
// choose whether to show alias prose, root prose, or both.
//
// The lookup functions all canonicalize before walking their local vectors so
// aliases expose the same dispatch surface as their root type. They still
// return the canonical member, type, or function object because those are the
// objects that own the actual dispatch entries.

auto Ttx::Type::Member::equivalent_to(const Member& other) const -> Bool {
  if (type && other.type) {
    return type->equivalent_to(*other.type);
  }

  return False;
}

auto Ttx::Type::canonical() const -> const Type* {
  // Aliases form a parent chain. Use the slow and fast pointer technique first
  // so a bad package cannot trap canonicalization in an infinite loop.
  //
  // Returning null keeps the type model honest. Resolution owns the source
  // context needed to report which alias declarations formed the cycle.
  const Type* slow = this;
  const Type* fast = this;
  while (fast && fast->alias_parent) {
    slow = slow ? slow->alias_parent : nullptr;
    fast = fast->alias_parent->alias_parent;

    if (slow && fast && slow == fast) {
      return nullptr;
    }
  }

  // The chain is acyclic, so a straight walk now reaches the root identity.
  // Documentation is not folded during this walk. Alias prose stays on the
  // alias and root prose stays on the root type.
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
  // Lookup goes through canonical identity so aliases expose the same members
  // as the type they name. The first authored name wins for this simple lookup,
  // matching Layout name access. The alias still keeps its own documentation
  // and authored name outside this lookup.
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
  // Nested type lookup is the `::` access path. It is intentionally separate
  // from member lookup so type metadata never has to be duplicated into the
  // projected layout shape. The first authored name wins for this name probe.
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
  // Function lookup is the `->` access path. Pure layouts cannot reach this
  // table because they have no type identity to dispatch through. Overload
  // selection needs argument layouts, so it should be a separate query instead
  // of making this simple name probe guess.
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
