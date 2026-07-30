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
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Language {

// Dialects are used to interpret TTX lexical streams in order to convert them
// into a usable TTX graph.
//
// A Tetrodotoxin toolchain consist of multiple dialects in order to construct
// it's full language support. After the initial Tetrodotoxin header is parsed
// the rest of the stream is passed to the target dialect if registered.
//
// Dialects are stateful for the duration
class Dialect {
 public:
  // Monograph is an independent subgraph of the larger TTX data graph that owns
  // its memory domain for its entire subtree context.
  //
  // Each custom Tetrodotoxin Dialect requires a distinct format.
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

    // Complete durable semantic facts after every source and restored
    // dependency has joined the shared graph.
    virtual auto post_pass(Ttx::Lexical::Errors& errors) -> void;

   protected:
    // The arena space which contains the sub portion of the
    Perimortem::Memory::Allocator::Arena& domain;

    // Documentation is provided by the wrapping dialect context as required by
    // the language spec, but individual dialects may choose to extend or alter
    // source provided information.
    const Ttx::Concept::Documentation& documentation;

    // Source formats can perform operations on their parent.
    Dialect& host;
  };

  constexpr Dialect(Ttx::Concept::Abstract& registry) : registry(registry) {}
  virtual ~Dialect() = 0;

  // Interpret takes in the domain arean where it will create the subgraph.
  // For caching it's useful to pass in a subgraph specific arena, but for one
  // shots its usually more performant to just reuse the parent arena since the
  // source tree is processed in immediate mode rather than retained mode.
  virtual auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& doc,
      Ttx::Concept::Abstract& registry)
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
  // The registry that was provide to resolve cross dialect queries.
  Ttx::Concept::Abstract& registry;
};

}  // namespace Tetrodotoxin::Language
