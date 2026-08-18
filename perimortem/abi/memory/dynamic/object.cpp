// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/abi/memory/dynamic/object.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"

#include "perimortem/abi/core/cleanup.hpp"

using namespace Perimortem;

static_assert(
    sizeof(Abi::Memory::Dynamic::Object::Finalizer) <=
    Perimortem::Core::Bibliotheca::legal_underwrite_size);

static auto get_cleanup_inventory() -> Abi::Core::Cleanup& {
  thread_local Abi::Core::Cleanup inventory;
  return inventory;
}

auto Abi::Memory::Dynamic::Object::allocate(Count bytes, Finalizer finalizer)
    -> Unsigned_8* {
  Perimortem::Core::Bibliotheca::Allocation allocation =
      Perimortem::Core::Bibliotheca::check_out(bytes);
  Perimortem::Core::Data::copy(allocation.ptr - sizeof(Finalizer), finalizer);
  return allocation.ptr;
}

auto Abi::Memory::Dynamic::Object::retain(Unsigned_8* payload) -> void {
  Perimortem::Core::Bibliotheca::reserve(payload);
}

auto Abi::Memory::Dynamic::Object::release(Unsigned_8* payload) -> void {
  if (Perimortem::Core::Bibliotheca::reservation_count(payload) == 1) {
    Finalizer finalizer;
    memcpy(&finalizer, payload - sizeof(Finalizer), sizeof(Finalizer));
    finalizer(payload);
  }
  Perimortem::Core::Bibliotheca::remit(payload);
}

auto Abi::Memory::Dynamic::Object::register_cleanup(Cleanup cleanup) -> void {
  // C++ destroys thread local objects in reverse construction order. Touching
  // Bibliotheca first makes Cleanup run every Object destructor before the
  // Librarian reclaims this worker's pages.
  Perimortem::Core::Bibliotheca::Allocation order =
      Perimortem::Core::Bibliotheca::check_out(1);
  Perimortem::Core::Bibliotheca::remit(order.ptr);
  get_cleanup_inventory().insert(cleanup);
}

extern "C" auto perimortem_dynamic_object_allocate(
    Count bytes,
    Abi::Memory::Dynamic::Object::Finalizer finalizer) -> Unsigned_8* {
  return Abi::Memory::Dynamic::Object::allocate(bytes, finalizer);
}

extern "C" auto perimortem_dynamic_object_retain(Unsigned_8* payload) -> void {
  Abi::Memory::Dynamic::Object::retain(payload);
}

extern "C" auto perimortem_dynamic_object_release(Unsigned_8* payload) -> void {
  Abi::Memory::Dynamic::Object::release(payload);
}

extern "C" auto perimortem_dynamic_object_register_cleanup(
    Abi::Memory::Dynamic::Object::Cleanup cleanup) -> void {
  Abi::Memory::Dynamic::Object::register_cleanup(cleanup);
}
