// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/import.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "tetrodotoxin/library/language/visibility.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language {

// Monograph retains one Library source transaction. Its synthetic Structure
// owns name lookup while Monograph keeps the source facts and ordered barriers
// that do not belong to a Type.
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

  // Every authored declaration enters the synthetic source Structure before
  // its later semantic barriers run. Duplicate names leave both owners intact.
  auto bind_static(Ttx::Concept::Abstract& binding, Visibility visibility)
      -> Bool;

  // Imports remain in authored order until linking can inspect each selected
  // Package member through its own contextual source route.
  auto retain_import(const Import& import) -> Bool;

  auto link() -> Bool override;

  auto finalize() -> Bool override;

  auto get_name() const -> Perimortem::Core::View::Bytes override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto get_source() -> Ttx::Model::Type&;

  auto get_source() const -> const Ttx::Model::Type&;

  auto get_authored_bindings() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>;

  auto get_imports() const -> Perimortem::Core::View::Vector<Import>;

  constexpr auto get_materializations() const -> const Materializations& {
    return materializations;
  }

  constexpr auto get_library_host() const -> Tetrodotoxin::Library::Dialect& {
    return library_host;
  }

  constexpr auto get_interpretation_context() const
      -> const Ttx::Concept::Abstract& {
    return interpretation_context;
  }

 private:
  auto link_imports() -> Bool;

  Tetrodotoxin::Library::Dialect& library_host;
  const Ttx::Concept::Abstract& interpretation_context;
  Materializations& materializations;
  Perimortem::Memory::Managed::Vector<Import> imports;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      authored_bindings;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      authored_binding_observations;
  Perimortem::Core::Option<Ttx::Concept::Reference<Ttx::Model::Type>>
      source_structure;
  Bool imports_linked = False;
};

}  // namespace Tetrodotoxin::Library::Language
