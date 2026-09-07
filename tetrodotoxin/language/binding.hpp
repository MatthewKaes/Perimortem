// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/query.hpp"

namespace Tetrodotoxin::Language {

// An authored binding keeps its spelling and documentation visible to source
// tools while resolve reaches the selected subject. Unlike the transparent TTX
// Alias, it may retain a live authority whose answer changes after a source
// edit. Storing that authority rather than its current answer keeps the next
// observation on the same route.
class Binding : public Ttx::Abstract {
 public:
  Binding(
      Perimortem::Core::View::Bytes name,
      ttx_abstract target,
      const Ttx::Concept::Documentation& documentation);
  auto get_name() const -> Perimortem::Core::View::Bytes override;
  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto resolve(ttx_abstract self) const -> ttx_abstract override;
  auto resolve_concept(ttx_borrowed_bytes route) const -> ttx_abstract override;

 protected:
  Binding(
      Perimortem::Core::View::Bytes name,
      const Ttx::Concept::Documentation& documentation);
  auto bind_target(ttx_abstract target) -> bool;

 private:
  const Perimortem::Core::View::Bytes name;
  const Ttx::Concept::Documentation& documentation;
  ttx_abstract target;
};

}  // namespace Tetrodotoxin::Language
