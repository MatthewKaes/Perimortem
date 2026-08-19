// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/abi/memory/dynamic/bytes.hpp"

#include "perimortem/core/bibliotheca.hpp"

using namespace Perimortem;

auto Abi::Memory::Dynamic::Bytes::retain(Unsigned_8* data) -> void {
  if (data) {
    Core::Bibliotheca::reserve(data);
  }
}

auto Abi::Memory::Dynamic::Bytes::release(Unsigned_8* data) -> void {
  if (data) {
    Core::Bibliotheca::remit(data);
  }
}

auto Abi::Memory::Dynamic::Bytes::is_unique(Unsigned_8* data) -> Bool {
  return !data || Core::Bibliotheca::reservation_count(data) == 1;
}

extern "C" auto perimortem_dynamic_bytes_retain(Unsigned_8* data) -> void {
  Abi::Memory::Dynamic::Bytes::retain(data);
}

extern "C" auto perimortem_dynamic_bytes_release(Unsigned_8* data) -> void {
  Abi::Memory::Dynamic::Bytes::release(data);
}
