// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "ttx/concept/reference.hpp"

namespace Tetrodotoxin::Library::Language {

// FoldError identifies the exact Expression whose folding transaction failed.
// It retains semantic identity rather than source state so a parser can attach
// its current Span while a later graph pass can log the same durable failure.
// The small owned value stays suitable for Utility Result error storage.
class FoldError {
 public:
  enum class Type : Unsigned_8 {
    Unknown = Unsigned_8(-1),
    InvalidOperationType = 0,
    InvalidInput,
    InvalidConstant,
    ResultTypeMismatch,
    NegativeOperand,
    CountOverflow,
    IndexOutOfBounds,
    RangeStartOutOfBounds,
    RangeSizeOutOfBounds,
    ArithmeticOverflow,
  };

  FoldError(Type type, const Expression& expression);

  auto get_type() const -> Type;
  auto get_expression() const -> const Expression&;
  auto get_name() const -> Perimortem::Core::View::Bytes;

 private:
  Type type;
  Ttx::Concept::Reference<Expression> expression;
};

static_assert(__is_trivially_destructible(FoldError));

}  // namespace Tetrodotoxin::Library::Language
