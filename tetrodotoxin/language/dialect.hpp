// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/object.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language {

// One Dialect instance remains installed for the Workspace lifetime so sources
// of the same language can share real semantic state. The universal header
// selects that instance before the remaining Cursor is handed to its grammar.
class Dialect {
 public:
  constexpr Dialect() = default;
  virtual ~Dialect() = 0;

  // The caller chooses the Arena that defines the returned graph lifetime.
  // Interpret borrows Cursor input under that same lifetime contract and
  // receives the exact owner supplied context separately. Each concrete
  // Dialect decides what that context means rather than inheriting one
  // universal source scope.
  virtual auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& doc,
      Ttx::Concept::Abstract& interpretation_context)
      -> Perimortem::Core::Option<Monograph&> = 0;

  // Encode only the durable facts owned by this Dialect. An engaged empty byte
  // value is a successful empty payload while no value reports unsupported or
  // failed encoding.
  virtual auto encode(const Monograph& monograph) const
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>;

  // Restore one opaque payload into the importing Workspace Arena. A
  // successful result and every durable fact it exposes must outlive the input
  // byte view.
  virtual auto restore(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes payload)
      -> Perimortem::Core::Option<Monograph&>;
};

}  // namespace Tetrodotoxin::Language
