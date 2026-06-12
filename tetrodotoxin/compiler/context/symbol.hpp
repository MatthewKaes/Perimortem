// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/utility/range.hpp"

namespace Tetrodotoxin::Compiler::Context {

class Symbol {
 public:
  enum class Visability : Bits_8 {
    Local = 0,
    Global = 1,
  };

  enum class Type : Bits_8 {
    None = 0,
    Object = 1,
    Func = 2,
  };

  enum class Location {
    Program,
    Strings,
    ReadOnly,
    External,
  };

  // Creates an object
  static auto create_string(
      Perimortem::Core::View::Bytes name,
      Perimortem::Utility::Range range) -> Symbol {
    Symbol symbol;
    symbol.name = name;
    symbol.type = Type::Object;
    symbol.context = Location::Strings;

    // Visability for all TTX strings is local.
    symbol.visability = Visability::Local;
    symbol.range = range;

    return symbol;
  }

  // Creates a function out of a range in the program block.
  static auto create_func(
      Perimortem::Core::View::Bytes name,
      Visability visability) -> Symbol {
    Symbol symbol;
    symbol.name = name;
    symbol.type = Type::Func;
    symbol.context = Location::Program;
    symbol.visability = visability;
    symbol.range = {0, 0};

    return symbol;
  }

  // Creates an extrenal reference to some type.
  static auto create_extrenal(Perimortem::Core::View::Bytes name, Type type)
      -> Symbol {
    Symbol symbol;
    symbol.name = name;
    symbol.type = type;
    symbol.context = Location::External;

    // Visability must be global since we are able to access it.
    // External symbols also have no range.
    symbol.visability = Visability::Global;
    symbol.range = {0, 0};

    return symbol;
  }

  auto get_name() const -> Perimortem::Core::View::Bytes { return name; }
  auto get_context() const -> Location { return context; }
  auto get_range() const -> Perimortem::Utility::Range { return range; }
  auto get_visability() const -> Visability { return visability; }
  auto get_type() const -> Type { return type; }

  auto set_range(Perimortem::Utility::Range range) -> void {
    this->range = range;
  }

 private:
  Perimortem::Core::View::Bytes name;
  Location context;
  Perimortem::Utility::Range range;
  Visability visability;
  Type type;
};

}  // namespace Tetrodotoxin::Compiler::Context
