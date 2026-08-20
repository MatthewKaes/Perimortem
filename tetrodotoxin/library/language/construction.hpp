// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/parameter.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"

namespace Tetrodotoxin::Library::Language {

// Construction is the provider-owned ABI Callable for one public authored
// aggregate Type. Each optional parameter corresponds to one externally
// assignable state Field. Absence selects the provider's authored fallback,
// keeping private defaults and implementation Callables out of Interfaces.
class Construction : public Model::Callable {
 public:
  TTX_CONTRACT(Construction, Model::Callable);

  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      Types::Composite& owner,
      Bool provider) -> Perimortem::Core::Option<Construction&>;

  auto create_call(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::Option<Model::Pack&> supplied = {}) const
      -> Perimortem::Core::Option<Model::Pack&>;

  TTX_NAME("$construct"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto resolve_context(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_parameters() const
      -> const Ttx::Concept::Layout& override {
    return parameters;
  }

  constexpr auto get_results() const -> const Ttx::Concept::Layout& override {
    return results;
  }

  auto reserve_declaration(Llvm::Program& program) const -> Bool override;

  auto complete_declaration(Llvm::Program& program) const -> Bool override;

  auto lower_declaration(Llvm::Program& program) const -> Bool override;

  constexpr auto get_owner() const -> const Types::Composite& { return owner; }

  constexpr auto has_provider_body() const -> Bool { return provider; }

 private:
  Construction(
      Perimortem::Memory::Allocator::Arena& arena,
      Types::Composite& owner,
      Perimortem::Memory::Managed::Vector<
          Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
          source_parameters,
      Perimortem::Memory::Managed::Vector<
          Ttx::Concept::Reference<const Ttx::Concept::Abstract>> source_results,
      Bool provider)
      : arena(arena),
        owner(owner),
        parameter_entries(source_parameters),
        result_entries(source_results),
        parameters(parameter_entries.get_view()),
        results(result_entries.get_view()),
        values(arena),
        provider(provider) {}

  auto complete_body() -> Bool;

  Perimortem::Memory::Allocator::Arena& arena;
  Types::Composite& owner;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      parameter_entries;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      result_entries;
  Ttx::Model::Layouts::Named parameters;
  Ttx::Model::Layouts::Fluid results;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Model::Pack>>
      values;
  Perimortem::Core::Option<Model::Pack&> initializer;
  Bool provider;
};

}  // namespace Tetrodotoxin::Library::Language
