// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "ttx/dialect/source/source.hpp"

namespace Tetrodotoxin::Resolution::Source {

// Stable source entry owned by the resolution cache.
//
// A record owns the storage whose lifetime is exactly one resolved source file:
// normalized source path, source bytes, parse arena, and the published TTX
// source envelope. Cache owns lookup keys and dependency edges. Resolver owns
// traversal, parsing, and error forwarding.
class Record {
 public:
  static auto create(
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source_text,
      Bool private_source = False) -> Record& {
    auto allocation = Perimortem::Core::Bibliotheca::check_out(sizeof(Record));
    return *new (allocation.ptr)
        Record(source_path, source_text, private_source);
  }

  static auto destroy(Record& record) -> void {
    record.~Record();
    Perimortem::Core::Bibliotheca::remit(
        Perimortem::Core::Data::cast<Bits_8>(&record));
  }

  Record(const Record&) = delete;
  auto operator=(const Record&) -> Record& = delete;

  auto set_source(Ttx::Dialect::Source::Source& source) -> void {
    source_info = &source;
  }

  auto publish(
      void* dialect_body,
      Perimortem::Core::View::Bytes import_name) -> void {
    this->dialect_body = dialect_body;
    if (!(this->import_name == import_name)) {
      this->import_name = import_name;
    }
  }

  constexpr auto get_arena() -> Perimortem::Memory::Allocator::Arena& {
    return arena;
  }
  constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return source_path;
  }
  constexpr auto get_import_name() const -> Perimortem::Core::View::Bytes {
    return import_name;
  }
  constexpr auto get_content() const -> Perimortem::Core::View::Bytes {
    return source_text;
  }
  constexpr auto get_source() -> Ttx::Dialect::Source::Source& {
    return *source_info;
  }
  constexpr auto get_source() const -> const Ttx::Dialect::Source::Source& {
    return *source_info;
  }
  constexpr auto get_dialect_body() const -> const void* {
    return dialect_body;
  }
  constexpr auto is_private() const -> Bool { return private_source; }

 private:
  Record(
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source_text,
      Bool private_source)
      : source_path(source_path),
        import_name(source_path),
        source_text(source_text),
        private_source(private_source) {}

  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Dynamic::Bytes source_path;
  Perimortem::Memory::Dynamic::Bytes import_name;
  Perimortem::Memory::Dynamic::Bytes source_text;
  Ttx::Dialect::Source::Source* source_info = nullptr;
  void* dialect_body = nullptr;
  Bool private_source = False;
};

}  // namespace Tetrodotoxin::Resolution::Source
