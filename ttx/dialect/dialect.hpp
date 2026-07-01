// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"

#include "ttx/parse/cursor.hpp"

namespace Ttx::Dialect {

// A dialect is the parser that owns the source after the ordered TTX envelope.
//
// The root TTX parser only understands documentation, the required dialect
// declaration, and imports. Everything after that belongs to the dialect named
// by the envelope.
class Dialect {
 public:
  // Registry is the process-wide dialect table used by source dispatch.
  //
  // The table stores dialect object addresses and their registered names
  // instead of a closed enum. That keeps new dialects extensible without
  // duplicating the dialect list in the lexer, parser, resolver, or compiler.
  // It owns only the table of non-owning dialect pointers. The dialect objects
  // themselves still belong to the package or static registration site that
  // created them.
  class Registry {
   public:
    class Registration {
     public:
      explicit Registration(const Dialect& dialect);
    };

    static auto add(const Dialect& dialect) -> void;
    static auto find(Perimortem::Core::View::Bytes name) -> const Dialect*;
    static auto get_dialects()
        -> Perimortem::Core::View::Vector<const Dialect*>;

   private:
    static auto dialects()
        -> Perimortem::Core::Static::Vector<const Dialect*, 64>&;
    static auto dialect_count() -> Count&;
  };

  virtual ~Dialect() = default;

  virtual auto get_name() const -> Perimortem::Core::View::Bytes = 0;
  virtual auto parse(Parse::Cursor& cursor) const -> void = 0;
};

}  // namespace Ttx::Dialect
