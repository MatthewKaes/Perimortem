// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/bibliotheca.hpp"
#include "perimortem/memory/managed/buffer.hpp"

namespace Perimortem::Memory::Dynamic {

// Record's should be used for heavy weight resources or large buffers that
// should be thread level memory managed while supporting multiple
//
// On writes if there are multiple owners the Record is forked to a copy.
// for some raw memory buffer that is untyped. There are no destructors or
// callbacks, on multiple consumers.
//
// Records are automatically pooled _by size_ with most recently touched blocks
// being the first to be reused.
//
//
// ** It's important to not put Records or Dynamic objects into Managed objects.
class Record {
 public:
  Record();
  Record(Count size);
  Record(const Record& rhs);

  // Optimized move that can avoid hitting the Bibliotheca if moving to an empty
  // Record.
  auto operator=(Record&& rhs) -> Record&;
  auto operator=(const Record& rhs) -> Record&;
  auto operator=(View::Bytes data) -> Record&;
  ~Record();

  // Clears the reservation this Record has on any resources.
  // This may or may not free the underlying resource depending on if this is
  // the only Record from the same source.
  auto clear() -> void;

  // Checks if the Record is loaded into memory.
  //
  // Getting a buffer to an invalid Record will force create one.
  // Getting a view to an invalid Record will return an empty View::Bytes.
  auto is_valid() const -> bool;

  // Returns a read-only view that's confirmed to be valid for as long
  // as the Record is valid.
  auto get_view() const -> View::Bytes;

  // Gets write access to the underlying buffer.
  // Can cause a fork if the caller isn't the sole owner of the Record.
  auto get_buffer() -> Managed::Buffer;

  // Returns the size of the current valid buffer.
  auto get_size() const -> Count;

  // Returns the full usable size of the buffer backing the Record.
  // While the details of the underlying buffer layout is undefined behavior,
  // the actual capacity value is well defined so exposing it can help with
  // a few client optimizations.
  auto get_capacity() const -> Count;

  // Returns the number of Records that point to the exact same data.
  // Returns 0 if the Record is currently invalid.
  auto outstanding_copies() const -> Count;

  // Set the size of the buffer while perserving the contents of the valid
  // range.
  //
  // Changing the size forks the Record when:
  // * If not the sole owner
  // * If the size requested is above the capacity of the original buffer.
  // * If the size requested utilize <25% of the original buffer.
  //
  // Any data beyond the original buffer size is guaranteed to be zero
  // initialized.
  auto resize(Count size) -> void { set_size(size, true); }

 private:
  auto get_data() const -> Byte*;
  auto set_size(Count size, bool copy_data) -> void;

  Allocator::Bibliotheca::Preface* reserved_block = nullptr;
};

}  // namespace Perimortem::Memory::Dynamic
