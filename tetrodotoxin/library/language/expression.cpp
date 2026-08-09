// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/expression.hpp"

#include "tetrodotoxin/library/language/constant.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

auto Language::Expression::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract&,
    Materializations&) -> Bool {
  auto source_anchor = get_anchor();

  if (get_type().resolve().is<Type>()) {
    return True;
  }

  source.report(
      source_anchor, "Expression did not resolve one exact Type."_view,
      "Complete its semantic inputs before finalization."_view);
  return False;
}

auto Language::Expression::Error::get_name() const -> View::Bytes {
  switch (type) {
  case Type::InvalidOperationType:
    return "invalid operation Type"_view;
  case Type::InvalidInput:
    return "invalid operation input"_view;
  case Type::InvalidConstant:
    return "invalid Constant domain"_view;
  case Type::ResultTypeMismatch:
    return "changed result Type"_view;
  case Type::ArithmeticOverflow:
    return "arithmetic overflow"_view;
  case Type::DivisionByZero:
    return "division by zero"_view;
  case Type::Unknown:
    return "unknown fold failure"_view;
  }

  return "unknown fold failure"_view;
}

auto Language::Expression::fold() -> Perimortem::Utility::
    Result<Perimortem::Core::Option<Expression&>, Error> {
  if (!folded.is_null()) {
    return folded.visit(
        []() -> Perimortem::Utility::Result<
                 Perimortem::Core::Option<Expression&>, Error> {
          return Perimortem::Core::Option<Expression&>{};
        },
        [](Expression& representation)
            -> Perimortem::Utility::Result<
                Perimortem::Core::Option<Expression&>, Error> {
          return representation;
        },
        [](const Error& error)
            -> Perimortem::Utility::Result<
                Perimortem::Core::Option<Expression&>, Error> {
          return error;
        });
  }

  const Abstract& expression_type = get_type().resolve();
  if (!expression_type.is<Type>()) {
    return Perimortem::Core::Option<Expression&>{};
  }

  auto result = fold_uncached();
  return result.visit(
      [&](const Perimortem::Core::Option<Expression&>& selected)
          -> Perimortem::Utility::Result<
              Perimortem::Core::Option<Expression&>, Error> {
        if (!selected) {
          return Perimortem::Core::Option<Expression&>{};
        }

        Expression& representation = *selected;
        const Abstract& result_type = representation.get_type().resolve();
        if (!representation.is<Constant>()) {
          Error error(Error::Type::InvalidConstant, *this);
          folded = error;
          return error;
        }

        if (!result_type.is<Type>() || &expression_type != &result_type) {
          Error error(Error::Type::ResultTypeMismatch, *this);
          folded = error;
          return error;
        }

        folded = representation;
        return Perimortem::Core::Option<Expression&>(representation);
      },
      [&](const Error& error)
          -> Perimortem::Utility::Result<
              Perimortem::Core::Option<Expression&>, Error> {
        folded = error;
        return error;
      });
}

auto Language::Expression::get_folded()
    -> Perimortem::Core::Option<Expression&> {
  return folded.visit(
      []() -> Perimortem::Core::Option<Expression&> { return {}; },
      [](Expression& representation) -> Perimortem::Core::Option<Expression&> {
        return representation;
      },
      [](const Error&) -> Perimortem::Core::Option<Expression&> { return {}; });
}

auto Language::Expression::get_folded() const
    -> Perimortem::Core::Option<const Expression&> {
  return folded.visit(
      []() -> Perimortem::Core::Option<const Expression&> { return {}; },
      [](const Expression& representation)
          -> Perimortem::Core::Option<const Expression&> {
        return representation;
      },
      [](const Error&) -> Perimortem::Core::Option<const Expression&> {
        return {};
      });
}

auto Language::Expression::fold_uncached() -> Perimortem::Utility::
    Result<Perimortem::Core::Option<Expression&>, Error> {
  return Perimortem::Core::Option<Expression&>{};
}
