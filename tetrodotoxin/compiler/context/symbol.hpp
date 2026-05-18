// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/utility/range.hpp"

namespace Tetrodotoxin::Compiler::Context {

class Symbol {
  using View = Perimortem::Core::View::Bytes;
  using Range = Perimortem::Utility::Range;

 public:
  enum class Visability : Byte {
    Local = 0,
    Global = 1,
  };

  enum class Type : Byte {
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
  static auto create_string(View name, Range range) -> Symbol {
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
  static auto create_func(View name, Visability visability) -> Symbol {
    Symbol symbol;
    symbol.name = name;
    symbol.type = Type::Func;
    symbol.context = Location::Program;
    symbol.visability = visability;
    symbol.range = {0, 0};

    return symbol;
  }

  // Creates an extrenal reference to some type.
  static auto create_extrenal(View name, Type type) -> Symbol {
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

  auto get_name() const -> View { return name; }
  auto get_context() const -> Location { return context; }
  auto get_range() const -> Range { return range; }
  auto get_visability() const -> Visability { return visability; }
  auto get_type() const -> Type { return type; }

  auto set_range(Range range) -> void { this->range = range; }

 private:
  View name;
  Location context;
  Range range;
  Visability visability;
  Type type;
};

}  // namespace Tetrodotoxin::Compiler::Context
