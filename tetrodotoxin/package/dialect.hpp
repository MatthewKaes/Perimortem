// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Package {

// Dialects are used to interpret TTX lexical streams in order to convert them
// into a usable TTX graph.
//
// A Tetrodotoxin toolchain consist of multiple dialects in order to construct
// it's full language support. After the initial Tetrodotoxin header is parsed
// the rest of the stream is passed to the target dialect if registered.
//
// Dialects are stateful for the duration
class Dialect : public ::Tetrodotoxin::Language::Dialect {
 public:
  // Interpret takes in the arena domain and the cursor pointing to the start of
  // the token stream to translate into TTX. The passed in arena is then passed
  // to the Source for long term storage if the source can be successfully
  // determined.
  auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& doc,
      Ttx::Concept::Abstract& registry)
      -> Perimortem::Utility::Option<Monograph&> override;
};

}  // namespace Tetrodotoxin::Package
