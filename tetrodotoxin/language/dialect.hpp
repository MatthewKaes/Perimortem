// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/object.hpp"

#include "perimortem/utility/option.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Language {

// One Dialect instance remains installed for the Workspace lifetime so sources
// of the same language can share real semantic state. The universal header
// selects that instance before the remaining Cursor is handed to its grammar.
class Dialect {
 public:
  // A Monograph is the retained semantic root produced by one source or
  // restored payload. Keeping the host and Arena explicit lets concrete graphs
  // share Workspace state without introducing a second source model.
  class Monograph : public Ttx::Concept::Abstract {
   public:
    virtual ~Monograph() = 0;

    Monograph(
        Perimortem::Memory::Allocator::Arena& domain,
        const Ttx::Concept::Documentation& documentation,
        Dialect& host)
        : domain(domain), documentation(documentation), host(host) {}

    constexpr auto get_documentation() const
        -> const Ttx::Concept::Documentation& override {
      return documentation;
    };

    // Completion waits until every source and restored dependency has joined
    // the graph because earlier execution could reject a valid forward edge. A
    // failure returns to the Workspace that knows which retained input was
    // being completed. Concrete owners log any graph context that would
    // otherwise be lost before returning.
    virtual auto post_pass() -> Bool;

   protected:
    // Concrete facts remain in the same lifetime domain as their Monograph so
    // graph edges never outlive their storage.
    Perimortem::Memory::Allocator::Arena& domain;

    // The opening Documentation remains attached to the semantic root because
    // later owners may need it after the parser transaction has ended.
    const Ttx::Concept::Documentation& documentation;

    // The installed host outlives every Monograph and carries shared Dialect
    // state needed during completion and persistence.
    Dialect& host;
  };

  constexpr Dialect(Ttx::Concept::Abstract& registry) : registry(registry) {}
  virtual ~Dialect() = 0;

  // The caller chooses the Arena that defines the returned graph lifetime.
  // Interpret borrows Cursor input under that same lifetime contract and
  // receives the exact source local scope separately from the Workspace wide
  // registry retained by this Dialect.
  virtual auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& doc,
      Ttx::Concept::Abstract& interpretation_context)
      -> Perimortem::Utility::Option<Monograph&> = 0;

  // Encode only the durable facts owned by this Dialect. An engaged empty byte
  // value is a successful empty payload while no value reports unsupported or
  // failed encoding.
  virtual auto encode(const Monograph& monograph) const
      -> Perimortem::Utility::Option<Perimortem::Memory::Dynamic::Bytes>;

  // Restore one opaque payload into the importing Workspace Arena. A
  // successful result and every durable fact it exposes must outlive the input
  // byte view.
  virtual auto restore(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes payload)
      -> Perimortem::Utility::Option<Monograph&>;

 protected:
  // The Workspace registry is shared so concrete Dialects resolve cross
  // language edges against the same semantic island.
  Ttx::Concept::Abstract& registry;
};

}  // namespace Tetrodotoxin::Language
