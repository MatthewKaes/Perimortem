// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/isa/base/implementation.hpp"

namespace Tetrodotoxin::Puffer::Resolution::Source {

// Owns every allocation produced while one source is evaluated.
//
// Resolution must establish this lifetime before Boot or a body ISA can build
// semantic facts. Storage deliberately has no dialect, import, or root Type
// fields, so an evaluation in progress cannot masquerade as a resolved source
// record. A successful evaluation places this storage behind one final Record.
class Storage {
 public:
  Storage(
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source_text);
  explicit Storage(Perimortem::Core::View::Bytes source_path);

  Storage(const Storage&) = delete;
  auto operator=(const Storage&) -> Storage& = delete;

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

  constexpr auto get_implementation() const
      -> const Tetrodotoxin::Isa::Base::Implementation& {
    return implementation;
  }

 private:
  auto retain(Perimortem::Core::View::Bytes bytes)
      -> Perimortem::Core::View::Bytes;

  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Bytes source_text;
  Perimortem::Core::View::Bytes module;
  Tetrodotoxin::Isa::Base::Implementation implementation;
};

}  // namespace Tetrodotoxin::Puffer::Resolution::Source
