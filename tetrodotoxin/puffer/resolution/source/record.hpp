// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/data.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/isa/base/implementation.hpp"
#include "tetrodotoxin/isa/dialect.hpp"
#include "tetrodotoxin/puffer/isa/boot/import.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Puffer::Resolution::Source {

// Stable source entry owned by the resolution cache.
//
// Resolver constructs a private Record before evaluation so every produced TTX
// fact lands in its final arena. Once Boot, imports, and the body ISA succeed,
// complete attaches the resolved semantic facts and only then may Source::Cache
// publish the Record. The retained module name gives every downstream consumer
// one source identity without teaching generic path code about TTX conventions.
// Incomplete records never escape the resolving call.
class Record {
 public:
  Record(
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source_text);
  explicit Record(Perimortem::Core::View::Bytes source_path);

  auto complete(
      const Tetrodotoxin::Isa::Dialect& dialect,
      Perimortem::Core::View::Vector<Tetrodotoxin::Puffer::Isa::Boot::Import>
          imports,
      const Ttx::Type& type) -> Bool;
  // Records are single lifetime owning objects and the `Dynamic::Object`
  // wrapper should be used to create handles.
  Record(const Record&) = delete;
  auto operator=(const Record&) -> Record& = delete;

  constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return source_path;
  }

  constexpr auto get_module() const -> Perimortem::Core::View::Bytes {
    return module;
  }

  constexpr auto get_content() const -> Perimortem::Core::View::Bytes {
    return source_text;
  }

  constexpr auto get_arena() -> Perimortem::Memory::Allocator::Arena& {
    return arena;
  }

  constexpr auto get_implementation()
      -> Tetrodotoxin::Isa::Base::Implementation& {
    return implementation;
  }

  constexpr auto get_dialect() const -> const Tetrodotoxin::Isa::Dialect& {
    return dialect;
  }

  constexpr auto get_imports() const -> Perimortem::Core::View::Vector<
      Tetrodotoxin::Puffer::Isa::Boot::Import> {
    return imports;
  }

  constexpr auto get_type() const -> const Ttx::Type& { return *type; }

  constexpr auto get_implementation() const
      -> const Tetrodotoxin::Isa::Base::Implementation& {
    return implementation;
  }

  constexpr auto is_complete() const -> Bool {
    return dialect.is_valid() && type != nullptr;
  }

 private:
  auto retain(Perimortem::Core::View::Bytes bytes)
      -> Perimortem::Core::View::Bytes;

  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Bytes source_text;
  Perimortem::Core::View::Bytes module;
  Tetrodotoxin::Isa::Base::Implementation implementation;
  Tetrodotoxin::Isa::Dialect dialect;
  Perimortem::Core::View::Vector<Tetrodotoxin::Puffer::Isa::Boot::Import>
      imports;
  const Ttx::Type* type = nullptr;
};

}  // namespace Tetrodotoxin::Puffer::Resolution::Source
