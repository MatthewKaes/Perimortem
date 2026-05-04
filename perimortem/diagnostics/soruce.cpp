// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/diagnostics/source.hpp"

using namespace Perimortem::Diagnostics;

auto Source::get_line() const -> Count {
  return abi.line;
}

auto Source::get_column() const -> Count {
  return abi.column;
}

auto Source::get_file() const -> Core::View::Bytes {
  return Core::View::Bytes(reinterpret_cast<const Byte*>(abi.file),
                           abi.file_size);
}


auto Source::get_function() const -> Core::View::Bytes {
  return Core::View::Bytes(reinterpret_cast<const Byte*>(abi.func),
                           abi.func_size);
}