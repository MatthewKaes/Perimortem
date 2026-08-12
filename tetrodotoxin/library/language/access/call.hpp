// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/argument_pack.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/layouts/composite.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Call owns one complete postfix `-> name(arguments)` invocation. Its one
// receiver Expression determines Static or Self selection from the exact
// semantic result produced during linking. Composite registration guarantees
// one Callable per name and receiver role, so Call validates one selected
// signature rather than searching an overload set.
class Call : public Expression {
 public:
  TTX_CONTRACT(Call, Expression, 0x1bc2984aba944842, 0x9ed57eef00ed826f);

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context,
      Expression& receiver) -> Perimortem::Core::Option<Expression&>;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& lexical_context,
      Materializations& materializations,
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope = {})
      -> Bool override;

  TTX_NAME(name);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_inputs() const -> const Ttx::Concept::Layout& override;

  auto get_results() const -> const Ttx::Concept::Layout&;

  auto get_callable() const
      -> Perimortem::Core::Option<const Ttx::Model::Callable&>;

  constexpr auto get_receiver() const -> const Expression& { return receiver; }

  constexpr auto get_name_token() const -> Ttx::Lexical::Token {
    return name_token;
  }

 private:
  // ReceiverLayout is the one value edge that precedes a Self invocation's
  // authored ArgumentPack. It returns the real receiver Expression while
  // fitting that value through Expression::fits(), so the reflected Call input
  // Layout preserves the same result proved during link without a proxy value.
  class ReceiverLayout : public Ttx::Concept::Layout {
   public:
    constexpr explicit ReceiverLayout(Expression& receiver)
        : receiver(receiver) {}

    constexpr auto get_size() const -> Count override { return 1; }

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
    Expression& receiver;
  };

  constexpr Call(
      Expression& receiver,
      Ttx::Lexical::Token name_token,
      Perimortem::Core::View::Bytes name,
      ArgumentPack& arguments,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor),
        receiver(receiver),
        name_token(name_token),
        name(name),
        arguments(arguments),
        receiver_inputs(receiver),
        self_inputs(receiver_inputs, arguments) {}

  Expression& receiver;
  Ttx::Lexical::Token name_token;
  Perimortem::Core::View::Bytes name;
  ArgumentPack& arguments;
  ReceiverLayout receiver_inputs;
  Ttx::Model::Layouts::Composite self_inputs;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Callable>>
      callable;
};

}  // namespace Tetrodotoxin::Library::Language::Access
