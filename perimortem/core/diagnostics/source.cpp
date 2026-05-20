// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/diagnostics/source.hpp"

using namespace Perimortem::Core::Diagnostics;

auto Source::get_line() const -> Count {
  return abi.line;
}

auto Source::get_column() const -> Count {
  return abi.column;
}

// The ABI gives us const char* — cast to Byte* here, outside consteval,
// where reinterpret_cast is legal.
auto Source::get_file() const -> Core::View::Bytes {
  return Core::View::Bytes(
      reinterpret_cast<const Byte*>(abi.file), abi.file_size);
}

auto Source::get_function() const -> Core::View::Bytes {
  return Core::View::Bytes(
      reinterpret_cast<const Byte*>(abi.func), abi.func_size);
}
