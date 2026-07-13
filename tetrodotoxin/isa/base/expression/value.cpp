// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/base/expression/value.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/escaped_text.hpp"

#include "tetrodotoxin/isa/base/context.hpp"
#include "tetrodotoxin/isa/base/expression/pack.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

static constexpr Static::Vector<Class::Type, 3> expression_reference_starts = {{
  Class::Type::Type,
  Class::Type::Addressable,
  Class::Type::Self,
}};

static constexpr Static::Vector<Class::Type, 2> expression_index_starts = {{
  Class::Type::SliceOp,
  Class::Type::IndexStart,
}};

static auto binary_operator(Class::Type type)
    -> Base::Expression::Value::Operator {
  switch (type) {
  case Class::Type::AddOp:
    return Base::Expression::Value::Operator::Add;
  case Class::Type::SubOp:
    return Base::Expression::Value::Operator::Subtract;
  case Class::Type::MulOp:
    return Base::Expression::Value::Operator::Multiply;
  case Class::Type::DivOp:
    return Base::Expression::Value::Operator::Divide;
  case Class::Type::ModOp:
    return Base::Expression::Value::Operator::Remainder;
  case Class::Type::CmpOp:
    return Base::Expression::Value::Operator::Equal;
  default:
    return Base::Expression::Value::Operator::None;
  }
}

static auto operator_precedence(Base::Expression::Value::Operator op) -> Count {
  switch (op) {
  case Base::Expression::Value::Operator::Multiply:
  case Base::Expression::Value::Operator::Divide:
  case Base::Expression::Value::Operator::Remainder:
    return 3;
  case Base::Expression::Value::Operator::Add:
  case Base::Expression::Value::Operator::Subtract:
    return 2;
  case Base::Expression::Value::Operator::Equal:
    return 1;
  default:
    return 0;
  }
}

static auto consume_index(Cursor& cursor, Base::Context& context) -> Bool;

static auto evaluate_expression(
    Cursor& cursor,
    Base::Context& context,
    Count minimum_precedence) -> Base::Expression::Value;

static auto evaluate_reference(Cursor& cursor, Base::Context& context)
    -> Base::Expression::Value {
  const Count start = cursor.get_token_index();
  View::Bytes root = cursor.consume().get_text();
  while (!cursor.matches(Class::Type::EndOfStream)) {
    if (cursor.matches(Class::Type::AddressOp)) {
      cursor.consume();
      if (!cursor.require(
              Class::Type::Addressable,
              "Expected member name after `.`."_view)) {
        return Base::Expression::Value();
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
          return Base::Expression::Value();
        }

        if (cursor.matches(Class::Type::PackingOp)) {
          cursor.consume();
          continue;
        }

        if (!cursor.matches(Class::Type::IndexEnd)) {
          cursor.token_error("Expected `,` or `]` after swizzle member."_view);
          return Base::Expression::Value();
        }
      }

      if (!cursor.require(
              Class::Type::IndexEnd, "Expected `]` after swizzle."_view)) {
        return Base::Expression::Value();
      }

      continue;
    }

    if (cursor.is_one_of(expression_index_starts)) {
      if (!consume_index(cursor, context)) {
        return Base::Expression::Value();
      }

      continue;
    }

    if (cursor.matches(Class::Type::CallOp)) {
      const Base::Expression::Value& owner =
          context.get_arena().construct<Base::Expression::Value>(
              Base::Expression::Value::reference(
                  root,
                  cursor.get_token_span(start, cursor.get_token_index())));
      cursor.consume();
      const Token* name = cursor.require(
          Class::Type::Addressable, "Expected function name after `->`."_view);
      if (name == nullptr) {
        return Base::Expression::Value();
      }

      const Base::Expression::Pack* arguments =
          Base::Expression::Pack::evaluate(cursor, context);
      if (arguments == nullptr) {
        return Base::Expression::Value();
      }

      return Base::Expression::Value::call(owner, name->get_text(), *arguments);
    }

    break;
  }

  return Base::Expression::Value::reference(
      root, cursor.get_token_span(start, cursor.get_token_index()));
}

static auto evaluate_primary(Cursor& cursor, Base::Context& context)
    -> Base::Expression::Value {
  if (cursor.matches(Class::Type::String)) {
    const Token& token = cursor.consume();
    View::Bytes text = token.get_text();
    if (text.get_size() < 2 || text[0] != '"' ||
        text[text.get_size() - 1] != '"') {
      cursor.range_error(token, token, "Malformed string literal."_view);
      return Base::Expression::Value();
    }

    return Base::Expression::Value::string(
        EscapedText::decode(
            context.get_arena(), text.slice(1, text.get_size() - 2)));
  }

  if (cursor.matches(Class::Type::Bytes)) {
    const Token& token = cursor.consume();
    View::Bytes text = token.get_text();
    if (text.get_size() < 4 || text[0] != '0' || text[1] != 'x' ||
        text[2] != '[' || text[text.get_size() - 1] != ']') {
      cursor.range_error(token, token, "Malformed byte literal."_view);
      return Base::Expression::Value();
    }

    Managed::Bytes bytes(context.get_arena());
    Bits_8 byte = 0;
    Bool high = True;
    for (Count i = 3; i + 1 < text.get_size(); i++) {
      Bits_8 c = text[i];
      if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
        continue;
      }

      Bits_8 nibble = c >= '0' && c <= '9'   ? c - '0'
                      : c >= 'a' && c <= 'f' ? c - 'a' + 10
                      : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                             : 0xFF;
      if (nibble == 0xFF) {
        cursor.range_error(token, token, "Malformed byte literal."_view);
        return Base::Expression::Value();
      }

      if (high) {
        byte = Bits_8(nibble << 4);
      } else {
        bytes.append(Bits_8(byte | nibble));
      }

      high = !high;
    }

    if (!high) {
      cursor.range_error(
          token, token, "Byte literal has an incomplete byte."_view);
      return Base::Expression::Value();
    }

    return Base::Expression::Value::bytes(bytes.get_view());
  }

  if (cursor.matches(Class::Type::Embedded)) {
    const Token& token = cursor.consume();
    View::Bytes text = token.get_text();
    if (text.get_size() < 4 || text[0] != '$' || text[1] != '[' ||
        text[text.get_size() - 1] != ']') {
      cursor.range_error(token, token, "Malformed embedded file literal."_view);
      return Base::Expression::Value();
    }

    View::Bytes content;
    if (!context.read_embedded(
            cursor.get_source_name(), text.slice(2, text.get_size() - 3),
            content)) {
      cursor.range_error(token, token, "Embedded file could not be read."_view);
      return Base::Expression::Value();
    }

    return Base::Expression::Value::bytes(content);
  }

  if (cursor.matches(Class::Type::True)) {
    cursor.consume();
    return Base::Expression::Value::boolean(True);
  }

  if (cursor.matches(Class::Type::False)) {
    cursor.consume();
    return Base::Expression::Value::boolean(False);
  }

  if (cursor.matches(Class::Type::Numeric)) {
    return Base::Expression::Value::numeric(cursor.consume().get_text());
  }

  if (cursor.matches(Class::Type::Float)) {
    return Base::Expression::Value::floating(cursor.consume().get_text());
  }

  if (cursor.matches(Class::Type::SubOp)) {
    const Token& sign = cursor.consume();
    if (cursor.matches(Class::Type::Numeric)) {
      return Base::Expression::Value::numeric(
          cursor.consume().get_text(), True);
    }

    if (cursor.matches(Class::Type::Float)) {
      return Base::Expression::Value::floating(
          cursor.consume().get_text(), True);
    }

    cursor.range_error(
        sign, sign, "Expected a numeric literal after `-`."_view);
    return Base::Expression::Value();
  }

  if (cursor.matches(Class::Type::PackingStart)) {
    const Base::Expression::Pack* pack =
        Base::Expression::Pack::evaluate(cursor, context);
    return pack == nullptr ? Base::Expression::Value()
                           : Base::Expression::Value::pack(*pack);
  }

  if (cursor.is_one_of(expression_reference_starts)) {
    return evaluate_reference(cursor, context);
  }

  cursor.token_error("Unsupported expression value."_view);
  return Base::Expression::Value();
}

static auto evaluate_expression(
    Cursor& cursor,
    Base::Context& context,
    Count minimum_precedence) -> Base::Expression::Value {
  Base::Expression::Value left = evaluate_primary(cursor, context);
  if (left.is_empty()) {
    return left;
  }

  while (!cursor.matches(Class::Type::EndOfStream)) {
    Base::Expression::Value::Operator op =
        binary_operator(cursor.current().get_class().get_type());
    Count precedence = operator_precedence(op);
    if (precedence < minimum_precedence) {
      break;
    }

    cursor.consume();
    Base::Expression::Value right =
        evaluate_expression(cursor, context, precedence + 1);
    if (right.is_empty()) {
      return Base::Expression::Value();
    }

    const Base::Expression::Value& stable_left =
        context.get_arena().construct<Base::Expression::Value>(left);
    const Base::Expression::Value& stable_right =
        context.get_arena().construct<Base::Expression::Value>(right);
    left = Base::Expression::Value::binary(op, stable_left, stable_right);
  }

  return left;
}

static auto consume_index(Cursor& cursor, Base::Context& context) -> Bool {
  cursor.consume();
  if (!cursor.matches(Class::Type::IndexEnd)) {
    Base::Expression::Value index = evaluate_expression(cursor, context, 1);
    if (index.is_empty()) {
      return False;
    }
  }

  return cursor.require(
             Class::Type::IndexEnd,
             "Expected `]` after expression index."_view) != nullptr;
}

auto Base::Expression::Value::evaluate(Cursor& cursor, Base::Context& context)
    -> Base::Expression::Value {
  return evaluate_expression(cursor, context, 1);
}
