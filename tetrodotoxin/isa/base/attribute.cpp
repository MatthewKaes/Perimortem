// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/base/attribute.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/reader/textual.hpp"

#include "perimortem/utility/table.hpp"

#include "tetrodotoxin/abi/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

constexpr Static::Vector<Pair<View::Bytes, Abi::Lowering>, 6>
    abi_lowering_names = {{
      {"void"_view, Abi::Lowering::Void},
      {"bool"_view, Abi::Lowering::Bool},
      {"integer"_view, Abi::Lowering::Integer},
      {"signed"_view, Abi::Lowering::Signed},
      {"real"_view, Abi::Lowering::Real},
      {"view_bytes"_view, Abi::Lowering::ViewBytes},
    }};

static auto attribute_key(View::Bytes source) -> View::Bytes {
  return !source.is_empty() && source[0] == '@' ? source.slice(1) : source;
}

static auto read_scalar(Cursor& cursor, View::Bytes key) -> Ttx::Attribute {
  if (!cursor.matches(Code::Type::PackingStart)) {
    return Ttx::Attribute(key);
  }

  cursor.consume();
  Bool negative = cursor.matches(Code::Type::SubOp);
  if (negative) {
    cursor.consume();
  }

  Ttx::Attribute::Kind kind = Ttx::Attribute::Kind::Empty;
  View::Bytes bytes;
  Unsigned_64 unsigned_value = 0;
  Signed_64 signed_value = 0;
  Real_64 real_value = 0;
  Bool boolean_value = False;
  const Token& token = cursor.current();
  switch (token.get_code().get_type()) {
  case Code::Type::Addressable:
  case Code::Type::Type:
  case Code::Type::String:
    if (negative) {
      cursor.token_error("Only numeric attribute values can be negative."_view);
      return Ttx::Attribute();
    }

    kind = Ttx::Attribute::Kind::Bytes;
    bytes = token.get_text();
    break;

  case Code::Type::Numeric: {
    Reader::Textual reader(token.get_text());
    Unsigned_64 value = reader.read_unsigned();
    if (reader.get_location() != reader.get_size()) {
      cursor.token_error("Attribute integer could not be decoded."_view);
      return Ttx::Attribute();
    }

    if (negative) {
      kind = Ttx::Attribute::Kind::Signed;
      signed_value = -Signed_64(value);
    } else {
      kind = Ttx::Attribute::Kind::Unsigned;
      unsigned_value = value;
    }

    break;
  }

  case Code::Type::Float: {
    Reader::Textual reader(token.get_text());
    Real_64 value = reader.read_real_64();
    if (reader.get_location() != reader.get_size()) {
      cursor.token_error("Attribute real could not be decoded."_view);
      return Ttx::Attribute();
    }

    kind = Ttx::Attribute::Kind::Real;
    real_value = negative ? -value : value;
    break;
  }

  case Code::Type::True:
  case Code::Type::False:
    if (negative) {
      cursor.token_error("Only numeric attribute values can be negative."_view);
      return Ttx::Attribute();
    }

    kind = Ttx::Attribute::Kind::Boolean;
    boolean_value = token.get_code() == Code::Type::True;
    break;

  default:
    cursor.token_error(
        "Expected one scalar attribute value or no argument list."_view,
        "Use another attribute instead of a structured value."_view);
    return Ttx::Attribute();
  }

  cursor.consume();
  if (!cursor.matches(Code::Type::PackingEnd)) {
    cursor.token_error(
        "Attribute accepts exactly one scalar value."_view,
        "Use another attribute for each additional fact."_view);
    return Ttx::Attribute();
  }

  cursor.consume();
  switch (kind) {
  case Ttx::Attribute::Kind::Bytes:
    return Ttx::Attribute(key, bytes);
  case Ttx::Attribute::Kind::Unsigned:
    return Ttx::Attribute(key, unsigned_value);
  case Ttx::Attribute::Kind::Signed:
    return Ttx::Attribute(key, signed_value);
  case Ttx::Attribute::Kind::Real:
    return Ttx::Attribute(key, real_value);
  case Ttx::Attribute::Kind::Boolean:
    return Ttx::Attribute(key, boolean_value);
  case Ttx::Attribute::Kind::Empty:
    return Ttx::Attribute(key);
  }

  return Ttx::Attribute();
}

static auto read_attribute(Cursor& cursor) -> Ttx::Attribute {
  const Token* token =
      cursor.require(Code::Type::Attribute, "Expected attribute."_view);
  if (token == nullptr) {
    return Ttx::Attribute();
  }

  View::Bytes key = attribute_key(token->get_text());
  Ttx::Attribute attribute = read_scalar(cursor, key);
  if (attribute.is_empty() || key != "abi"_view) {
    return attribute;
  }

  Abi::Lowering lowering =
      Table<Abi::Lowering, abi_lowering_names>::find_or_default(
          attribute.get_bytes(), Abi::Lowering::Invalid);
  if (lowering == Abi::Lowering::Invalid) {
    cursor.token_error(
        "Unknown ABI lowering. Expected one of "
        "[void, bool, integer, signed, real, view_bytes]."_view);
    return Ttx::Attribute();
  }

  return Ttx::Attribute(key, Unsigned_64(lowering));
}

auto Base::Attribute::consume_all(Cursor& cursor) -> Bool {
  while (cursor.matches(Code::Type::Attribute)) {
    Ttx::Attribute attribute = read_attribute(cursor);
    if (attribute.is_empty()) {
      return False;
    }
  }

  return True;
}

auto Base::Attribute::evaluate_all(
    Cursor& cursor,
    Perimortem::Memory::Managed::Vector<Ttx::Attribute>& attributes) -> Bool {
  while (cursor.matches(Code::Type::Attribute)) {
    Ttx::Attribute attribute = read_attribute(cursor);
    if (attribute.is_empty()) {
      return False;
    }

    attributes.insert(attribute);
  }

  return True;
}

auto Base::Attribute::append_all(
    View::Vector<Ttx::Attribute> source,
    Perimortem::Memory::Managed::Vector<Ttx::Attribute>& target) -> void {
  for (Count i = 0; i < source.get_size(); i++) {
    target.insert(source[i]);
  }
}
