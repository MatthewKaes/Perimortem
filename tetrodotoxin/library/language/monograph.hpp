// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/import.hpp"
#include "ttx/concept/reference.hpp"

namespace Tetrodotoxin::Library::Language {

// Monograph owns the exact local Function scope and the ordered public view
// for one Library source. Both inventories borrow the same Function identities
// from the Environment graph Arena.
class Monograph : public Tetrodotoxin::Language::Dialect::Monograph {
 public:
  using ClassCatagory = Monograph;
  static constexpr Perimortem::System::Uuid contract_id{
    0x4f5524bd23e84c30,
    0x8d7a8798bf1a25d2,
  };

  Monograph(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Tetrodotoxin::Library::Dialect& host,
      const Ttx::Concept::Abstract& interpretation_context);

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Tetrodotoxin::Language::Dialect::Monograph::implements(requested);
  }

  // The Function and every nested view must share this Monograph Arena
  // lifetime. Duplicate names leave lookup and publication unchanged.
  auto bind_function(Function& function) -> Bool;

  // Imports remain in authored order until the post pass can see every
  // Package member. Retaining the value adds no parser state to the graph.
  auto retain_import(const Import& import) -> void;

  auto post_pass() -> Bool override;

  auto get_name() const -> Perimortem::Core::View::Bytes override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto get_public_functions() const
      -> Perimortem::Core::View::Vector<Ttx::Concept::Reference<Function>>;

 private:
  const Ttx::Concept::Abstract& interpretation_context;
  Perimortem::Memory::Managed::Vector<Import> imports;
  Perimortem::Memory::Managed::
      Map<Perimortem::Core::View::Bytes, Ttx::Concept::Reference<Function>>
          functions;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Function>>
      public_functions;
};

}  // namespace Tetrodotoxin::Library::Language
