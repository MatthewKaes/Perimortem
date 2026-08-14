// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/diagnostic.hpp"
#include "ttx/lexical/anchor.hpp"

namespace Tetrodotoxin::Language {

// Diagnostics owns the ordered source independent failures for one outer
// source or restored member. Every participating Monograph borrows this exact
// transaction so child activity stays in occurrence order without a merge.
class Diagnostics {
 public:
  explicit Diagnostics(Perimortem::Memory::Allocator::Arena& domain);

  Diagnostics(const Diagnostics&) = delete;
  Diagnostics(Diagnostics&&) = delete;
  auto operator=(const Diagnostics&) -> Diagnostics& = delete;
  auto operator=(Diagnostics&&) -> Diagnostics& = delete;

  auto report(
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) -> void;

  auto get_values() const -> Perimortem::Core::View::Vector<Diagnostic>;

 private:
  Perimortem::Memory::Allocator::Arena& domain;
  Perimortem::Memory::Managed::Vector<Diagnostic> values;
};

}  // namespace Tetrodotoxin::Language
