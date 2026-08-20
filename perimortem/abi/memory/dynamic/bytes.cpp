// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/abi/memory/dynamic/bytes.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem;

extern "C" auto perimortem_dynamic_bytes_retain(
    const Abi::Memory::Dynamic::Bytes* value) -> void {
  if (value && value->get_data()) {
    Core::Bibliotheca::reserve(const_cast<Unsigned_8*>(value->get_data()));
  }
}

extern "C" auto perimortem_dynamic_bytes_release(
    const Abi::Memory::Dynamic::Bytes* value) -> void {
  if (value && value->get_data()) {
    Core::Bibliotheca::remit(const_cast<Unsigned_8*>(value->get_data()));
  }
}

extern "C" auto perimortem_dynamic_bytes_concat(
    Memory::Dynamic::Bytes* output,
    Core::View::Bytes left,
    Core::View::Bytes right) -> void {
  if (!output) {
    Core::Diagnostics::Log::fatal(
        "Dynamic Bytes ABI received an empty result address."_view);
  }

  new (output) Memory::Dynamic::Bytes(left);
  output->concat(right);
}
