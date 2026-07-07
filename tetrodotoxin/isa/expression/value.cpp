// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/expression/value.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/serialization/escaped_text.hpp"

#include "tetrodotoxin/isa/expression/pack.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

static constexpr Static::Vector<Class::Type, 2> expression_reference_starts = {
  Class::Type::Addressable,
  Class::Type::Self,
};

static constexpr Static::Vector<Class::Type, 2> expression_index_starts = {
  Class::Type::SliceOp,
  Class::Type::IndexStart,
};

static auto binary_operator(Class::Type type) -> Expression::Value::Operator {
  switch (type) {
  case Class::Type::AddOp:
    return Expression::Value::Operator::Add;
  case Class::Type::SubOp:
    return Expression::Value::Operator::Subtract;
  case Class::Type::MulOp:
    return Expression::Value::Operator::Multiply;
  case Class::Type::DivOp:
    return Expression::Value::Operator::Divide;
  default:
    return Expression::Value::Operator::None;
  }
}

static auto operator_precedence(Expression::Value::Operator op) -> Count {
  switch (op) {
  case Expression::Value::Operator::Multiply:
  case Expression::Value::Operator::Divide:
    return 2;
  case Expression::Value::Operator::Add:
  case Expression::Value::Operator::Subtract:
    return 1;
  default:
    return 0;
  }
}

static auto consume_index(Cursor& cursor, Context& context) -> Bool;

static auto evaluate_expression(
    Cursor& cursor,
    Context& context,
    Count minimum_precedence) -> Expression::Value;

static auto evaluate_reference(Cursor& cursor, Context& context)
    -> Expression::Value {
  const Count start = cursor.get_token_index();
  View::Bytes root = cursor.consume().get_text();
  while (!cursor.matches(Class::Type::EndOfStream)) {
    if (cursor.matches(Class::Type::AddressOp)) {
      cursor.consume();
      if (!cursor.require(
              Class::Type::Addressable,
              "Expected member name after `.`."_view)) {
        return Expression::Value();
      }
      continue;
    }

    if (cursor.matches(Class::Type::SwizzleOp)) {
      cursor.consume();
      while (!cursor.matches(Class::Type::EndOfStream) &&
             !cursor.matches(Class::Type::IndexEnd)) {
        if (!cursor.require(
                Class::Type::Addressable,
                "Expected swizzle member name."_view)) {
          return Expression::Value();
        }

        if (cursor.matches(Class::Type::PackingOp)) {
          cursor.consume();
          continue;
        }

        if (!cursor.matches(Class::Type::IndexEnd)) {
          cursor.token_error("Expected `,` or `]` after swizzle member."_view);
          return Expression::Value();
        }
      }

      if (!cursor.require(
              Class::Type::IndexEnd, "Expected `]` after swizzle."_view)) {
        return Expression::Value();
      }
      continue;
    }

    if (cursor.is_one_of(expression_index_starts)) {
      if (!consume_index(cursor, context)) {
        return Expression::Value();
      }
      continue;
    }

    if (cursor.matches(Class::Type::CallOp)) {
      cursor.consume();
      if (!cursor.require(
              Class::Type::Addressable,
              "Expected function name after `->`."_view)) {
        return Expression::Value();
      }

      if (cursor.matches(Class::Type::PackingStart) &&
          Expression::Pack::evaluate(cursor, context) == nullptr) {
        return Expression::Value();
      }
      continue;
    }

    break;
  }

  return Expression::Value::reference(
      root, cursor.get_token_span(start, cursor.get_token_index()));
}

static auto evaluate_primary(Cursor& cursor, Context& context)
    -> Expression::Value {
  if (cursor.matches(Class::Type::String)) {
    const Token& token = cursor.consume();
    View::Bytes text = token.get_text();
    if (text.get_size() < 2 || text[0] != '"' ||
        text[text.get_size() - 1] != '"') {
      cursor.range_error(token, token, "Malformed string literal."_view);
      return Expression::Value();
    }

    return Expression::Value::string(EscapedText::decode(
        context.get_arena(), text.slice(1, text.get_size() - 2)));
  }

  if (cursor.matches(Class::Type::Numeric)) {
    return Expression::Value::numeric(cursor.consume().get_text());
  }

  if (cursor.matches(Class::Type::Float)) {
    return Expression::Value::floating(cursor.consume().get_text());
  }

  if (cursor.matches(Class::Type::PackingStart)) {
    const Expression::Pack* pack = Expression::Pack::evaluate(cursor, context);
    return pack == nullptr ? Expression::Value() : Expression::Value::pack(*pack);
  }

  if (cursor.is_one_of(expression_reference_starts)) {
    return evaluate_reference(cursor, context);
  }

  cursor.token_error("Unsupported expression value."_view);
  return Expression::Value();
}

static auto evaluate_expression(
    Cursor& cursor,
    Context& context,
    Count minimum_precedence) -> Expression::Value {
  Expression::Value left = evaluate_primary(cursor, context);
  if (left.is_empty()) {
    return left;
  }

  while (!cursor.matches(Class::Type::EndOfStream)) {
    Expression::Value::Operator op =
        binary_operator(cursor.current().get_class().get_type());
    Count precedence = operator_precedence(op);
    if (precedence < minimum_precedence) {
      break;
    }

    cursor.consume();
    Expression::Value right =
        evaluate_expression(cursor, context, precedence + 1);
    if (right.is_empty()) {
      return Expression::Value();
    }

    const Expression::Value& stable_left =
        context.get_arena().construct<Expression::Value>(left);
    const Expression::Value& stable_right =
        context.get_arena().construct<Expression::Value>(right);
    left = Expression::Value::binary(op, stable_left, stable_right);
  }

  return left;
}

static auto consume_index(Cursor& cursor, Context& context) -> Bool {
  cursor.consume();
  if (!cursor.matches(Class::Type::IndexEnd)) {
    Expression::Value index = evaluate_expression(cursor, context, 1);
    if (index.is_empty()) {
      return False;
    }
  }

  return cursor.require(
             Class::Type::IndexEnd,
             "Expected `]` after expression index."_view) != nullptr;
}

auto Expression::Value::evaluate(Cursor& cursor, Context& context) -> Value {
  return evaluate_expression(cursor, context, 1);
}
