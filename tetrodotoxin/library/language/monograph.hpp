// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/managed/map.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/import.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/concept/reference.hpp"

namespace Tetrodotoxin::Library::Language {

// Monograph owns the exact local Structure and Function scope with independent
// authored public order for one Library source. Each declaration retains its
// own source facts while one installed Dialect Materializations inventory lives
// beside every graph it constructs in the Environment Arena.
class Monograph : public Tetrodotoxin::Language::Monograph {
 private:
  Monograph(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Tetrodotoxin::Library::Dialect& host,
      const Ttx::Concept::Abstract& interpretation_context,
      Materializations& materializations);

 public:
  using ClassCatagory = Monograph;
  static constexpr Perimortem::System::Uuid contract_id{
    0x4f5524bd23e84c30,
    0x8d7a8798bf1a25d2,
  };

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Tetrodotoxin::Library::Dialect& host,
      const Ttx::Concept::Abstract& interpretation_context,
      Materializations& materializations) -> Monograph&;

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Tetrodotoxin::Language::Monograph::implements(requested);
  }

  // The Function and every nested view must share this Monograph Arena
  // lifetime. Duplicate names leave lookup and publication unchanged.
  auto bind_function(Function& function) -> Bool;

  // Structures occupy the same exact local name surface as Functions. Their
  // private visibility controls publication without hiding local Type lookup.
  auto bind_structure(Types::Structure& structure) -> Bool;

  // Imports remain in authored order until linking can see every
  // Package member. Retaining the value adds no parser state to the graph.
  auto retain_import(const Import& import) -> void;

  auto link() -> Bool override;

  auto finalize() -> Bool override;

  auto get_name() const -> Perimortem::Core::View::Bytes override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto get_public_functions() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Function>>;

  auto get_functions() const
      -> Perimortem::Core::View::Vector<Ttx::Concept::Reference<Function>>;

  auto get_public_structures() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Types::Structure>>;

  auto get_structures() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<Types::Structure>>;

  auto get_imports() const -> Perimortem::Core::View::Vector<Import>;

  constexpr auto get_materializations() const -> const Materializations& {
    return materializations;
  }

 private:
  auto link_imports() -> Bool;

  Tetrodotoxin::Library::Dialect& library_host;
  const Ttx::Concept::Abstract& interpretation_context;
  Materializations& materializations;
  Perimortem::Memory::Managed::Vector<Import> imports;
  Perimortem::Memory::Managed::Map<
      Perimortem::Core::View::Bytes,
      Ttx::Concept::Reference<const Function>>
      functions;
  Perimortem::Memory::Managed::Map<
      Perimortem::Core::View::Bytes,
      Ttx::Concept::Reference<const Types::Structure>>
      structures;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Function>>
      authored_functions;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<const Function>>
      public_functions;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Types::Structure>>
      authored_structures;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Types::Structure>>
      public_structures;
};

}  // namespace Tetrodotoxin::Library::Language
