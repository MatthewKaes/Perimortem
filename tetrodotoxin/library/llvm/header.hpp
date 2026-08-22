// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/llvm/carriers.hpp"
#include "tetrodotoxin/library/llvm/export.hpp"
#include "tetrodotoxin/library/llvm/functions.hpp"
#include "tetrodotoxin/library/llvm/globals.hpp"
#include "tetrodotoxin/library/llvm/unit.hpp"
#include "tetrodotoxin/linker/fingerprint.hpp"

namespace Tetrodotoxin::Library::Llvm {

// Header is the generated C interface for one completed publication set. Its
// bytes share the compilation Arena and remain valid with the other products.
class Header {
 public:
  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      const Carriers& carriers,
      const Functions& functions,
      const Globals& globals,
      const Unit& unit,
      Perimortem::Core::View::Vector<Export> exports)
      -> Perimortem::Core::Option<Header>;

  static auto identify(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes source,
      Perimortem::Core::View::Bytes owner,
      Tetrodotoxin::Linker::Fingerprint fingerprint)
      -> Perimortem::Core::Option<Header>;

  constexpr auto get_view() const -> Perimortem::Core::View::Bytes {
    return value;
  }

 private:
  constexpr Header(Perimortem::Core::View::Bytes value) : value(value) {}

  Perimortem::Core::View::Bytes value;
};

}  // namespace Tetrodotoxin::Library::Llvm
