// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;

Dynamic::Bytes::Bytes(Count reserved_capacity) {
  if (reserved_capacity == 0) {
    return;
  }

  auto alloc = Bibliotheca::check_out(reserved_capacity);
  replace(alloc.ptr, 0, alloc.capacity);
}

Dynamic::Bytes::Bytes(const Core::View::Bytes view) {
  if (view.get_size() == 0) {
    return;
  }

  auto alloc = Bibliotheca::check_out(view.get_size());
  memcpy(alloc.ptr, view.get_data(), view.get_size());
  replace(alloc.ptr, view.get_size(), alloc.capacity);
}

Dynamic::Bytes::Bytes(const Bytes& rhs)
    : data(rhs.data), size(rhs.size), capacity(rhs.capacity) {
  if (data) {
    Bibliotheca::reserve(data);
  }
}

Dynamic::Bytes::Bytes(Bytes&& rhs)
    : data(rhs.data), size(rhs.size), capacity(rhs.capacity) {
  rhs.replace(nullptr, 0, 0);
}

auto Dynamic::Bytes::operator=(Core::View::Bytes view) -> Bytes& {
  proxy(view);
  return *this;
}

auto Dynamic::Bytes::operator=(const Bytes& rhs) -> Bytes& {
  if (this == &rhs) {
    return *this;
  }

  if (rhs.data) {
    Bibliotheca::reserve(rhs.data);
  }

  release();
  replace(rhs.data, rhs.size, rhs.capacity);
  return *this;
}

auto Dynamic::Bytes::operator=(Bytes&& rhs) -> Bytes& {
  if (this == &rhs) {
    return *this;
  }

  release();
  replace(rhs.data, rhs.size, rhs.capacity);
  rhs.replace(nullptr, 0, 0);
  return *this;
}

Dynamic::Bytes::~Bytes() {
  release();
}

auto Dynamic::Bytes::append(Unsigned_8 byte) -> void {
  Count size = get_size();
  prepare_write(size + 1);
  data[size] = byte;
  this->size = size + 1;
}

auto Dynamic::Bytes::append(Unsigned_8 byte, Count amount) -> void {
  if (amount == 0) {
    return;
  }

  Count size = get_size();
  prepare_write(size + amount);
  Data::set(data + size, byte, amount);
  this->size = size + amount;
}

auto Dynamic::Bytes::concat(Core::View::Bytes view) -> void {
  if (view.is_empty()) {
    return;
  }

  if (view.get_size() > Count(-1) - get_size()) {
    Diagnostics::Log::fatal(
        "Dynamic Bytes concatenation exceeds the addressable size."_view);
  }

  Count size = get_size();
  Count required_size = size + view.get_size();
  Bool can_write = (!data || Bibliotheca::reservation_count(data) == 1) &&
                   required_size <= get_capacity();
  if (can_write) {
    Data::copy(data + size, view.get_data(), view.get_size());
    this->size = required_size;
    return;
  }

  Count requested_capacity = Core::Math::max(required_size, get_capacity());
  Bibliotheca::Allocation allocation =
      Bibliotheca::check_out(requested_capacity);
  if (size != 0) {
    Data::copy(allocation.ptr, data, size);
  }
  Data::copy(allocation.ptr + size, view.get_data(), view.get_size());
  release();
  replace(allocation.ptr, required_size, allocation.capacity);
}

auto Dynamic::Bytes::get_access() -> Core::Access::Bytes {
  prepare_write(get_size());
  return Core::Access::Bytes(data, get_size());
}

auto Dynamic::Bytes::prepare_write(Count required_capacity) -> void {
  Bool can_write = (!data || Bibliotheca::reservation_count(data) == 1) &&
                   required_capacity <= get_capacity();
  if (can_write) {
    return;
  }

  Count requested_capacity = Core::Math::max(required_capacity, get_capacity());
  if (requested_capacity == 0) {
    return;
  }

  Bibliotheca::Allocation allocation =
      Bibliotheca::check_out(requested_capacity);
  Count size = get_size();
  if (size != 0) {
    Data::copy(allocation.ptr, data, size);
  }
  release();
  replace(allocation.ptr, size, allocation.capacity);
}

auto Dynamic::Bytes::proxy(Core::View::Bytes view) -> void {
  if (view.is_empty()) {
    reset();
    return;
  }

  Bibliotheca::Allocation allocation = Bibliotheca::check_out(view.get_size());
  Data::copy(allocation.ptr, view.get_data(), view.get_size());
  release();
  replace(allocation.ptr, view.get_size(), allocation.capacity);
}

auto Dynamic::Bytes::set(Unsigned_8 target) -> void {
  if (is_empty()) {
    return;
  }

  prepare_write(get_size());
  Data::set(data, target, get_size());
}

auto Dynamic::Bytes::convert(Unsigned_8 source, Unsigned_8 target) -> void {
  prepare_write(get_size());
  for (Count i = 0; i < get_size(); i++) {
    if (data[i] == source) {
      data[i] = target;
    }
  }
}

auto Dynamic::Bytes::slice(Count start, Count size) const -> Core::View::Bytes {
  if (start >= get_size()) {
    return Core::View::Bytes();
  }

  return Core::View::Bytes(data + start, Math::min(size, get_size() - start));
}

auto Dynamic::Bytes::resize(Count new_size) -> void {
  ensure_capacity(new_size);
  size = new_size;
}

auto Dynamic::Bytes::forgetful_resize(Count required_size) -> void {
  // Get the capacity bounds and check if we need a realloc.
  // If the block fits in the current Bibliotheca archive then reuse it.
  // If the block size requires at least one step up or step down then request a
  // new block.
  Count capacity = get_capacity();
  Bool reusable = required_size <= capacity && required_size > (capacity >> 1);
  if (reusable && (!data || Bibliotheca::reservation_count(data) == 1)) {
    size = required_size;
    return;
  }

  if (required_size == 0) {
    reset();
    return;
  }

  auto alloc = Bibliotheca::check_out(required_size);
  release();
  replace(alloc.ptr, required_size, alloc.capacity);
}

auto Dynamic::Bytes::shrink(Count bytes_to_remove) -> void {
  Count size = get_size();
  if (bytes_to_remove >= size) {
    clear();
    return;
  }

  if (bytes_to_remove == 0) {
    return;
  }

  prepare_write(size);
  Count remaining = size - bytes_to_remove;
  memmove(data, data + bytes_to_remove, remaining);
  this->size = remaining;
}

auto Dynamic::Bytes::operator[](Count index) const -> Unsigned_8 {
  return get_view()[index];
}

auto Dynamic::Bytes::at(Count index) const -> Unsigned_8 {
  return get_view()[index];
}

auto Dynamic::Bytes::clear() -> void {
  size = 0;
}

auto Dynamic::Bytes::release() -> void {
  if (data) {
    Bibliotheca::remit(data);
  }

  replace(nullptr, 0, 0);
}

auto Dynamic::Bytes::reset() -> void {
  release();
}

auto Dynamic::Bytes::ensure_capacity(Count required_size) -> void {
  // Check if we can already fit required buffer.
  if (required_size <= get_capacity()) {
    return;
  }

  // Since the current block doesn't fit in the current archive fetch and
  // transfer to a new block.
  auto alloc = Bibliotheca::check_out(required_size);
  if (data) {
    memcpy(alloc.ptr, data, get_size());
  }

  Count size = get_size();
  release();
  replace(alloc.ptr, size, alloc.capacity);
}

auto Dynamic::Bytes::replace(
    Unsigned_8* selected_data,
    Count selected_size,
    Count selected_capacity) -> void {
  data = selected_data;
  size = selected_size;
  capacity = selected_capacity;
}
