// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/object.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem;

auto Core::Object<>::create(const Descriptor& descriptor) -> Object {
  return create(descriptor, 1, descriptor.size);
}

auto Core::Object<>::create(
    const Descriptor& descriptor,
    Count count,
    Count element_size) -> Object {
  Bool valid =
      descriptor.size != 0 && descriptor.alignment != 0 &&
      (descriptor.alignment & (descriptor.alignment - 1)) == 0 &&
      descriptor.alignment <= Bibliotheca::allocation_alignment &&
      descriptor.finalize && count != 0 && element_size == descriptor.size &&
      count <= Count(-1) / element_size;
  if (!valid) {
    Diagnostics::Log::fatal(
        "Core Object received an invalid runtime descriptor."_view);
  }

  Bibliotheca::Allocation allocation =
      Bibliotheca::check_out(count * element_size);
  Bibliotheca::bind_object(allocation.ptr, &descriptor);
  return Object(allocation.ptr);
}

auto Core::Object<>::retain() const -> void {
  if (payload) {
    get_descriptor();
    Bibliotheca::reserve(payload);
  }
}

auto Core::Object<>::release() const -> void {
  if (!payload) {
    return;
  }

  const Descriptor& descriptor = get_descriptor();
  if (Bibliotheca::reservation_count(payload) == 1) {
    descriptor.finalize(payload);
  }

  Bibliotheca::remit(payload);
}

auto Core::Object<>::get_capacity() const -> Count {
  return Bibliotheca::capacity(payload);
}

auto Core::Object<>::get_reservations() const -> Count {
  return payload ? Bibliotheca::reservation_count(payload) : 0;
}

auto Core::Object<>::get_descriptor() const -> const Descriptor& {
  const void* descriptor = Bibliotheca::get_object(payload);
  if (!descriptor) {
    Diagnostics::Log::fatal(
        "Core Object received a handle without its runtime descriptor."_view);
  }

  return *static_cast<const Descriptor*>(descriptor);
}

static auto select_object(U8* payload) -> Core::Object<> {
  return Core::Object<>(payload);
}

extern "C" auto perimortem_core_object_allocate(
    const perimortem_object_descriptor* descriptor) -> U8* {
  if (!descriptor) {
    Core::Diagnostics::Log::fatal(
        "Core Object received an empty descriptor."_view);
  }

  return Core::Object<>::create(*descriptor).get_payload();
}

extern "C" auto perimortem_core_object_allocate_buffer(
    const perimortem_object_descriptor* descriptor,
    perimortem_count count,
    perimortem_count element_size) -> U8* {
  if (!descriptor) {
    Core::Diagnostics::Log::fatal(
        "Core Object received an empty descriptor."_view);
  }

  return Core::Object<>::create(*descriptor, count, element_size).get_payload();
}

extern "C" auto perimortem_core_object_retain(U8* payload) -> void {
  select_object(payload).retain();
}

extern "C" auto perimortem_core_object_release(U8* payload) -> void {
  select_object(payload).release();
}

extern "C" auto perimortem_core_object_capacity(U8* payload)
    -> perimortem_count {
  return select_object(payload).get_capacity();
}

extern "C" auto perimortem_core_object_clone(
    U8* payload,
    const perimortem_object_descriptor* descriptor,
    perimortem_count element_size) -> U8* {
  if (!descriptor || element_size == 0) {
    Core::Diagnostics::Log::fatal(
        "Core Object clone received an invalid buffer contract."_view);
  }

  Core::Object<> current(payload);
  Count capacity = current.get_capacity();
  if (capacity == 0) {
    return {};
  }

  Count count = capacity / element_size;
  Core::Object<> replacement =
      Core::Object<>::create(*descriptor, count, element_size);
  Core::Data::copy(replacement.get_payload(), payload, count * element_size);
  current.release();
  return replacement.get_payload();
}

extern "C" auto perimortem_core_object_reservations(U8* payload)
    -> perimortem_count {
  return select_object(payload).get_reservations();
}

extern "C" auto perimortem_core_object_reserve(
    U8* payload,
    const perimortem_object_descriptor* descriptor,
    perimortem_count count,
    perimortem_count element_size,
    const U8* default_value) -> U8* {
  if (!descriptor || element_size == 0 || count > Count(-1) / element_size ||
      (count != 0 && !default_value)) {
    Core::Diagnostics::Log::fatal(
        "Core Object reserve received an invalid buffer contract."_view);
  }

  Core::Object<> current(payload);
  Count current_capacity = current.get_capacity() / element_size;
  if (count <= current_capacity) {
    return payload;
  }

  Count requested = Core::Math::max(Count(count), current_capacity);
  if (requested == 0) {
    return {};
  }

  Core::Object<> replacement =
      Core::Object<>::create(*descriptor, requested, element_size);
  Count replacement_capacity = replacement.get_capacity() / element_size;
  if (current_capacity != 0) {
    Core::Data::copy(
        replacement.get_payload(), payload, current_capacity * element_size);
  }

  for (Count index = current_capacity; index < replacement_capacity; index++) {
    Core::Data::copy(
        replacement.get_payload() + index * element_size, default_value,
        element_size);
  }

  current.release();
  return replacement.get_payload();
}

extern "C" auto perimortem_core_object_finalize_trivial(U8*) -> void {}
