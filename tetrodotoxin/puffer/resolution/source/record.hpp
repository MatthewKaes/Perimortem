// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data.hpp"
#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/puffer/isa/boot/envelope.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Puffer::Resolution::Source {

// Stable source entry owned by the resolution cache.
//
// A record owns the storage whose lifetime is exactly one resolved source file.
// Source bytes, token bytes, the Boot preamble, and the root TTX type produced
// by the selected ISA all live in this arena. The Resolver and Cache keep
// Record alive through Dynamic::Object handles, leaving Record focused on the
// local virtualized state of one source stream.
class Record {
 public:
  Record(
      Perimortem::Core::View::Bytes path,
      Perimortem::Core::View::Bytes text,
      Bool private_record)
      : source_path(copy(arena, path)),
        import_name(source_path),
        source_text(copy(arena, text)),
        tokenizer(arena, source_text, source_path),
        private_source(private_record) {}

  // Records are single lifetime owning objects and the `Dynamic::Object`
  // wrapper should be used to create handles.
  Record(const Record&) = delete;
  auto operator=(const Record&) -> Record& = delete;

  auto set_boot(Tetrodotoxin::Puffer::Isa::Boot::Envelope& boot) -> void {
    boot_info = &boot;
  }

  auto publish(Ttx::Type& type, Perimortem::Core::View::Bytes import_name)
      -> void {
    this->type = &type;
    if (!(this->import_name == import_name)) {
      this->import_name = import_name;
    }
  }

  constexpr auto get_tokenizer() const -> const Ttx::Lexical::Tokenizer& {
    return tokenizer;
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
  constexpr auto get_boot() -> Tetrodotoxin::Puffer::Isa::Boot::Envelope& {
    return *boot_info;
  }
  constexpr auto get_boot() const
      -> const Tetrodotoxin::Puffer::Isa::Boot::Envelope& {
    return *boot_info;
  }
  constexpr auto get_type() -> Ttx::Type* { return type; }
  constexpr auto get_type() const -> const Ttx::Type* { return type; }
  constexpr auto is_private() const -> Bool { return private_source; }

 private:
  static auto copy(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes bytes) -> Perimortem::Core::View::Bytes {
    if (bytes.is_empty()) {
      return Perimortem::Core::View::Bytes();
    }

    Bits_8* data = arena.allocate(bytes.get_size());
    Perimortem::Core::Data::copy(data, bytes.get_data(), bytes.get_size());
    return Perimortem::Core::View::Bytes(data, bytes.get_size());
  }

  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Bytes import_name;
  Perimortem::Core::View::Bytes source_text;
  Ttx::Lexical::Tokenizer tokenizer;
  Tetrodotoxin::Puffer::Isa::Boot::Envelope* boot_info = nullptr;
  Ttx::Type* type = nullptr;
  Bool private_source = False;
};

}  // namespace Tetrodotoxin::Puffer::Resolution::Source
