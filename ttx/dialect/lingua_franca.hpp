// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"

#include "ttx/lexical/cursor.hpp"

namespace Ttx::Dialect {

// Represents an arbitrary parser / generator and querry engine for TTX.
// The language itself is built out of an extensible number of dialect
// used for parsing and managing language extensions and features.
class LinguaFranca {
 public:
  // TODO: Should return a type erased castable type that stores an static
  // address as a unique identifier for checking if any two pointers are sourced
  // from the same type and are safely castable and wrap the destructor.
  using ParseFunction = void* (*)(Lexical::Cursor & cursor);

  // Used to make C++ register the object in the binary using dependency
  // injection.
  // TODO: The TTX compiler should just support injected registration natively
  // and as a build time manner as well using the linker.
  class Registration {
   public:
    Registration(
        Perimortem::Core::View::Bytes name,
        LinguaFranca::ParseFunction parser) {
      LinguaFranca::add(LinguaFranca(name, parser));
    }
  };

  LinguaFranca() = default;

  // Looks up a dialect by name.
  // If one can't be found an empty dialect is returned instead.
  static auto find(Perimortem::Core::View::Bytes name) -> const LinguaFranca;

  // Lists all registered dialect in this runtime.
  static auto get_dialect() -> Perimortem::Core::View::Vector<LinguaFranca>;

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }
  constexpr auto get_parser() const -> ParseFunction { return parser; };

 private:
  LinguaFranca(Perimortem::Core::View::Bytes name, ParseFunction parser)
      : name(name), parser(parser) {}

  static auto add(const LinguaFranca dialect) -> void;

  Perimortem::Core::View::Bytes name;
  ParseFunction parser = nullptr;
};

}  // namespace Ttx::Dialect
