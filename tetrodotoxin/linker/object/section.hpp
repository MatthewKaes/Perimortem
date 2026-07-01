// Perimortem Engine
// Copyright (c) Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/view/bytes.hpp"

namespace Tetrodotoxin::Linker::Object {

// Section is a terminal object-file payload. It names which generated block the
// bytes belong to, but it does not know which compiler or language produced
// those bytes. The linker can package these sections without depending on source,
// dialects, resolution, or compiler-local bookkeeping.
class Section {
 public:
  enum class Type : Bits_8 {
    Invalid,
    Program,
    Strings,
    ReadOnly,
  };

  constexpr Section() = default;
  constexpr Section(Type type, Perimortem::Core::View::Bytes data)
      : type(type), data(data) {}

  constexpr auto get_type() const -> Type { return type; }
  constexpr auto get_data() const -> Perimortem::Core::View::Bytes {
    return data;
  }

 private:
  Type type = Type::Invalid;
  Perimortem::Core::View::Bytes data;
};

}  // namespace Tetrodotoxin::Linker::Object
