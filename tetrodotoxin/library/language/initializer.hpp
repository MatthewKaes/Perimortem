// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language {

// Initializer is the authored `new` signal for one declaration that already
// supplies an Object Type. It retains supplied values in source order while
// the Object and its Fields remain the only initialization shape.
class Initializer : public Expression {
 public:
  TTX_CONTRACT(Initializer, Expression, 0x921ccce2e4934c8d, 0x9d19d3434e18f49a);

  static auto is_next(const Ttx::Lexical::Cursor& cursor) -> Bool;

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context)
      -> Perimortem::Core::Option<Initializer&>;

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Expression>>
          inputs,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names,
      Bool named,
      Ttx::Lexical::Anchor anchor) -> Initializer&;

  Initializer(const Initializer&) = delete;
  Initializer(Initializer&&) = delete;
  auto operator=(const Initializer&) -> Initializer& = delete;
  auto operator=(Initializer&&) -> Initializer& = delete;

  TTX_NAME("Initializer"_view);

  auto get_documentation() const
      -> const Ttx::Concept::Documentation& override {
    return Ttx::Concept::Documentation::get_empty();
  }

  auto get_type() const -> const Ttx::Concept::Abstract& override;

  constexpr auto get_inputs() const -> const Ttx::Concept::Layout& override {
    return input_layout;
  }

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& lexical_context,
      Materializations& materializations,
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope = {})
      -> Bool override;

 private:
  class InputLayout : public Ttx::Concept::Layout {
   public:
    constexpr InputLayout(
        const Perimortem::Memory::Managed::Vector<
            Ttx::Concept::Reference<Expression>>& inputs,
        const Perimortem::Memory::Managed::Vector<
            Ttx::Concept::Reference<const Ttx::Concept::Abstract>>&
            observations,
        Bool named)
        : inputs(inputs), observations(observations), named(named) {}

    constexpr auto get_size() const -> Count override {
      return inputs.get_size();
    }

    constexpr auto get_abstract(Count index) const
        -> Perimortem::Core::Option<const Ttx::Concept::Abstract&> override;

    auto fits_at(const Ttx::Concept::Layout& target, Count target_offset) const
        -> Bool override;

    auto get_fitted_at(
        const Ttx::Concept::Layout& target,
        Count target_offset,
        Count target_index) const
        -> Perimortem::Utility::Result<
            const Ttx::Concept::Abstract&,
            Ttx::Concept::Layout::Errors> override;

   private:
    const Perimortem::Memory::Managed::Vector<
        Ttx::Concept::Reference<Expression>>& inputs;
    const Perimortem::Memory::Managed::Vector<
        Ttx::Concept::Reference<const Ttx::Concept::Abstract>>& observations;
    Bool named;
  };

  Initializer(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Expression>>
          inputs,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> names,
      Bool named,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);

  auto supplies(
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope,
      const Types::Object& target,
      const Field& field) const -> Bool;

  auto has_mandatory_cycle(
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope,
      const Types::Object& target,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<const Types::Object>> path) const -> Bool;

  Perimortem::Memory::Allocator::Arena& domain;
  Materializations& materializations;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Expression>>
      inputs;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      observations;
  Bool named;
  InputLayout input_layout;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Types::Object>>
      expected_type;
};

}  // namespace Tetrodotoxin::Library::Language
