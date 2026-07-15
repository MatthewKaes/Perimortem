// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/object.hpp"

#include "tetrodotoxin/isa/dialect.hpp"
#include "tetrodotoxin/puffer/isa/boot/import.hpp"
#include "tetrodotoxin/puffer/resolution/source/storage.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Puffer::Resolution::Source {

// Completed source entry owned by the resolution cache.
//
// Storage owns the allocations required during evaluation. Record is created
// only after Boot, imports, and the selected body ISA have all succeeded, so a
// cache entry always has a valid dialect and root Type by construction.
class Record {
 public:
  Record(
      Perimortem::Memory::Dynamic::Object<Storage> storage,
      const Tetrodotoxin::Isa::Dialect& dialect,
      Perimortem::Core::View::Vector<Tetrodotoxin::Puffer::Isa::Boot::Import>
          imports,
      const Ttx::Type& type)
      : storage(storage), dialect(dialect), imports(imports), type(type) {}

  Record(const Record&) = delete;
  auto operator=(const Record&) -> Record& = delete;

  constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return storage->get_source_path();
  }

  constexpr auto get_module() const -> Perimortem::Core::View::Bytes {
    return storage->get_module();
  }

  constexpr auto get_content() const -> Perimortem::Core::View::Bytes {
    return storage->get_content();
  }

  constexpr auto get_dialect() const -> const Tetrodotoxin::Isa::Dialect& {
    return dialect;
  }

  constexpr auto get_imports() const -> Perimortem::Core::View::Vector<
      Tetrodotoxin::Puffer::Isa::Boot::Import> {
    return imports;
  }

  constexpr auto get_type() const -> const Ttx::Type& { return type; }

  constexpr auto get_implementation() const
      -> const Tetrodotoxin::Isa::Base::Implementation& {
    return storage->get_implementation();
  }

 private:
  Perimortem::Memory::Dynamic::Object<Storage> storage;
  Tetrodotoxin::Isa::Dialect dialect;
  Perimortem::Core::View::Vector<Tetrodotoxin::Puffer::Isa::Boot::Import>
      imports;
  const Ttx::Type& type;
};

}  // namespace Tetrodotoxin::Puffer::Resolution::Source
