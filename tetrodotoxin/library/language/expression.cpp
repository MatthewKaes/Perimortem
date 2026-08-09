// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/expression.hpp"

#include "tetrodotoxin/library/language/constant.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto select_source_anchor(const Language::Expression& expression)
    -> Perimortem::Utility::Option<Ttx::Lexical::Anchor> {
  return expression.get_anchor().visit(
      []() -> Perimortem::Utility::Option<Ttx::Lexical::Anchor> { return {}; },
      [](const Ttx::Lexical::Anchor& anchor)
          -> Perimortem::Utility::Option<Ttx::Lexical::Anchor> {
        return anchor;
      });
}

auto Language::Expression::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract&,
    Materializations&) -> Bool {
  auto source_anchor = select_source_anchor(*this);

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
    Result<Perimortem::Utility::Option<Expression&>, Error> {
  if (!folded.is_null()) {
    return folded.visit(
        []() -> Perimortem::Utility::Result<
                 Perimortem::Utility::Option<Expression&>, Error> {
          return Perimortem::Utility::Option<Expression&>{};
        },
        [](Expression& representation)
            -> Perimortem::Utility::Result<
                Perimortem::Utility::Option<Expression&>, Error> {
          return representation;
        },
        [](const Error& error)
            -> Perimortem::Utility::Result<
                Perimortem::Utility::Option<Expression&>, Error> {
          return error;
        });
  }

  const Abstract& expression_type = get_type().resolve();
  if (!expression_type.is<Type>()) {
    return Perimortem::Utility::Option<Expression&>{};
  }

  auto result = fold_uncached();
  return result.visit(
      [&](const Perimortem::Utility::Option<Expression&>& selected)
          -> Perimortem::Utility::Result<
              Perimortem::Utility::Option<Expression&>, Error> {
        if (!selected) {
          return Perimortem::Utility::Option<Expression&>{};
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
        return Perimortem::Utility::Option<Expression&>(representation);
      },
      [&](const Error& error)
          -> Perimortem::Utility::Result<
              Perimortem::Utility::Option<Expression&>, Error> {
        folded = error;
        return error;
      });
}

auto Language::Expression::get_folded()
    -> Perimortem::Utility::Option<Expression&> {
  return folded.visit(
      []() -> Perimortem::Utility::Option<Expression&> { return {}; },
      [](Expression& representation)
          -> Perimortem::Utility::Option<Expression&> {
        return representation;
      },
      [](const Error&) -> Perimortem::Utility::Option<Expression&> {
        return {};
      });
}

auto Language::Expression::get_folded() const
    -> Perimortem::Utility::Option<const Expression&> {
  return folded.visit(
      []() -> Perimortem::Utility::Option<const Expression&> { return {}; },
      [](const Expression& representation)
          -> Perimortem::Utility::Option<const Expression&> {
        return representation;
      },
      [](const Error&) -> Perimortem::Utility::Option<const Expression&> {
        return {};
      });
}

auto Language::Expression::fold_uncached() -> Perimortem::Utility::
    Result<Perimortem::Utility::Option<Expression&>, Error> {
  return Perimortem::Utility::Option<Expression&>{};
}
