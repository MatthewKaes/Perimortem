// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/selection.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/foreign/function.hpp"
#include "tetrodotoxin/library/language/foreign/state.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language::Foreign {

// Surface is one source local Foreign namespace. It retains the ABI and real
// State and Function identities in authored order while dot and arrow request
// their categories through distinct selectors.
class Surface final : public Ttx::Concept::Abstract {
 public:
  TTX_CONTRACT(Surface, Ttx::Concept::Abstract);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Model::Type& resolution_scope) -> Surface&;

  Surface(const Surface&) = delete;
  Surface(Surface&&) = delete;
  auto operator=(const Surface&) -> Surface& = delete;
  auto operator=(Surface&&) -> Surface& = delete;

  auto parse(Monograph& source, Ttx::Lexical::Cursor& cursor) -> Bool;
  auto link_types(Monograph& source) -> Bool;
  auto link_callables(Monograph& source) -> Bool;
  auto finalize(Monograph& source) -> Bool;

  TTX_NAME("foreign"_view);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto resolve() const -> const Ttx::Concept::Abstract& override {
    return *this;
  }

  auto resolve_context(Perimortem::Core::View::Bytes) const
      -> const Ttx::Concept::Abstract& override;

  auto get_states() const {
    return Perimortem::Core::View::Selection(
        declarations.get_view(), [](const auto& declaration) -> Bool {
          return declaration.get().template is<State>();
        });
  }

  auto get_functions() const {
    return Perimortem::Core::View::Selection(
        declarations.get_view(), [](const auto& declaration) -> Bool {
          return declaration.get().template is<Function>();
        });
  }

  auto select_state(Perimortem::Core::View::Bytes name) const
      -> Perimortem::Core::Option<const State&>;

  auto select_function(Perimortem::Core::View::Bytes name) const
      -> Perimortem::Core::Option<const Function&>;

  constexpr auto get_abi() const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> {
    return abi;
  }

 private:
  enum class Stage : Unsigned_8 {
    Authored,
    TypesLinked,
    CallablesLinked,
    Finalized,
  };

  Surface(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Model::Type& resolution_scope)
      : domain(domain),
        resolution_scope(resolution_scope),
        declarations(domain) {}

  Perimortem::Memory::Allocator::Arena& domain;
  const Ttx::Model::Type& resolution_scope;
  Perimortem::Core::Option<Perimortem::Core::View::Bytes> abi;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      declarations;
  Stage stage = Stage::Authored;
};

}  // namespace Tetrodotoxin::Library::Language::Foreign
