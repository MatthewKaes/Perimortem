// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "ttx/concept/reference.hpp"

namespace Tetrodotoxin::Library::Language {

// Operation is the Expression base for executable value operations. It owns
// authored input order and the one exact result Type selected after every
// child links. A concrete operation supplies its legality and Constant
// evaluation without asking Parser to interpret semantic Types.
class Operation : public Expression {
 public:
  TTX_CONTRACT(Operation, Expression, 0x7b99d8819ace4f54, 0x84d44c33d0c23202);

  constexpr auto get_inputs() const -> const Ttx::Concept::Layout& override {
    return input_layout;
  }

  auto get_type() const -> const Ttx::Concept::Abstract& override;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& context,
      Materializations& materializations) -> Bool override;

 protected:
  Operation(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Perimortem::Core::View::Vector<Ttx::Concept::Reference<Expression>>
          inputs,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor);

  // Operation retains one typed edge inventory. Its private Layout exposes
  // const reflection without erasing the mutation required by link and fold.
  auto get_input(Count index) -> Perimortem::Core::Option<Expression&>;
  auto get_input(Count index) const
      -> Perimortem::Core::Option<const Expression&>;

  auto fold_input(Count index) -> Perimortem::Utility::
      Result<Perimortem::Core::Option<Expression&>, Expression::Error>;

  // Concrete evaluation runs only after the ordered traversal completed.
  // This observation unwraps that cached result without changing the
  // authored edge or starting another computation.
  auto get_folded_input(Count index) -> Perimortem::Core::Option<Expression&>;

  virtual auto evaluate_constants(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations) -> Perimortem::Utility::
      Result<Perimortem::Core::Option<Constant&>, Expression::Error> = 0;

  virtual auto reaches_next_input(Count folded_input, const Expression& folded)
      const -> Bool;

  virtual auto select_type(Materializations& materializations) const
      -> Perimortem::Core::Option<const Ttx::Model::Type&> = 0;

  auto fold_uncached() -> Perimortem::Utility::
      Result<Perimortem::Core::Option<Expression&>, Expression::Error> override;

 private:
  class InputLayout : public Ttx::Concept::Layout {
   public:
    constexpr explicit InputLayout(
        const Perimortem::Memory::Managed::Vector<
            Ttx::Concept::Reference<Expression>>& inputs)
        : inputs(inputs) {}

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
  };

  Perimortem::Memory::Allocator::Arena& domain;
  Materializations& materializations;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Expression>>
      inputs;
  InputLayout input_layout;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Type>>
      result_type;
};

}  // namespace Tetrodotoxin::Library::Language

// Binary Operations share their category proof, construction surface, and
// canonical semantic name while keeping parsing and evaluation visible.
#define BINARY_OP_CONTRACT(type, high, low)                                    \
  TTX_CONTRACT(type, Operation, high, low);                                    \
  static auto create_authored(                                                 \
      Perimortem::Memory::Allocator::Arena& domain,                            \
      Materializations& materializations, Expression& left, Expression& right, \
      Ttx::Lexical::Anchor anchor) -> type&;                                   \
  static auto create_synthetic(                                                \
      Perimortem::Memory::Allocator::Arena& domain,                            \
      Materializations& materializations, Expression& left, Expression& right) \
      -> type&;                                                                \
  constexpr auto get_name() const -> Perimortem::Core::View::Bytes override {  \
    return #type##_view;                                                       \
  }

// Concrete Operations own distinct semantics while sharing the same authored
// and synthetic construction path over one retained operand inventory.
#define TTX_BINARY_OP(type)                                                    \
  auto Tetrodotoxin::Library::Language::Operations::type::create_authored(     \
      Perimortem::Memory::Allocator::Arena& domain,                            \
      Materializations& materializations, Expression& left, Expression& right, \
      Ttx::Lexical::Anchor anchor) -> type& {                                  \
    return Expression::create_authored<type>(                                  \
        domain, anchor, [&](auto source) -> type {                             \
          return type(domain, materializations, left, right, source);          \
        });                                                                    \
  }                                                                            \
  auto Tetrodotoxin::Library::Language::Operations::type::create_synthetic(    \
      Perimortem::Memory::Allocator::Arena& domain,                            \
      Materializations& materializations, Expression& left, Expression& right) \
      -> type& {                                                               \
    return Expression::create_synthetic<type>(                                 \
        domain, [&](auto source) -> type {                                     \
          return type(domain, materializations, left, right, source);          \
        });                                                                    \
  }                                                                            \
  Tetrodotoxin::Library::Language::Operations::type::type(                     \
      Perimortem::Memory::Allocator::Arena& domain,                            \
      Materializations& materializations, Expression& left, Expression& right, \
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)                   \
      : Operation(                                                             \
            domain, materializations,                                          \
            Perimortem::Core::Static::Vector<                                  \
                Ttx::Concept::Reference<Expression>, 2>{{left, right}},        \
            anchor) {}

#define TTX_UNARY_OP(type)                                                  \
  auto Tetrodotoxin::Library::Language::Operations::type::create_authored(  \
      Perimortem::Memory::Allocator::Arena& domain,                         \
      Materializations& materializations, Expression& operand,              \
      Ttx::Lexical::Anchor anchor) -> type& {                               \
    return Expression::create_authored<type>(                               \
        domain, anchor, [&](auto source) -> type {                          \
          return type(domain, materializations, operand, source);           \
        });                                                                 \
  }                                                                         \
  auto Tetrodotoxin::Library::Language::Operations::type::create_synthetic( \
      Perimortem::Memory::Allocator::Arena& domain,                         \
      Materializations& materializations, Expression& operand) -> type& {   \
    return Expression::create_synthetic<type>(                              \
        domain, [&](auto source) -> type {                                  \
          return type(domain, materializations, operand, source);           \
        });                                                                 \
  }                                                                         \
  Tetrodotoxin::Library::Language::Operations::type::type(                  \
      Perimortem::Memory::Allocator::Arena& domain,                         \
      Materializations& materializations, Expression& operand,              \
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)                \
      : Operation(                                                          \
            domain, materializations,                                       \
            Perimortem::Core::Static::Vector<                               \
                Ttx::Concept::Reference<Expression>, 1>{{operand}},         \
            anchor) {}
