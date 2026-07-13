// Perimortem Engine
// Copyright (c) Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

namespace Tetrodotoxin::Linker::Object {

// Section is a terminal object-file payload. It names which generated block the
// bytes belong to, but it does not know which compiler or language produced
// those bytes. The linker can package these sections without depending on
// source, dialect, resolution, or compiler-local bookkeeping.
class Section {
 public:
  enum class Type : Bits_8 {
    Undefined,
    Program,
    Strings,
    ReadOnly,
  };

  constexpr Section() = default;
  constexpr Section(Type type, Perimortem::Core::View::Bytes data)
      : type(type), data(data) {}

  static constexpr auto undefined() -> Section {
    return Section(Type::Undefined, Perimortem::Core::View::Bytes());
  }

  constexpr auto get_type() const -> Type { return type; }
  constexpr auto get_data() const -> Perimortem::Core::View::Bytes {
    return data;
  }

 private:
  Type type = Type::Undefined;
  Perimortem::Core::View::Bytes data;
};

}  // namespace Tetrodotoxin::Linker::Object
