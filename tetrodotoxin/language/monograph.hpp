// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/import.hpp"
#include "ttx/concept/domain.hpp"

namespace Tetrodotoxin::Language {

// The native provider owns this root and its Arena. Workspace keeps them alive
// through the provider's source graph handle, so another frontend can supply
// the same services without adopting this class or allocator.
class Monograph : public Ttx::Domain {
 public:
  Monograph(
      Perimortem::Memory::Allocator::Arena& arena,
      ttx_abstract language,
      const Ttx::Concept::Documentation& documentation,
      ttx_abstract context);
  virtual ~Monograph() = default;
  auto get_language() const -> ttx_abstract { return language; }
  auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return documentation;
  }
  virtual auto get_root() const -> ttx_abstract { return get_abi(); }
  virtual auto get_layer(ttx_abstract requested) const -> ttx_abstract;
  auto retain_import(
      const Import::Description& description,
      Perimortem::Core::Option<Ttx::Lexical::Associations&> associations = {})
      -> Bool;
  auto get_imports() const -> Perimortem::Core::View::Vector<Import*> {
    return imports.get_view();
  }
  virtual auto validate(Ttx::Lexical::Cursor& cursor) const -> Bool;
  auto resolve_concept(ttx_borrowed_bytes route) const -> ttx_abstract override;
  auto layout() const -> ttx_layout override;
  void visit_concepts(ttx_concept_sink result) const override;

 protected:
  Perimortem::Memory::Allocator::Arena& arena;
  const ttx_abstract context;

 private:
  const ttx_abstract language;
  const Ttx::Concept::Documentation& documentation;
  Perimortem::Memory::Managed::Vector<Import*> imports;
};

}  // namespace Tetrodotoxin::Language
