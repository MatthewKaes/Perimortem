// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/abi/system/terminal.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/terminal.hpp"

using namespace Perimortem;

extern "C" auto perimortem_system_terminal_read_line(
    Core::Option<Memory::Dynamic::Bytes>* output) -> void {
  if (!output) {
    Core::Diagnostics::Log::fatal(
        "Terminal ABI received an empty result address."_view);
  }

  System::Terminal terminal;
  new (output) Core::Option<Memory::Dynamic::Bytes>(terminal.read_line());
}

extern "C" auto perimortem_system_terminal_write_line(
    Abi::Memory::Dynamic::Bytes data) -> bool {
  System::Terminal terminal;
  return bool(
      terminal.write_line(Core::View::Bytes(data.get_data(), data.get_size())));
}
