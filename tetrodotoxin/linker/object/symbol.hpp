// Perimortem Engine
// Copyright (c) Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/utility/range.hpp"

namespace Tetrodotoxin::Linker::Object {

// Symbol is the linker facing name for a generated function or data range. Its
// definition state remains explicit because section zero is an object format
// reservation, not a substitute for the generator's construction decision.
class Symbol {
 public:
  enum class Visibility : Unsigned_8 {
    Local,
    Global,
  };

  enum class Type : Unsigned_8 {
    None,
    Object,
    Function,
  };

  static auto create_string(
      Perimortem::Core::View::Bytes name,
      Unsigned_16 section_index,
      Perimortem::Utility::Range range) -> Symbol {
    Symbol symbol(name);
    symbol.definition = Definition::Defined;
    symbol.type = Type::Object;
    symbol.section_index = section_index;
    symbol.visibility = Visibility::Local;
    symbol.range = range;
    return symbol;
  }

  static auto create_function(
      Perimortem::Core::View::Bytes name,
      Unsigned_16 section_index,
      Visibility visibility) -> Symbol {
    Symbol symbol(name);
    symbol.definition = Definition::Defined;
    symbol.type = Type::Function;
    symbol.section_index = section_index;
    symbol.visibility = visibility;
    symbol.range = {0, 0};
    return symbol;
  }

  static auto create_read_only(
      Perimortem::Core::View::Bytes name,
      Unsigned_16 section_index,
      Perimortem::Utility::Range range,
      Visibility visibility = Visibility::Global) -> Symbol {
    Symbol symbol(name);
    symbol.definition = Definition::Defined;
    symbol.type = Type::Object;
    symbol.section_index = section_index;
    symbol.visibility = visibility;
    symbol.range = range;
    return symbol;
  }

  static auto create_undefined(Perimortem::Core::View::Bytes name, Type type)
      -> Symbol {
    Symbol symbol(name);
    symbol.definition = Definition::Undefined;
    symbol.type = type;
    symbol.section_index = 0;
    symbol.visibility = Visibility::Global;
    symbol.range = {0, 0};
    return symbol;
  }

  constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
    return name.get_view();
  }

  auto get_section_index() const -> Unsigned_16 { return section_index; }
  auto get_range() const -> Perimortem::Utility::Range { return range; }

  auto get_visibility() const -> Visibility { return visibility; }
  auto get_type() const -> Type { return type; }
  auto is_defined() const -> Bool { return definition == Definition::Defined; }
  auto is_undefined() const -> Bool {
    return definition == Definition::Undefined;
  }

  auto set_range(Perimortem::Utility::Range range) -> void {
    this->range = range;
  }

 private:
  enum class Definition : Unsigned_8 {
    Undefined,
    Defined,
  };

  Symbol(Perimortem::Core::View::Bytes name) : name(name) {}

  Perimortem::Memory::Dynamic::Bytes name;
  Unsigned_16 section_index = 0;
  Perimortem::Utility::Range range;
  Visibility visibility = Visibility::Local;
  Type type = Type::None;
  Definition definition = Definition::Undefined;
};

}  // namespace Tetrodotoxin::Linker::Object
