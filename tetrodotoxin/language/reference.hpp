// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/concept/abstract.hpp"

namespace Tetrodotoxin::Language {

// A reference remembers where to ask and the authored question. The host can
// be a foreign capability or a stable source authority, so resolving another
// edit needs neither a native object conversion nor a cached target to repair.
class Reference : public Ttx::Abstract {
 public:
  Reference(ttx_abstract host, Perimortem::Core::View::Bytes name);
  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      ttx_abstract host,
      Perimortem::Core::View::Bytes name) -> Reference&;
  auto resolve(ttx_abstract self) const -> ttx_abstract override;
  auto get_name() const -> Perimortem::Core::View::Bytes override;
  auto get_host() const -> ttx_abstract;

 private:
  const ttx_abstract host;
  const Perimortem::Core::View::Bytes name;
};

}  // namespace Tetrodotoxin::Language
