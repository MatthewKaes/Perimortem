// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/abi/system/terminal.hpp"

#include "perimortem/system/terminal.hpp"

using namespace Perimortem;

extern "C" auto perimortem_system_terminal_read_line()
    -> Abi::Core::Option<Abi::Memory::Dynamic::Bytes> {
  System::Terminal terminal;
  auto line = terminal.read_line();
  if (!line) {
    return Abi::Core::Option<Abi::Memory::Dynamic::Bytes>::create();
  }

  Abi::Memory::Dynamic::Bytes result = Abi::Memory::Dynamic::Bytes::create(
      line->get_view().get_data(), line->get_size(), line->get_capacity());
  perimortem_dynamic_bytes_retain(&result);
  return Abi::Core::Option<Abi::Memory::Dynamic::Bytes>::create(result);
}

extern "C" auto perimortem_system_terminal_write_line(
    Abi::Memory::Dynamic::Bytes data) -> bool {
  System::Terminal terminal;
  return bool(
      terminal.write_line(Core::View::Bytes(data.get_data(), data.get_size())));
}
