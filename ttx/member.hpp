// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/attribute.hpp"
#include "ttx/documentation.hpp"

namespace Ttx {

class Type;

// Member is the shared entry view used by every shaped list.
//
// Aggregate members, layout fields, function parameters, function returns, and
// repack results all reduce to ordered entries that may reference types,
// defaults, documentation, and sometimes local names. Names are present when
// authored syntax or a receiving boundary supplies them. Swizzle and slice
// results normally use the same entry view without carrying names forward.
// Sharing this view keeps the model from inventing separate node kinds for the
// same layout question.
//
// A member references a type but does not own it. The referenced type answers
// identity and function dispatch questions. The member answers local shape
// questions such as whether the entry is named, whether it can be omitted
// during construction, which documentation was written for this use, and
// which immutable attributes qualify this exact entry.
class Member {
 public:
  constexpr Member() = delete;
  constexpr Member(
      Perimortem::Core::View::Bytes name,
      const Type& type,
      Bool defaulted = False,
      Documentation documentation = Documentation(),
      Perimortem::Core::View::Vector<Attribute> attributes = {})
      : name(name),
        type(&type),
        defaulted(defaulted),
        documentation(documentation),
        attributes(attributes) {}

  constexpr Member(
      Perimortem::Core::View::Bytes name,
      const Type& type,
      Documentation documentation,
      Bool defaulted = False,
      Perimortem::Core::View::Vector<Attribute> attributes = {})
      : Member(name, type, defaulted, documentation, attributes) {}

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }

  constexpr auto get_type() const -> const Type& { return *type; }
  // Compares the stored identity without beginning or observing the Type's
  // lifetime. Recursive builders use this only while validating an edge to a
  // reserved Type. Semantic consumers should query get_type() after the graph
  // has been completed.
  constexpr auto references(const Type* candidate) const -> Bool {
    return type == candidate;
  }
  constexpr auto get_documentation() const -> Documentation {
    return documentation;
  }

  constexpr auto get_attributes() const
      -> Perimortem::Core::View::Vector<Attribute> {
    return attributes;
  }

  constexpr auto is_named() const -> Bool { return !name.is_empty(); }
  constexpr auto is_defaulted() const -> Bool { return defaulted; }

  // Member equivalence is the reusable type-slot check used by layout
  // algorithms.
  //
  // It ignores documentation, attributes, defaults, and member names.
  // Documentation and attributes are metadata. Defaults are construction
  // policy owned by the target layout. Names are interpreted by Layout before
  // it asks two members whether their referenced types are equivalent.
  //
  // That split lets a named layout compare `.x` to `.x`, while a positional
  // layout can still compare entry 0 to entry 0 without inventing a separate
  // member kind.
  auto equivalent_to(const Member& other) const -> Bool;

 private:
  constexpr Member(
      Perimortem::Core::View::Bytes name,
      const Type* type,
      Bool defaulted,
      Documentation documentation,
      Perimortem::Core::View::Vector<Attribute> attributes)
      : name(name),
        type(type),
        defaulted(defaulted),
        documentation(documentation),
        attributes(attributes) {}

  Perimortem::Core::View::Bytes name;
  const Type* type;
  Bool defaulted = False;
  Documentation documentation;
  Perimortem::Core::View::Vector<Attribute> attributes;
};

}  // namespace Ttx
