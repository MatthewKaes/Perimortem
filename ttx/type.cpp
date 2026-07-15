// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;

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

auto Ttx::Type::canonical() const -> const Type& {
  // Aliases form a parent chain. Use the slow and fast pointer technique first
  // so a bad package cannot trap canonicalization in an infinite loop.
  //
  // Returning Invalid keeps the type model non-null while still giving
  // resolution a bottom value it can diagnose at the source location that
  // created the cycle.
  const Type* slow = this;
  const Type* fast = this;
  while (fast && fast->alias_parent) {
    slow = slow ? slow->alias_parent : nullptr;
    fast = fast->alias_parent->alias_parent;
    if (slow && fast && slow == fast) {
      return invalid();
    }
  }

  // The chain is acyclic, so a straight walk now reaches the root identity.
  // Documentation is not folded during this walk. Alias prose stays on the
  // alias and root prose stays on the root type.
  const Type* canonical_type = this;
  while (canonical_type && canonical_type->alias_parent) {
    canonical_type = canonical_type->alias_parent;
  }

  return canonical_type == nullptr ? invalid() : *canonical_type;
}

auto Ttx::Type::equivalent_to(const Type& other) const -> Bool {
  const Type& left = canonical();
  const Type& right = other.canonical();
  return !left.is_invalid() && !right.is_invalid() && &left == &right;
}

auto Ttx::Type::display_name() const -> View::Bytes {
  const Attribute* attribute = find_attribute(display_name_attribute);
  return attribute != nullptr && !attribute->get_bytes().is_empty()
             ? attribute->get_bytes()
             : name;
}

auto Ttx::Type::has_display_name() const -> Bool {
  const Attribute* attribute = find_attribute(display_name_attribute);
  return attribute != nullptr && !attribute->get_bytes().is_empty();
}

auto Ttx::Type::describe() const -> Dynamic::Bytes {
  Dynamic::Bytes description;
  description.concat(display_name());
  if (!is_alias()) {
    return description;
  }

  const Type* target = &get_alias_parent();
  const Type& canonical_type = canonical();
  if (target->is_invalid() || canonical_type.is_invalid()) {
    description.concat(" alias of <invalid type>"_view);
    return description;
  }

  if (!target->has_display_name()) {
    target = &canonical_type;
  }

  description.concat(" alias of "_view);
  description.concat(target->display_name());
  return description;
}

auto Ttx::Type::find_member(Perimortem::Core::View::Bytes name) const
    -> const Member* {
  // Lookup goes through canonical identity so aliases expose the same members
  // as the type they name. The first authored name wins for this simple lookup,
  // matching Layout name access. The alias still keeps its own documentation
  // and authored name outside this lookup.
  const Type& type = canonical();
  if (type.is_invalid()) {
    return nullptr;
  }

  return type.layout.find_member(name);
}

auto Ttx::Type::find_type(Perimortem::Core::View::Bytes name) const
    -> const Type* {
  // Nested type lookup is the `::` access path. It is intentionally separate
  // from member lookup so type metadata never has to be duplicated into the
  // projected layout shape. The first authored name wins for this name probe.
  const Type& type = canonical();
  if (type.is_invalid()) {
    return nullptr;
  }

  for (Count i = 0; i < type.types.get_size(); i++) {
    const Type& nested_type = type.types[i].get_type();
    if (nested_type.get_name() == name) {
      return &nested_type;
    }
  }

  return nullptr;
}

auto Ttx::Type::find_type_function(Perimortem::Core::View::Bytes name) const
    -> const Function* {
  const Type& type = canonical();
  if (type.is_invalid()) {
    return nullptr;
  }

  for (Count i = 0; i < type.type_functions.get_size(); i++) {
    if (type.type_functions[i].get_name() == name) {
      return &type.type_functions[i];
    }
  }

  return nullptr;
}

auto Ttx::Type::find_addressable_function(
    Perimortem::Core::View::Bytes name) const -> const Function* {
  const Type& type = canonical();
  if (type.is_invalid()) {
    return nullptr;
  }

  for (Count i = 0; i < type.addressable_functions.get_size(); i++) {
    if (type.addressable_functions[i].get_name() == name) {
      return &type.addressable_functions[i];
    }
  }

  return nullptr;
}

auto Ttx::Type::find_attribute(Perimortem::Core::View::Bytes key) const
    -> const Attribute* {
  for (Count i = 0; i < attributes.get_size(); i++) {
    if (attributes[i].get_key() == key) {
      return &attributes[i];
    }
  }

  return nullptr;
}

auto Ttx::Type::resolve_attribute(Perimortem::Core::View::Bytes key) const
    -> Attribute {
  if (canonical().is_invalid()) {
    return Attribute();
  }

  // Canonicalization above is only the cycle guard. The projected lookup still
  // walks every alias in authored order so an intermediate alias can override a
  // value-producing fact before the root type is reached.
  const Type* type = this;
  while (type != nullptr) {
    const Attribute* attribute = type->find_attribute(key);
    if (attribute != nullptr) {
      return *attribute;
    }

    type = type->alias_parent;
  }

  return Attribute();
}
