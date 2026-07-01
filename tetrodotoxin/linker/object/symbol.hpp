// Perimortem Engine
// Copyright (c) Matt Kaes

#pragma once

#include "perimortem/core/data.hpp"
#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/view/bytes.hpp"

#include "perimortem/utility/range.hpp"

namespace Tetrodotoxin::Linker::Object {

// Symbol is the linker-facing name for a generated function or data range.
// It is intentionally part of the linker model because the only stable
// consumer today is the object packaging layer. A future compiler can emit this
// payload, but the payload should stay owned by the linker until a broader
// object-module concept is earned by more than one owner.
class Symbol {
 public:
  enum class Visibility : Bits_8 {
    Local = 0,
    Global = 1,
  };

  enum class Type : Bits_8 {
    None = 0,
    Object = 1,
    Function = 2,
  };

  enum class Location : Bits_8 {
    Program,
    Strings,
    ReadOnly,
    External,
  };

  static constexpr auto create_string(
      Perimortem::Core::View::Bytes name,
      Perimortem::Utility::Range range) -> Symbol {
    Symbol symbol;
    symbol.name = name;
    symbol.type = Type::Object;
    symbol.location = Location::Strings;
    symbol.visibility = Visibility::Local;
    symbol.range = range;
    return symbol;
  }

  static constexpr auto create_function(
      Perimortem::Core::View::Bytes name,
      Visibility visibility) -> Symbol {
    Symbol symbol;
    symbol.name = name;
    symbol.type = Type::Function;
    symbol.location = Location::Program;
    symbol.visibility = visibility;
    symbol.range = {0, 0};
    return symbol;
  }

  static constexpr auto create_read_only(
      Perimortem::Core::View::Bytes name,
      Perimortem::Utility::Range range,
      Visibility visibility = Visibility::Global) -> Symbol {
    Symbol symbol;
    symbol.name = name;
    symbol.type = Type::Object;
    symbol.location = Location::ReadOnly;
    symbol.visibility = visibility;
    symbol.range = range;
    return symbol;
  }

  static constexpr auto create_external(
      Perimortem::Core::View::Bytes name,
      Type type) -> Symbol {
    Symbol symbol;
    symbol.name = name;
    symbol.type = type;
    symbol.location = Location::External;
    symbol.visibility = Visibility::Global;
    symbol.range = {0, 0};
    return symbol;
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name;
  }
  constexpr auto get_location() const -> Location { return location; }
  constexpr auto get_range() const -> Perimortem::Utility::Range {
    return range;
  }
  constexpr auto get_visibility() const -> Visibility { return visibility; }
  constexpr auto get_type() const -> Type { return type; }

  constexpr auto set_range(Perimortem::Utility::Range range) -> void {
    this->range = range;
  }

 private:
  constexpr Symbol() = default;

  Perimortem::Core::View::Bytes name;
  Location location = Location::External;
  Perimortem::Utility::Range range;
  Visibility visibility = Visibility::Local;
  Type type = Type::None;
};

}  // namespace Tetrodotoxin::Linker::Object
