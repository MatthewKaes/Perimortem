// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Ttx::Lexical {
class Cursor;
}

namespace Tetrodotoxin::Library::Language::Model {

// Completion belongs to an authored declaration owner while its source
// transaction is converging. Keeping these phases outside Type prevents a
// value domain from becoming the linker, initializer, and finalization policy
// for every language that happens to use it. Because completion serves only
// the active Library source transaction, it never becomes a TTX concept or
// survives as another published graph surface.
class Completion {
 public:
  virtual ~Completion() = default;

  virtual auto link_aliases() -> Count { return 0; }
  virtual auto validate_aliases(Ttx::Lexical::Cursor&) const -> Bool {
    return True;
  }
  virtual auto link_types(Ttx::Lexical::Cursor&) -> Bool { return True; }
  virtual auto link_callable_signatures(Ttx::Lexical::Cursor&) -> Bool {
    return True;
  }
  virtual auto link_fields(Ttx::Lexical::Cursor&) -> Bool { return True; }
  virtual auto validate_layout(Ttx::Lexical::Cursor&) const -> Bool {
    return True;
  }
  virtual auto link_initializers(Ttx::Lexical::Cursor&) -> Bool { return True; }
  virtual auto link_callable_bodies(Ttx::Lexical::Cursor&) -> Bool {
    return True;
  }
  virtual auto finalize(Ttx::Lexical::Cursor&) -> Bool { return True; }

  virtual auto link_restored_types() -> Bool { return True; }
  virtual auto link_restored_callable_signatures() -> Bool { return True; }
  virtual auto link_restored_fields() -> Bool { return True; }
  virtual auto link_restored_initializers() -> Bool { return True; }
  virtual auto finalize_restored() -> Bool { return True; }
};

}  // namespace Tetrodotoxin::Library::Language::Model
