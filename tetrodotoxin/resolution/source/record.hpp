// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/isa/boot/boot.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Resolution::Source {

// Stable source entry owned by the resolution cache.
//
// A record owns the storage whose lifetime is exactly one resolved source file:
// normalized source path, source bytes, evaluation arena, Boot envelope, and
// the root TTX type published by the selected ISAs are all stored by the
// Record. The Resolver's `Cache` owns lookup keys and dependency edges which
// keeps the Record focused on the local virtualized state of executing a single
// TTX token bytecode stream.
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

  auto set_boot(Tetrodotoxin::Isa::Boot& boot) -> void { boot_info = &boot; }

  auto publish(Ttx::Type& type, Perimortem::Core::View::Bytes import_name)
      -> void {
    this->type = &type;
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
  constexpr auto get_boot() -> Tetrodotoxin::Isa::Boot& { return *boot_info; }
  constexpr auto get_boot() const -> const Tetrodotoxin::Isa::Boot& {
    return *boot_info;
  }
  constexpr auto get_type() -> Ttx::Type* { return type; }
  constexpr auto get_type() const -> const Ttx::Type* { return type; }
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
  Tetrodotoxin::Isa::Boot* boot_info = nullptr;
  Ttx::Type* type = nullptr;
  Bool private_source = False;
};

}  // namespace Tetrodotoxin::Resolution::Source
