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

static auto materialize_bytes_type(
    Library::Language::Materializations& materializations,
    Cursor& cursor,
    Span span,
    Count size) -> Option<const Type&> {
  // Keep the max array length the same as what the Bibliotheca can manage.
  constexpr Unsigned_64 max_extent = Unsigned_64(1) << 36;
  if (size > Count(max_extent)) {
    cursor.create_expression_error(
        span, "Bytes literal exceeds Library's Fixed extent range."_view,
        "Reduce the byte count below the 68,719,476,736 extent limit."_view);
    return {};
  }

  Static::Vector<Library::Language::Generic::Argument, 2> arguments = {{
    Library::Language::Generic::Argument(Library::Dialect::get_unsigned_8()),
    Library::Language::Generic::Argument(Unsigned_64(size)),
  }};
  auto materialized = materializations.materialize(
      Library::Language::Generics::Fixed::get_formula(), arguments.get_view());
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
    View::Bytes value) -> Option<Library::Language::Constant&> {
  auto type =
      materialize_bytes_type(materializations, cursor, span, value.get_size());
  return type.visit(
      []() -> Option<Library::Language::Constant&> { return {}; },
      [&](const Ttx::Model::Type& type)
          -> Option<Library::Language::Constant&> {
        Anchor anchor = Anchor::create(span.get_start(), span);

        cursor.consume();
        return Library::Language::Constants::Bytes::create_authored(
            domain, type, value, anchor);
      });
}

static auto parse_quoted(
    Allocator::Arena& domain,
    Library::Language::Materializations& materializations,
    Cursor& cursor) -> Option<Library::Language::Constant&> {
  Span literal_span(cursor.current());
  View::Bytes text = literal_span.caculate_text(cursor.get_source_text());
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

  auto decoded = domain.allocate(decoded_size);
  auto* decoded_data = decoded.get_data();
  Count output = 0;
  for (Count i = 0; i < payload.get_size(); i++) {
    if (payload[i] == '\\') {
      i++;
    }

    decoded_data[output] = payload[i];
    output++;
  }

  return construct_retained_bytes(
      domain, materializations, cursor, literal_span,
      View::Bytes(decoded.get_data(), decoded.get_size()));
}

static auto parse_byte_array(
    Allocator::Arena& domain,
    Library::Language::Materializations& materializations,
    Cursor& cursor) -> Option<Library::Language::Constant&> {
  Span literal_span(cursor.current());
  View::Bytes text = literal_span.caculate_text(cursor.get_source_text());
  View::Bytes payload = text.slice(3, text.get_size() - 4);
  Count digits = 0;

  // Whitespace separates authored digits but contributes no retained byte.
  // Count first so malformed pairs never publish a partial Constant.
  for (Count i = 0; i < payload.get_size(); i++) {
    Unsigned_8 value = payload[i];
    Bool hexadecimal = Lexicon::is_hex(value);
    if (!hexadecimal && !Lexicon::is_whitespace(value)) {
      cursor.create_expression_error(
          literal_span, "Bytes literal contains a non hexadecimal digit."_view,
          "Use hexadecimal pairs containing only 0 through 9 and A through "
          "F."_view);
      return {};
    }

    digits += hexadecimal ? 1 : 0;
  }

  if (digits % 2 != 0) {
    cursor.create_expression_error(
        literal_span,
        "Bytes literal ends with an incomplete hexadecimal byte."_view,
        "Add or remove one hexadecimal digit so every byte has two digits."_view);
    return {};
  }

  auto decoded = domain.allocate(digits / 2);
  auto* decoded_data = decoded.get_data();
  Count nibble = 0;
  Unsigned_8 byte = 0;
  for (Count i = 0; i < payload.get_size(); i++) {
    if (Lexicon::is_whitespace(payload[i])) {
      continue;
    }

    if (nibble % 2 == 0) {
      byte = Unsigned_8(Lexicon::get_hex_value(payload[i]) << 4);
    } else {
      decoded_data[nibble / 2] = byte | Lexicon::get_hex_value(payload[i]);
    }

    nibble++;
  }

  return construct_retained_bytes(
      domain, materializations, cursor, literal_span,
      View::Bytes(decoded.get_data(), decoded.get_size()));
}

static auto parse_embedded(
    Allocator::Arena& domain,
    Library::Language::Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context) -> Option<Library::Language::Constant&> {
  Span literal_span(cursor.current());
  View::Bytes route = literal_span.caculate_text(cursor.get_source_text());

  const Abstract& selected = source_context.resolve_context(route).resolve();
  auto error = selected.select<Tetrodotoxin::Language::Error>();
  if (error) {
    auto report = cursor.create_report(literal_span);
    error->describe(report);
    return {};
  }

  auto resource = selected.select<Tetrodotoxin::Language::Resource>();
  if (!resource) {
    cursor.create_expression_error(
        literal_span,
        "Embedded literal did not resolve to a Package Resource."_view,
        "Check the package relative route and confirm the Resource exists."_view);
    return {};
  }

  View::Bytes retained = resource->get_value();

  // Resource keeps its backing stable for the caller domain. Borrow it directly
  // so same domain imports retain one allocation for the semantic island.
  return construct_retained_bytes(
      domain, materializations, cursor, literal_span, retained);
}

// Tokenization has already selected the Flag domain. Literal therefore uses
// the Code directly and introduces no second truth spelling policy.
static auto parse_flag(Allocator::Arena& domain, Cursor& cursor)
    -> Option<Library::Language::Constant&> {
  Token token = cursor.consume();
  Anchor anchor = Anchor::create(token, Span(token));

  const auto& type = Library::Dialect::get_bool();
  if (token.get_code() == Code::Type::True) {
    return Library::Language::Constants::True::create_authored(
        domain, type, anchor);
  }

  return Library::Language::Constants::False::create_authored(
      domain, type, anchor);
}

template <Count radix>
static auto parse_unsigned(Allocator::Arena& domain, Cursor& cursor)
    -> Option<Library::Language::Constant&> {
  Span literal_text(cursor.current());
  Reader::Textual reader(literal_text.caculate_text(cursor.get_source_text())
                             .slice(radix == 16 ? 2 : 0));

  // Textual must consume the complete Token payload. Accepting a valid prefix
  // would publish a different value for malformed authored bytes.
  Unsigned_64 value = reader.read_unsigned(radix);
  if (!reader.is_valid() || reader.get_location() != reader.get_size()) {
    cursor.create_expression_error(
        literal_text, "Unable to parse unsigned literal value."_view);
    return {};
  }

  // Consumption follows complete validation so failure leaves the transaction
  // at the literal that needs the diagnostic.
  Anchor anchor = Anchor::create(literal_text.get_start(), literal_text);

  cursor.consume();
  return Library::Language::Constants::Unsigned::create_authored(
      domain, Library::Dialect::get_unsigned_64(), value, anchor);
}

static auto parse_signed(Allocator::Arena& domain, Cursor& cursor)
    -> Option<Library::Language::Constant&> {
  Span literal_text(cursor.current(), cursor.peek(1));
  Reader::Textual reader(literal_text.caculate_text(cursor.get_source_text()));

  // The leading sign and digits form one semantic value even though the Lexer
  // exposes two Tokens. Textual must reject any unconsumed authored bytes.
  Signed_64 value = reader.read_signed();
  if (!reader.is_valid() || reader.get_location() != reader.get_size()) {
    cursor.create_expression_error(
        literal_text, "Unable to parse signed literal value."_view);
    return {};
  }

  // Both Tokens become durable progress only after the complete value parses.
  Anchor anchor = Anchor::create(literal_text.get_start(), literal_text);

  cursor.consume();
  cursor.consume();
  return Library::Language::Constants::Signed::create_authored(
      domain, Library::Dialect::get_signed_64(), value, anchor);
}

template <Signed_64 token_width>
static auto parse_real(Allocator::Arena& domain, Cursor& cursor)
    -> Option<Library::Language::Constant&> {
  Span literal_text(cursor.current(), cursor.peek(token_width - 1));
  Reader::Textual reader(literal_text.caculate_text(cursor.get_source_text()));

  // Textual sees the complete signed or unsigned spelling so partial numeric
  // acceptance cannot change the Constant represented by the source.
  Real_64 value = reader.read_real_64();
  if (!reader.is_valid() || reader.get_location() != reader.get_size()) {
    cursor.create_expression_error(
        literal_text, "Unable to parse real literal value."_view);
    return {};
  }

  // A negative Real owns its sign Token too. Consume the exact lexical width
  // only after validation preserves one atomic literal transaction.
  Anchor anchor = Anchor::create(literal_text.get_start(), literal_text);

  for (Signed_64 i = 0; i < token_width; i++) {
    cursor.consume();
  }

  return Library::Language::Constants::Real::create_authored(
      domain, Library::Dialect::get_real_64(), value, anchor);
}

auto Library::Language::Parser::Literal::parse(
    Allocator::Arena& domain,
    Library::Language::Monograph& source,
    Cursor& cursor) -> Option<Constant&> {
  auto& materializations = source.get_materializations();
  const Abstract& source_context = source.get_interpretation_context();

  // A leading subtraction spelling admits only signed decimal and Real
  // literals. Without it the ordinary unsigned parser keeps its full domain.
  if (cursor.matches(Code::Type::SubOp)) {
    switch (cursor.peek(1).get_code().get_type()) {
    case Code::Type::Numeric:
      return parse_signed(domain, cursor);
    case Code::Type::Float:
      return parse_real<2>(domain, cursor);
    default:
      cursor.create_expression_error(
          Span(cursor.current(), cursor.peek(1)),
          "A negative literal requires a decimal integer or real."_view,
          "Use decimal spelling after `-` or remove the negative sign."_view);
      return {};
    }
  }

  // The probe changes no caller state or durable parser fact. The helpers keep
  // the original transaction so a signed value retains its complete Span.
  switch (cursor.get_code().get_type()) {
  case Code::Type::String:
    return parse_quoted(domain, materializations, cursor);
  case Code::Type::Bytes:
    return parse_byte_array(domain, materializations, cursor);
  case Code::Type::True:
  case Code::Type::False:
    return parse_flag(domain, cursor);
  case Code::Type::Numeric:
    return parse_unsigned<10>(domain, cursor);
  case Code::Type::Hex:
    return parse_unsigned<16>(domain, cursor);
  case Code::Type::Float:
    return parse_real<1>(domain, cursor);
  case Code::Type::Embedded:
    return parse_embedded(domain, materializations, cursor, source_context);
  default:
    cursor.create_token_error(
        "Library literal parser requires a supported literal operand."_view,
        "Use a string, byte array, flag, integer, real, or embedded "
        "Resource literal."_view);
    return {};
  }
}
