// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Model::Source {

// Dialect is the durable named contract for one Tetrodotoxin evaluation
// building block. Concrete token-consuming implementations belong to
// Tetrodotoxin::Interpreter, while Source and Dependency retain this Model
// identity for lookup, formatting, and regeneration.
//
// Evaluation consumes authored token bytecode and constructs real TTX facts
// directly. The caller supplies the Abstract context in which those facts are
// interpreted and owns retaining the returned result. A Dialect can hand the
// remaining Cursor and enriched context to another named Dialect without
// producing an AST, registry record, or replacement IR. Dialect does not imply
// a compiler phase or require inputs to be flattened into one private
// representation.
//
// Definition continuations are ordinary Interpreter compile time classes. They
// do not inherit Dialect or Abstract merely to participate in a fixed grammar
// map.
class Dialect : public Ttx::Concept::Abstract {
 public:
  using ContractOwner = Dialect;
  static constexpr Perimortem::System::Uuid contract_id{
    0x3a2ed9ff58564c04,
    0x84a6010d77e46898,
  };

  auto implements(Perimortem::System::Uuid requested) const -> Bool override;

  auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Ttx::Concept::Abstract& override;

  // Dialects describe evaluation policy rather than authored semantic output.
  // Documentation belongs to the Abstracts they construct.
  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

  virtual auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& context) const
      -> const Ttx::Concept::Abstract& = 0;
};

}  // namespace Tetrodotoxin::Model::Source
