// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/union.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "ttx/concept/layout.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language {

// Expression is the Abstract contract for one evaluatable value. Expression
// identity remains distinct from Type identity so two values of the same Type
// remain distinct facts in the semantic DAG.
//
// Authored Expressions retain one lexical Anchor containing their complete
// Span and the independent Token a diagnostic should emphasize. Synthetic
// Expressions retain no Anchor because there is no source fact to invent.
// This distinction remains independent from folding and lowering.
//
// get_type() returns the Type produced by the expression or Invalid when the
// source owner cannot establish one. get_inputs() exposes the ordered values
// required to evaluate the expression. Library owns parsing, operator
// legality, executable bodies, and value fitting while reflection consumes
// these stable queries.
class Expression : public Ttx::Concept::Abstract {
 public:
  class Error {
   public:
    enum class Type : Unsigned_8 {
      Unknown = Unsigned_8(-1),
      InvalidOperationType = 0,
      InvalidInput,
      InvalidConstant,
      ResultTypeMismatch,
      ArithmeticOverflow,
      DivisionByZero,
    };

    constexpr Error(Type type, const Expression& expression)
        : type(type), expression(expression) {}

    constexpr auto get_type() const -> Type { return type; }
    constexpr auto get_expression() const -> const Expression& {
      return expression;
    }
    auto get_name() const -> Perimortem::Core::View::Bytes;

   private:
    Type type;
    const Expression& expression;
  };

  using ClassCatagory = Expression;
  static constexpr Perimortem::System::Uuid contract_id{
    0xb9716e09506c45e3,
    0x9537c9b4b327e108,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Concept::Abstract::implements(requested);
  }

  constexpr auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override {
    return get_type().resolve().resolve_context(route);
  }

  virtual constexpr auto get_type() const -> const Ttx::Concept::Abstract& = 0;
  virtual constexpr auto get_inputs() const -> const Ttx::Concept::Layout& = 0;

  // Linking enriches this exact source node after every declaration identity
  // is available. Constants already carry complete Types, while Identifier
  // and Operation owners attach their existing graph edges without replacing
  // the authored Expression.
  virtual auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& context,
      Materializations& materializations) -> Bool;

  // Folding is a cached result of this exact Expression. The source node
  // and every authored edge remain available regardless of the selected
  // Constant, dynamic result, or failure.
  auto fold() -> Perimortem::Utility::
      Result<Perimortem::Core::Option<Expression&>, Error>;

  auto get_folded() -> Perimortem::Core::Option<Expression&>;
  auto get_folded() const -> Perimortem::Core::Option<const Expression&>;

  constexpr auto get_anchor() const
      -> const Perimortem::Core::Option<Ttx::Lexical::Anchor>& {
    return anchor;
  }

  // Ordinary expressions fit only their resolved Type. Constant domains may
  // extend this rule when their value proves a contextual conversion safe.
  virtual constexpr auto fits(const Ttx::Model::Type& target) const -> Bool {
    const Ttx::Concept::Abstract& source_type = get_type().resolve();
    const Ttx::Concept::Abstract& target_type = target.resolve();
    return source_type.is<Ttx::Model::Type>() &&
           target_type.is<Ttx::Model::Type>() && &source_type == &target_type;
  }

 protected:
  // Concrete owners supply the builder because only their factory may use the
  // private constructor. Expression selects source context and the Arena
  // begins the object's lifetime once at its final address.
  template <typename type, typename builder_type>
  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Anchor anchor,
      builder_type&& builder) -> type& {
    static_assert(__is_base_of(Expression, type));
    Perimortem::Core::Option<Ttx::Lexical::Anchor> source(anchor);
    return domain.construct_from<type>([&builder, source]() {
      return static_cast<builder_type&&>(builder)(source);
    });
  }

  template <typename type, typename builder_type>
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      builder_type&& builder) -> type& {
    static_assert(__is_base_of(Expression, type));
    Perimortem::Core::Option<Ttx::Lexical::Anchor> source;
    return domain.construct_from<type>([&builder, source]() {
      return static_cast<builder_type&&>(builder)(source);
    });
  }

  constexpr explicit Expression(
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : anchor(anchor) {}

  Expression(const Expression&) = delete;
  Expression(Expression&&) = delete;
  auto operator=(const Expression&) -> Expression& = delete;
  auto operator=(Expression&&) -> Expression& = delete;

  virtual auto fold_uncached() -> Perimortem::Utility::
      Result<Perimortem::Core::Option<Expression&>, Error>;

 private:
  Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor;
  Perimortem::Core::Static::Union<Expression&, Error> folded;
};

static_assert(__is_trivially_destructible(Expression::Error));

}  // namespace Tetrodotoxin::Library::Language
