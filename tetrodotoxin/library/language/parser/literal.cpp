// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/parser/literal.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/reader/textual.hpp"

#include "tetrodotoxin/language/error.hpp"
#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/generics/fixed.hpp"
#include "ttx/lexical/lexicon.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;

static constexpr Library::Language::Generics::Fixed fixed_formula;
static constexpr Unsigned_64 signed_maximum = (Unsigned_64(-1) >> 1);
static constexpr Unsigned_64 signed_minimum_magnitude = signed_maximum + 1;
static constexpr Signed_64 signed_minimum =
    Signed_64(-9223372036854775807LL - 1);

static auto parse_unsigned_text(View::Bytes text, Unsigned_8 base)
    -> Option<Unsigned_64> {
  Reader::Textual reader(text);
  Unsigned_64 value = reader.read_unsigned(base);
  if (!reader.is_valid() || reader.get_location() != reader.get_size()) {
    return {};
  }

  return value;
}

static auto parse_real_text(View::Bytes text) -> Option<Real_64> {
  Reader::Textual reader(text);
  Real_64 value = reader.read_real_64();
  if (!reader.is_valid() || reader.get_location() != reader.get_size()) {
    return {};
  }

  return value;
}

static auto materialize_bytes_type(
    Library::Language::Materializations& materializations,
    Cursor& cursor,
    Span span,
    Count size) -> Option<const Type&> {
  if (size > Count(signed_maximum)) {
    cursor.create_expression_error(
        span, "Bytes literal exceeds Library's Fixed extent range."_view,
        "Reduce the byte count below the Signed_64 extent limit."_view);
    return {};
  }

  Static::Vector<Library::Language::Generic::Argument, 2> arguments = {{
    Library::Language::Generic::Argument(Library::Dialect::get_unsigned_8()),
    Library::Language::Generic::Argument(Signed_64(size)),
  }};
  auto materialized =
      materializations.materialize(fixed_formula, arguments.get_view());
  if (!materialized) {
    auto report = cursor.create_report(span);
    report << "Library could not materialize `Fixed[Unsigned_8, "_view
           << Unsigned_64(size) << "]` for this Bytes literal."_view;
    report.get_hint()
        << "Check that Fixed and canonical Unsigned_8 are available."_view;
  }

  return materialized;
}

static auto construct_retained_bytes(
    Allocator::Arena& domain,
    Library::Language::Materializations& materializations,
    Cursor& cursor,
    Span span,
    View::Bytes value) -> Option<const Library::Language::Constant&> {
  auto type =
      materialize_bytes_type(materializations, cursor, span, value.get_size());
  if (!type) {
    return {};
  }

  return domain.construct<Library::Language::Constants::Bytes>(*type, value);
}

static auto parse_quoted(
    Allocator::Arena& domain,
    Library::Language::Materializations& materializations,
    Cursor& cursor) -> Option<const Library::Language::Constant&> {
  Token token = cursor.consume();
  Span span(token);
  View::Bytes text = token.caculate_text(cursor.get_source_text());
  View::Bytes payload = text.slice(1, text.get_size() - 2);
  Count decoded_size = payload.get_size();

  // Escape markers belong to the authored spelling rather than the retained
  // value. Count them before allocation so the Arena receives the exact fact.
  for (Count i = 0; i < payload.get_size(); i++) {
    if (payload[i] == '\\') {
      decoded_size--;
      i++;
    }
  }

  auto decoded = domain.reserve<Unsigned_8>(decoded_size);
  Count output = 0;
  for (Count i = 0; i < payload.get_size(); i++) {
    if (payload[i] == '\\') {
      i++;
    }

    decoded[output] = payload[i];
    output++;
  }

  return construct_retained_bytes(
      domain, materializations, cursor, span,
      View::Bytes(decoded.get_data(), decoded.get_size()));
}

static auto parse_byte_array(
    Allocator::Arena& domain,
    Library::Language::Materializations& materializations,
    Cursor& cursor) -> Option<const Library::Language::Constant&> {
  Token token = cursor.consume();
  Span span(token);
  View::Bytes text = token.caculate_text(cursor.get_source_text());
  View::Bytes payload = text.slice(3, text.get_size() - 4);
  Count digits = 0;

  // Whitespace separates authored digits but contributes no retained byte.
  // Count first so malformed pairs never publish a partial Constant.
  for (Count i = 0; i < payload.get_size(); i++) {
    Unsigned_8 value = payload[i];
    Bool hexadecimal = Lexicon::is_hex(value);
    if (!hexadecimal && !Lexicon::is_whitespace(value)) {
      cursor.create_expression_error(
          span, "Bytes literal contains a non hexadecimal digit."_view,
          "Use hexadecimal pairs containing only 0 through 9 and A through "
          "F."_view);
      return {};
    }

    digits += hexadecimal ? 1 : 0;
  }

  if (digits % 2 != 0) {
    cursor.create_expression_error(
        span, "Bytes literal ends with an incomplete hexadecimal byte."_view,
        "Add or remove one hexadecimal digit so every byte has two digits."_view);
    return {};
  }

  auto decoded = domain.reserve<Unsigned_8>(digits / 2);
  Count nibble = 0;
  Unsigned_8 byte = 0;
  for (Count i = 0; i < payload.get_size(); i++) {
    if (Lexicon::is_whitespace(payload[i])) {
      continue;
    }

    if (nibble % 2 == 0) {
      byte = Unsigned_8(Lexicon::get_hex_value(payload[i]) << 4);
    } else {
      decoded[nibble / 2] = byte | Lexicon::get_hex_value(payload[i]);
    }

    nibble++;
  }

  return construct_retained_bytes(
      domain, materializations, cursor, span,
      View::Bytes(decoded.get_data(), decoded.get_size()));
}

static auto parse_flag(Allocator::Arena& domain, Cursor& cursor)
    -> Option<const Library::Language::Constant&> {
  Token token = cursor.consume();
  const auto& type = Library::Dialect::get_bool();
  if (token.get_code() == Code::Type::True) {
    return domain.construct<Library::Language::Constants::True>(type);
  }

  return domain.construct<Library::Language::Constants::False>(type);
}

static auto parse_integer(
    Allocator::Arena& domain,
    Cursor& cursor,
    Bool negative) -> Option<const Library::Language::Constant&> {
  Token sign = negative ? cursor.consume() : cursor.current();
  Token token = cursor.consume();
  Span span = negative ? Span(sign, token) : Span(token);
  View::Bytes text = token.caculate_text(cursor.get_source_text());
  Unsigned_8 base = token.get_code() == Code::Type::Hex ? 16 : 10;
  if (base == 16) {
    text = text.slice(2, text.get_size() - 2);
  }

  auto magnitude = parse_unsigned_text(text, base);
  if (!magnitude) {
    cursor.create_expression_error(
        span, "Integer literal exceeds Library's 64 bit literal domain."_view,
        "Reduce the magnitude or use a value supplied by another owner."_view);
    return {};
  }

  if (negative) {
    if (*magnitude > signed_minimum_magnitude) {
      cursor.create_expression_error(
          span,
          "Negative integer literal is outside Library Signed_64 range."_view,
          "Use a magnitude no greater than 9223372036854775808 or construct "
          "a wider value through another owner."_view);
      return {};
    }

    Signed_64 value = *magnitude == signed_minimum_magnitude
                          ? signed_minimum
                          : Signed_64(*magnitude);
    if (value != signed_minimum) {
      value = -value;
    }

    return domain.construct<Library::Language::Constants::Signed>(
        Library::Dialect::get_signed_64(), value);
  }

  return domain.construct<Library::Language::Constants::Unsigned>(
      Library::Dialect::get_unsigned_64(), *magnitude);
}

static auto parse_real(Allocator::Arena& domain, Cursor& cursor, Bool negative)
    -> Option<const Library::Language::Constant&> {
  Token sign = negative ? cursor.consume() : cursor.current();
  Token token = cursor.consume();
  Span span = negative ? Span(sign, token) : Span(token);
  View::Bytes text = token.caculate_text(cursor.get_source_text());
  auto parsed = parse_real_text(text);
  if (!parsed) {
    cursor.create_expression_error(
        span, "Real literal is outside Library Real_64's finite range."_view,
        "Use a finite value within Real_64's range or construct it through "
        "another owner."_view);
    return {};
  }

  Real_64 value = negative ? -*parsed : *parsed;
  return domain.construct<Library::Language::Constants::Real>(
      Library::Dialect::get_real_64(), value);
}

static auto parse_embedded(
    Allocator::Arena& domain,
    Library::Language::Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context)
    -> Option<const Library::Language::Constant&> {
  Token token = cursor.consume();
  Span literal_span(token);
  View::Bytes instruction = token.caculate_text(cursor.get_source_text());

  const Abstract& selected =
      source_context.resolve_context(instruction).resolve();
  if (selected.is<Tetrodotoxin::Language::Error>()) {
    auto report = cursor.create_report(literal_span);
    static_cast<const Tetrodotoxin::Language::Error&>(selected).describe(
        report);
    return {};
  }

  if (!selected.is<Tetrodotoxin::Language::Resource>()) {
    cursor.create_expression_error(
        literal_span,
        "Embedded literal did not resolve to a Package Resource."_view,
        "Check the package relative route and confirm the Resource exists."_view);
    return {};
  }

  View::Bytes retained =
      static_cast<const Tetrodotoxin::Language::Resource&>(selected)
          .get_value();

  // Resource keeps its backing stable for the caller domain. Borrow it directly
  // so same domain imports retain one allocation for the semantic island.
  return construct_retained_bytes(
      domain, materializations, cursor, literal_span, retained);
}

auto Library::Language::Parser::Literal::parse(
    Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context) -> Option<const Constant&> {
  Cursor transaction = cursor;
  Bool negative = transaction.matches(Code::Type::SubOp);
  Code code = transaction.get_code();
  if (negative) {
    Cursor probe = transaction;
    probe.consume();
    code = probe.get_code();
  }

  // The probe changes no caller state or durable parser fact. The helpers keep
  // the original transaction so a signed value retains its complete Span.
  Option<const Constant&> parsed;
  if (negative && code == Code::Type::Numeric) {
    parsed = parse_integer(domain, transaction, True);
  } else if (negative && code == Code::Type::Float) {
    parsed = parse_real(domain, transaction, True);
  } else if (negative) {
    transaction.create_token_error(
        "A negative Library literal requires a decimal integer or real."_view,
        "Use decimal spelling after `-` or remove the negative sign."_view);
  } else {
    switch (code.get_type()) {
    case Code::Type::String:
      parsed = parse_quoted(domain, materializations, transaction);
      break;
    case Code::Type::Bytes:
      parsed = parse_byte_array(domain, materializations, transaction);
      break;
    case Code::Type::True:
    case Code::Type::False:
      parsed = parse_flag(domain, transaction);
      break;
    case Code::Type::Numeric:
    case Code::Type::Hex:
      parsed = parse_integer(domain, transaction, False);
      break;
    case Code::Type::Float:
      parsed = parse_real(domain, transaction, False);
      break;
    case Code::Type::Embedded:
      parsed =
          parse_embedded(domain, materializations, transaction, source_context);
      break;
    default:
      transaction.create_token_error(
          "Library literal parser requires a supported literal operand."_view,
          "Use a string, byte array, flag, integer, real, or embedded "
          "Resource literal."_view);
      break;
    }
  }

  if (parsed) {
    cursor.sync(transaction);
  }

  return parsed;
}
