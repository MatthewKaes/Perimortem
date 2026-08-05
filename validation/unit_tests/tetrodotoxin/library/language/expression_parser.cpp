// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness ExpressionParserTests = {
  .name = "Tetrodotoxin::Library::Language::Expression parser"_view,
};

class ExpressionParserResource : public Tetrodotoxin::Language::Resource {
 public:
  ExpressionParserResource(Allocator::Arena& domain, View::Bytes value)
      : value(domain.proxy(value)) {}

  auto get_value() const -> View::Bytes override { return value; }

 private:
  View::Bytes value;
};

class ExpressionParserContext : public Abstract {
 public:
  ExpressionParserContext(Allocator::Arena& domain)
      : table(domain, "0123456789"_view) {}

  auto get_name() const -> View::Bytes override { return "Context"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    if (route == "$[table]"_view) {
      table_seen = True;
      return table;
    }

    return Ttx::Concept::Invalid::get_invalid();
  }

  mutable Bool table_seen = False;
  ExpressionParserResource table;

  auto get_table_value() const -> View::Bytes { return table.get_value(); }
};

static auto matches_token(const Cursor& cursor, Token expected) -> Bool {
  Token current = cursor.current();
  return current.get_offset() == expected.get_offset() &&
         current.get_code() == expected.get_code();
}

static auto parse_one(
    Allocator::Arena& domain,
    Library::Language::Materializations& materializations,
    const Abstract& context,
    View::Bytes source,
    Errors& errors) -> Option<const Library::Language::Expression&> {
  Tokenizer tokenizer(domain, source, "expression-parser.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto parsed = Library::Language::Parser::Expression::parse(
      domain, materializations, cursor, context);
  if (parsed && !cursor.matches(Code::Type::Terminal)) {
    return {};
  }

  return parsed;
}

static auto rejects(
    Allocator::Arena& domain,
    Library::Language::Materializations& materializations,
    const Abstract& context,
    View::Bytes source,
    View::Bytes operation = ":["_view) -> Bool {
  Errors errors;
  Tokenizer tokenizer(domain, source, "rejected-expression.ttx"_view);
  Cursor cursor(tokenizer, errors);
  Token start = cursor.current();
  auto parsed = Library::Language::Parser::Expression::parse(
      domain, materializations, cursor, context);
  if (parsed || !matches_token(cursor, start) || errors.get_size() != 1) {
    return False;
  }

  Count postfix = Algorithm::search(source, operation);
  if (postfix == Count(-1)) {
    return False;
  }

  Allocator::Arena render_arena;
  View::Bytes rendered = errors.render_message(render_arena, 0);
  Count caret = Algorithm::search(rendered, "^"_view);
  if (caret == Count(-1)) {
    return False;
  }

  Count width = 1;
  while (caret + width < rendered.get_size() &&
         rendered[caret + width] == '-') {
    width++;
  }

  return width == source.get_size() - postfix;
}

static auto render_rejection(
    Allocator::Arena& domain,
    Library::Language::Materializations& materializations,
    const Abstract& context,
    View::Bytes source,
    Allocator::Arena& rendering) -> View::Bytes {
  Errors errors;
  Tokenizer tokenizer(domain, source, "rejected-expression.ttx"_view);
  Cursor cursor(tokenizer, errors);
  Token start = cursor.current();
  auto parsed = Library::Language::Parser::Expression::parse(
      domain, materializations, cursor, context);
  if (parsed || !matches_token(cursor, start) || errors.get_size() != 1) {
    return {};
  }

  return errors.render_message(rendering, 0);
}

static auto contains(View::Bytes value, View::Bytes expected) -> Bool {
  return Algorithm::search(value, expected) != Count(-1);
}

template <typename selected_type>
static auto select_abstract(const Abstract& value)
    -> Option<const selected_type&> {
  return value.visit<selected_type>(
      [](const selected_type& selected) -> Option<const selected_type&> {
        return selected;
      },
      [](const Abstract&) -> Option<const selected_type&> { return {}; });
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, delegation_and_index) {
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  ExpressionParserContext context(domain);
  Errors unsliced_errors;
  Errors same_type_errors;
  Errors first_errors;
  Errors last_errors;
  Errors signed_zero_errors;

  auto unsliced = parse_one(
      domain, materializations, context, "\"abcd\""_view, unsliced_errors);
  auto same_type = parse_one(
      domain, materializations, context, "\"abcdef\":[1, 4]"_view,
      same_type_errors);
  auto first = parse_one(
      domain, materializations, context, "\"AB\":[0]"_view, first_errors);
  auto last = parse_one(
      domain, materializations, context, "0x[00 FF]:[1]"_view, last_errors);
  auto signed_zero = parse_one(
      domain, materializations, context, "0x[7F]:[-0]"_view,
      signed_zero_errors);
  auto unsliced_bytes =
      unsliced ? select_abstract<Library::Language::Constants::Bytes>(*unsliced)
               : Option<const Library::Language::Constants::Bytes&>();
  auto first_value =
      first ? select_abstract<Library::Language::Constants::Unsigned>(*first)
            : Option<const Library::Language::Constants::Unsigned&>();
  auto last_value =
      last ? select_abstract<Library::Language::Constants::Unsigned>(*last)
           : Option<const Library::Language::Constants::Unsigned&>();
  auto signed_zero_value =
      signed_zero ? select_abstract<Library::Language::Constants::Unsigned>(
                        *signed_zero)
                  : Option<const Library::Language::Constants::Unsigned&>();

  ASSERT(unsliced && same_type && first && last && signed_zero);
  ASSERT(unsliced_bytes && first_value && last_value && signed_zero_value);
  EXPECT(unsliced->is<Library::Language::Constant>());
  EXPECT(unsliced->is<Library::Language::Constants::Bytes>());
  EXPECT(unsliced->get_inputs().is_empty());
  EXPECT(&unsliced->get_type() == &same_type->get_type());
  EXPECT_TEXT(unsliced_bytes->get_value(), "abcd"_view);
  EXPECT(first_value->get_value() == Unsigned_64('A'));
  EXPECT(last_value->get_value() == Unsigned_64(255));
  EXPECT(signed_zero_value->get_value() == Unsigned_64(0x7F));
  EXPECT(&first->get_type() == &Library::Dialect::get_unsigned_8());
  EXPECT(&last->get_type() == &Library::Dialect::get_unsigned_8());
  EXPECT(&signed_zero->get_type() == &Library::Dialect::get_unsigned_8());
  EXPECT(unsliced_errors.is_empty());
  EXPECT(same_type_errors.is_empty());
  EXPECT(first_errors.is_empty());
  EXPECT(last_errors.is_empty());
  EXPECT(signed_zero_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, slice_shapes) {
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  ExpressionParserContext context(domain);
  Errors full_errors;
  Errors interior_errors;
  Errors empty_errors;
  Errors chained_errors;
  Errors embedded_errors;
  Errors recursive_errors;

  auto full = parse_one(
      domain, materializations, context, "\"abcd\":[0, 4]"_view, full_errors);
  auto interior = parse_one(
      domain, materializations, context, "0x[00 01 02 03]:[1, 2]"_view,
      interior_errors);
  auto empty = parse_one(
      domain, materializations, context, "\"abcd\":[4, 0]"_view, empty_errors);
  auto chained = parse_one(
      domain, materializations, context, "\"abcdef\":[1, 4]:[1, 2]"_view,
      chained_errors);
  auto embedded = parse_one(
      domain, materializations, context, "$[table]:[2, 4]"_view,
      embedded_errors);
  auto recursive = parse_one(
      domain, materializations, context,
      "\"abcdef\":[0x[01]:[0], 0x[02]:[0]]"_view, recursive_errors);
  auto full_bytes =
      full ? select_abstract<Library::Language::Constants::Bytes>(*full)
           : Option<const Library::Language::Constants::Bytes&>();
  auto interior_bytes =
      interior ? select_abstract<Library::Language::Constants::Bytes>(*interior)
               : Option<const Library::Language::Constants::Bytes&>();
  auto empty_bytes =
      empty ? select_abstract<Library::Language::Constants::Bytes>(*empty)
            : Option<const Library::Language::Constants::Bytes&>();
  auto chained_bytes =
      chained ? select_abstract<Library::Language::Constants::Bytes>(*chained)
              : Option<const Library::Language::Constants::Bytes&>();
  auto embedded_bytes =
      embedded ? select_abstract<Library::Language::Constants::Bytes>(*embedded)
               : Option<const Library::Language::Constants::Bytes&>();
  auto recursive_bytes =
      recursive
          ? select_abstract<Library::Language::Constants::Bytes>(*recursive)
          : Option<const Library::Language::Constants::Bytes&>();
  auto full_type =
      full ? select_abstract<Library::Language::Types::Fixed>(full->get_type())
           : Option<const Library::Language::Types::Fixed&>();
  auto empty_type =
      empty
          ? select_abstract<Library::Language::Types::Fixed>(empty->get_type())
          : Option<const Library::Language::Types::Fixed&>();
  auto chained_type = chained
                          ? select_abstract<Library::Language::Types::Fixed>(
                                chained->get_type())
                          : Option<const Library::Language::Types::Fixed&>();
  View::Bytes interior_value =
      interior_bytes ? interior_bytes->get_value() : View::Bytes();
  View::Bytes embedded_value =
      embedded_bytes ? embedded_bytes->get_value() : View::Bytes();

  ASSERT(full && interior && empty && chained && embedded && recursive);
  ASSERT(
      full_bytes && interior_bytes && empty_bytes && chained_bytes &&
      embedded_bytes && recursive_bytes && full_type && empty_type &&
      chained_type);
  EXPECT_TEXT(full_bytes->get_value(), "abcd"_view);
  ASSERT_EQ(interior_value.get_size(), Count(2));
  EXPECT(interior_value[0] == 1);
  EXPECT(interior_value[1] == 2);
  EXPECT(empty_bytes->get_value().is_empty());
  EXPECT_TEXT(chained_bytes->get_value(), "cd"_view);
  EXPECT_TEXT(embedded_bytes->get_value(), "2345"_view);
  EXPECT_TEXT(recursive_bytes->get_value(), "bc"_view);
  EXPECT(context.table_seen);
  EXPECT(embedded_value.get_data() == context.get_table_value().get_data() + 2);
  EXPECT(embedded->get_inputs().is_empty());

  EXPECT(full_type->get_extent() == 4);
  EXPECT(empty_type->get_extent() == 0);
  EXPECT(chained_type->get_extent() == 2);
  EXPECT(&full_type->get_element_type() == &Library::Dialect::get_unsigned_8());
  EXPECT(
      &empty_type->get_element_type() == &Library::Dialect::get_unsigned_8());
  EXPECT(
      &chained_type->get_element_type() == &Library::Dialect::get_unsigned_8());
  EXPECT(full_errors.is_empty());
  EXPECT(interior_errors.is_empty());
  EXPECT(empty_errors.is_empty());
  EXPECT(chained_errors.is_empty());
  EXPECT(embedded_errors.is_empty());
  EXPECT(recursive_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, failure_atomicity) {
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  ExpressionParserContext context(domain);

  EXPECT(rejects(domain, materializations, context, "\"abc\":[-1]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[1.0]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[-1, 1]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[true, 1]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[0, -1]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[0, false]"_view));
  EXPECT(rejects(
      domain, materializations, context,
      "\"abc\":[18446744073709551616]"_view));
  EXPECT(rejects(
      domain, materializations, context,
      "\"abc\":[0, 18446744073709551615]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[3]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[4, 0]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[2, 2]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[1 1]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[1,]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[1, 1"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[,1]"_view));
  EXPECT(rejects(domain, materializations, context, "true:[0]"_view));
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, diagnostic_facts) {
  Allocator::Arena domain;
  Allocator::Arena rendering;
  Library::Language::Materializations materializations(domain);
  ExpressionParserContext context(domain);

  View::Bytes receiver = render_rejection(
      domain, materializations, context, "true:[0]"_view, rendering);
  View::Bytes operand = render_rejection(
      domain, materializations, context, "\"abc\":[true]"_view, rendering);
  View::Bytes negative = render_rejection(
      domain, materializations, context, "\"abc\":[-1]"_view, rendering);
  View::Bytes overflow = render_rejection(
      domain, materializations, context, "\"abc\":[18446744073709551616]"_view,
      rendering);
  View::Bytes extent = render_rejection(
      domain, materializations, context,
      "\"abc\":[0, 18446744073709551615]"_view, rendering);
  View::Bytes index = render_rejection(
      domain, materializations, context, "\"abc\":[3]"_view, rendering);
  View::Bytes start = render_rejection(
      domain, materializations, context, "\"abc\":[4, 0]"_view, rendering);
  View::Bytes size = render_rejection(
      domain, materializations, context, "\"abc\":[2, 2]"_view, rendering);
  View::Bytes syntax = render_rejection(
      domain, materializations, context, "\"abc\":[1,]"_view, rendering);
  View::Bytes real = render_rejection(
      domain, materializations, context, "\"abc\":[1.0]"_view, rendering);

  EXPECT(contains(receiver, "receiver Type `Bool`"_view));
  EXPECT(contains(receiver, "Fixed, View, or Access"_view));
  EXPECT(contains(operand, "index Type `Bool`"_view));
  EXPECT(contains(operand, "signed or unsigned integer Type"_view));
  EXPECT(contains(negative, "index value -1 is negative"_view));
  EXPECT(contains(overflow, "18446744073709551616"_view));
  EXPECT(contains(overflow, "integer or Count range"_view));
  EXPECT(contains(extent, "size value 18446744073709551615"_view));
  EXPECT(contains(extent, "Count or Fixed extent range"_view));
  EXPECT(contains(index, "index 3"_view));
  EXPECT(contains(index, "containing 3 bytes"_view));
  EXPECT(contains(start, "start 4"_view));
  EXPECT(contains(start, "containing 3 bytes"_view));
  EXPECT(contains(size, "size 2"_view));
  EXPECT(contains(size, "remaining 1 bytes after start 2"_view));
  EXPECT(contains(syntax, "malformed index or range operands"_view));
  EXPECT(contains(real, "Slice index Type `Real_64`"_view));
  EXPECT(contains(real, "not a signed or unsigned integer Type"_view));
  EXPECT(rejects(domain, materializations, context, "true:[0]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[true]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[-1]"_view));
  EXPECT(rejects(
      domain, materializations, context,
      "\"abc\":[18446744073709551616]"_view));
  EXPECT(rejects(
      domain, materializations, context,
      "\"abc\":[0, 18446744073709551615]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[3]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[4, 0]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[2, 2]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[1,]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[1.0]"_view));
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, multiply_precedence_and_failure) {
  Allocator::Arena domain;
  Allocator::Arena rendering;
  Library::Language::Materializations materializations(domain);
  ExpressionParserContext context(domain);
  Errors product_errors;
  Errors slice_errors;

  auto product = parse_one(
      domain, materializations, context, "2 * 3"_view, product_errors);
  auto sliced = parse_one(
      domain, materializations, context, "0x[02 03]:[0] * 0x[04]:[0]"_view,
      slice_errors);
  View::Bytes mismatch = render_rejection(
      domain, materializations, context, "2 * true"_view, rendering);
  View::Bytes overflow = render_rejection(
      domain, materializations, context, "18446744073709551615 * 2"_view,
      rendering);
  auto product_value =
      product
          ? select_abstract<Library::Language::Constants::Unsigned>(*product)
          : Option<const Library::Language::Constants::Unsigned&>();
  auto sliced_value =
      sliced ? select_abstract<Library::Language::Constants::Unsigned>(*sliced)
             : Option<const Library::Language::Constants::Unsigned&>();

  ASSERT(product && sliced);
  ASSERT(product_value && sliced_value);
  EXPECT(product->is<Library::Language::Constants::Unsigned>());
  EXPECT(product_value->get_value() == 6);
  EXPECT(sliced_value->get_value() == 8);
  EXPECT(&product->get_type() == &Library::Dialect::get_unsigned_64());
  EXPECT(&sliced->get_type() == &Library::Dialect::get_unsigned_8());
  EXPECT(contains(mismatch, "left Type `Unsigned_64`"_view));
  EXPECT(contains(mismatch, "right Type `Bool`"_view));
  EXPECT(contains(overflow, "overflows selected Type `Unsigned_64`"_view));
  EXPECT(contains(overflow, "18446744073709551615 and 2"_view));
  EXPECT(rejects(domain, materializations, context, "2 *"_view, "*"_view));
  EXPECT(rejects(domain, materializations, context, "2 * true"_view, "*"_view));
  EXPECT(rejects(
      domain, materializations, context, "18446744073709551615 * 2"_view,
      "*"_view));
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, divide_precedence_and_failure) {
  Allocator::Arena domain;
  Allocator::Arena rendering;
  Library::Language::Materializations materializations(domain);
  ExpressionParserContext context(domain);
  Errors association_errors;
  Errors divide_multiply_errors;
  Errors multiply_divide_errors;
  Errors slice_errors;

  auto association = parse_one(
      domain, materializations, context, "24 / 4 / 2"_view, association_errors);
  auto divide_multiply = parse_one(
      domain, materializations, context, "24 / 4 * 2"_view,
      divide_multiply_errors);
  auto multiply_divide = parse_one(
      domain, materializations, context, "24 * 4 / 2"_view,
      multiply_divide_errors);
  auto sliced = parse_one(
      domain, materializations, context, "0x[18]:[0] / 0x[04]:[0]"_view,
      slice_errors);
  View::Bytes mismatch = render_rejection(
      domain, materializations, context, "24 / true"_view, rendering);
  View::Bytes zero = render_rejection(
      domain, materializations, context, "24 / 0"_view, rendering);
  auto association_value =
      association ? select_abstract<Library::Language::Constants::Unsigned>(
                        *association)
                  : Option<const Library::Language::Constants::Unsigned&>();
  auto divide_multiply_value =
      divide_multiply ? select_abstract<Library::Language::Constants::Unsigned>(
                            *divide_multiply)
                      : Option<const Library::Language::Constants::Unsigned&>();
  auto multiply_divide_value =
      multiply_divide ? select_abstract<Library::Language::Constants::Unsigned>(
                            *multiply_divide)
                      : Option<const Library::Language::Constants::Unsigned&>();
  auto sliced_value =
      sliced ? select_abstract<Library::Language::Constants::Unsigned>(*sliced)
             : Option<const Library::Language::Constants::Unsigned&>();

  ASSERT(association && divide_multiply && multiply_divide && sliced);
  ASSERT(
      association_value && divide_multiply_value && multiply_divide_value &&
      sliced_value);
  EXPECT(association_value->get_value() == 3);
  EXPECT(divide_multiply_value->get_value() == 12);
  EXPECT(multiply_divide_value->get_value() == 48);
  EXPECT(sliced_value->get_value() == 6);
  EXPECT(&association->get_type() == &Library::Dialect::get_unsigned_64());
  EXPECT(&sliced->get_type() == &Library::Dialect::get_unsigned_8());
  EXPECT(contains(mismatch, "left Type `Unsigned_64`"_view));
  EXPECT(contains(mismatch, "right Type `Bool`"_view));
  EXPECT(contains(zero, "zero as an integer divisor"_view));
  EXPECT(contains(zero, "selected Type `Unsigned_64`"_view));
  EXPECT(rejects(domain, materializations, context, "24 /"_view, "/"_view));
  EXPECT(
      rejects(domain, materializations, context, "24 / true"_view, "/"_view));
  EXPECT(rejects(domain, materializations, context, "24 / 0"_view, "/"_view));
  EXPECT(association_errors.is_empty());
  EXPECT(divide_multiply_errors.is_empty());
  EXPECT(multiply_divide_errors.is_empty());
  EXPECT(slice_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, modulo_precedence_and_failure) {
  Allocator::Arena domain;
  Allocator::Arena rendering;
  Library::Language::Materializations materializations(domain);
  ExpressionParserContext context(domain);
  Errors association_errors;
  Errors divide_errors;
  Errors multiply_errors;
  Errors sliced_errors;
  Errors signed_errors;

  auto association = parse_one(
      domain, materializations, context, "25 % 6 % 4"_view, association_errors);
  auto divided = parse_one(
      domain, materializations, context, "25 / 6 % 3"_view, divide_errors);
  auto multiplied = parse_one(
      domain, materializations, context, "25 % 6 * 3"_view, multiply_errors);
  auto sliced = parse_one(
      domain, materializations, context, "0x[19]:[0] % 0x[06]:[0]"_view,
      sliced_errors);
  auto signed_value = parse_one(
      domain, materializations, context, "-7 % -3"_view, signed_errors);
  View::Bytes mismatch = render_rejection(
      domain, materializations, context, "25 % true"_view, rendering);
  View::Bytes zero = render_rejection(
      domain, materializations, context, "25 % 0"_view, rendering);
  auto association_value =
      association ? select_abstract<Library::Language::Constants::Unsigned>(
                        *association)
                  : Option<const Library::Language::Constants::Unsigned&>();
  auto divided_value =
      divided
          ? select_abstract<Library::Language::Constants::Unsigned>(*divided)
          : Option<const Library::Language::Constants::Unsigned&>();
  auto multiplied_value =
      multiplied
          ? select_abstract<Library::Language::Constants::Unsigned>(*multiplied)
          : Option<const Library::Language::Constants::Unsigned&>();
  auto sliced_value =
      sliced ? select_abstract<Library::Language::Constants::Unsigned>(*sliced)
             : Option<const Library::Language::Constants::Unsigned&>();
  auto signed_result =
      signed_value
          ? select_abstract<Library::Language::Constants::Signed>(*signed_value)
          : Option<const Library::Language::Constants::Signed&>();

  ASSERT(
      association && divided && multiplied && sliced && signed_value &&
      association_value && divided_value && multiplied_value && sliced_value &&
      signed_result);
  EXPECT(association_value->get_value() == 1);
  EXPECT(divided_value->get_value() == 1);
  EXPECT(multiplied_value->get_value() == 3);
  EXPECT(sliced_value->get_value() == 1);
  EXPECT(signed_result->get_value() == -1);
  EXPECT(&association->get_type() == &Library::Dialect::get_unsigned_64());
  EXPECT(&sliced->get_type() == &Library::Dialect::get_unsigned_8());
  EXPECT(&signed_value->get_type() == &Library::Dialect::get_signed_64());
  EXPECT(contains(mismatch, "left Type `Unsigned_64`"_view));
  EXPECT(contains(mismatch, "right Type `Bool`"_view));
  EXPECT(contains(zero, "zero as a divisor"_view));
  EXPECT(contains(zero, "selected Type `Unsigned_64`"_view));
  EXPECT(rejects(domain, materializations, context, "25 %"_view, "%"_view));
  EXPECT(
      rejects(domain, materializations, context, "25 % true"_view, "%"_view));
  EXPECT(rejects(domain, materializations, context, "25 % 0"_view, "%"_view));
  EXPECT(association_errors.is_empty());
  EXPECT(divide_errors.is_empty());
  EXPECT(multiply_errors.is_empty());
  EXPECT(sliced_errors.is_empty());
  EXPECT(signed_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, subtract_precedence_and_failure) {
  Allocator::Arena domain;
  Allocator::Arena rendering;
  Library::Language::Materializations materializations(domain);
  ExpressionParserContext context(domain);
  Errors precedence_errors;
  Errors association_errors;
  Errors slice_errors;
  Errors negative_errors;

  auto precedence = parse_one(
      domain, materializations, context, "10 - 2 * 3"_view, precedence_errors);
  auto association = parse_one(
      domain, materializations, context, "10 - 2 - 3"_view, association_errors);
  auto sliced = parse_one(
      domain, materializations, context,
      "0x[0A]:[0] - 0x[02]:[0] * 0x[03]:[0]"_view, slice_errors);
  auto negative = parse_one(
      domain, materializations, context, "-10 - -2"_view, negative_errors);
  View::Bytes mismatch = render_rejection(
      domain, materializations, context, "10 - true"_view, rendering);
  View::Bytes underflow = render_rejection(
      domain, materializations, context, "0 - 1"_view, rendering);
  auto precedence_value =
      precedence
          ? select_abstract<Library::Language::Constants::Unsigned>(*precedence)
          : Option<const Library::Language::Constants::Unsigned&>();
  auto association_value =
      association ? select_abstract<Library::Language::Constants::Unsigned>(
                        *association)
                  : Option<const Library::Language::Constants::Unsigned&>();
  auto sliced_value =
      sliced ? select_abstract<Library::Language::Constants::Unsigned>(*sliced)
             : Option<const Library::Language::Constants::Unsigned&>();
  auto negative_value =
      negative
          ? select_abstract<Library::Language::Constants::Signed>(*negative)
          : Option<const Library::Language::Constants::Signed&>();

  ASSERT(precedence && association && sliced && negative);
  ASSERT(
      precedence_value && association_value && sliced_value && negative_value);
  EXPECT(precedence_value->get_value() == 4);
  EXPECT(association_value->get_value() == 5);
  EXPECT(sliced_value->get_value() == 4);
  EXPECT(negative_value->get_value() == -8);
  EXPECT(&precedence->get_type() == &Library::Dialect::get_unsigned_64());
  EXPECT(&association->get_type() == &Library::Dialect::get_unsigned_64());
  EXPECT(&sliced->get_type() == &Library::Dialect::get_unsigned_8());
  EXPECT(contains(mismatch, "left Type `Unsigned_64`"_view));
  EXPECT(contains(mismatch, "right Type `Bool`"_view));
  EXPECT(contains(underflow, "cannot represent 0 minus 1"_view));
  EXPECT(contains(underflow, "selected Type `Unsigned_64`"_view));
  EXPECT(rejects(domain, materializations, context, "10 -"_view, "-"_view));
  EXPECT(
      rejects(domain, materializations, context, "10 - true"_view, "-"_view));
  EXPECT(rejects(domain, materializations, context, "0 - 1"_view, "-"_view));
  EXPECT(precedence_errors.is_empty());
  EXPECT(association_errors.is_empty());
  EXPECT(slice_errors.is_empty());
  EXPECT(negative_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, less_precedence_and_failure) {
  Allocator::Arena domain;
  Allocator::Arena rendering;
  Library::Language::Materializations materializations(domain);
  ExpressionParserContext context(domain);
  Errors precedence_errors;
  Errors false_errors;
  Errors slice_errors;

  auto precedence = parse_one(
      domain, materializations, context, "10 - 2 * 3 < 5"_view,
      precedence_errors);
  auto equal =
      parse_one(domain, materializations, context, "5 < 5"_view, false_errors);
  auto sliced = parse_one(
      domain, materializations, context, "0x[01]:[0] < 0x[02]:[0]"_view,
      slice_errors);
  View::Bytes mismatch = render_rejection(
      domain, materializations, context, "1 < true"_view, rendering);
  View::Bytes chained = render_rejection(
      domain, materializations, context, "1 < 2 < 3"_view, rendering);

  ASSERT(precedence && equal && sliced);
  EXPECT(precedence->is<Library::Language::Constants::True>());
  EXPECT(equal->is<Library::Language::Constants::False>());
  EXPECT(sliced->is<Library::Language::Constants::True>());
  EXPECT(&precedence->get_type() == &Library::Dialect::get_bool());
  EXPECT(&equal->get_type() == &Library::Dialect::get_bool());
  EXPECT(&sliced->get_type() == &Library::Dialect::get_bool());
  EXPECT(contains(mismatch, "left Type `Unsigned_64`"_view));
  EXPECT(contains(mismatch, "right Type `Bool`"_view));
  EXPECT(contains(chained, "left Type `Bool`"_view));
  EXPECT(contains(chained, "right Type `Unsigned_64`"_view));
  EXPECT(rejects(domain, materializations, context, "1 <"_view, "<"_view));
  EXPECT(rejects(domain, materializations, context, "1 < true"_view, "<"_view));
  EXPECT(precedence_errors.is_empty());
  EXPECT(false_errors.is_empty());
  EXPECT(slice_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, greater_precedence_and_failure) {
  Allocator::Arena domain;
  Allocator::Arena rendering;
  Library::Language::Materializations materializations(domain);
  ExpressionParserContext context(domain);
  Errors precedence_errors;
  Errors equal_errors;
  Errors arithmetic_errors;
  Errors slice_errors;

  auto precedence = parse_one(
      domain, materializations, context, "10 - 2 * 3 > 3"_view,
      precedence_errors);
  auto equal =
      parse_one(domain, materializations, context, "5 > 5"_view, equal_errors);
  auto arithmetic = parse_one(
      domain, materializations, context, "5 > 2 * 3"_view, arithmetic_errors);
  auto sliced = parse_one(
      domain, materializations, context, "0x[02]:[0] > 0x[01]:[0]"_view,
      slice_errors);
  View::Bytes mismatch = render_rejection(
      domain, materializations, context, "1 > true"_view, rendering);
  View::Bytes chained = render_rejection(
      domain, materializations, context, "3 > 2 > 1"_view, rendering);
  View::Bytes greater_less = render_rejection(
      domain, materializations, context, "3 > 2 < 4"_view, rendering);
  View::Bytes less_greater = render_rejection(
      domain, materializations, context, "1 < 2 > 0"_view, rendering);

  ASSERT(precedence && equal && arithmetic && sliced);
  EXPECT(precedence->is<Library::Language::Constants::True>());
  EXPECT(equal->is<Library::Language::Constants::False>());
  EXPECT(arithmetic->is<Library::Language::Constants::False>());
  EXPECT(sliced->is<Library::Language::Constants::True>());
  EXPECT(&precedence->get_type() == &Library::Dialect::get_bool());
  EXPECT(&equal->get_type() == &Library::Dialect::get_bool());
  EXPECT(&sliced->get_type() == &Library::Dialect::get_bool());
  EXPECT(contains(mismatch, "left Type `Unsigned_64`"_view));
  EXPECT(contains(mismatch, "right Type `Bool`"_view));
  EXPECT(contains(chained, "left Type `Bool`"_view));
  EXPECT(contains(chained, "right Type `Unsigned_64`"_view));
  EXPECT(contains(greater_less, "left Type `Bool`"_view));
  EXPECT(contains(less_greater, "left Type `Bool`"_view));
  EXPECT(rejects(domain, materializations, context, "1 >"_view, ">"_view));
  EXPECT(rejects(domain, materializations, context, "1 > true"_view, ">"_view));
  EXPECT(precedence_errors.is_empty());
  EXPECT(equal_errors.is_empty());
  EXPECT(arithmetic_errors.is_empty());
  EXPECT(slice_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, equal_domains_and_precedence) {
  Allocator::Arena domain;
  Allocator::Arena rendering;
  Library::Language::Materializations materializations(domain);
  ExpressionParserContext context(domain);
  Errors unsigned_errors;
  Errors signed_errors;
  Errors real_errors;
  Errors flag_errors;
  Errors bytes_errors;
  Errors bytes_difference_errors;
  Errors left_precedence_errors;
  Errors right_precedence_errors;
  Errors association_errors;

  auto unsigned_value = parse_one(
      domain, materializations, context, "8 == 8"_view, unsigned_errors);
  auto signed_value = parse_one(
      domain, materializations, context, "-8 == -8"_view, signed_errors);
  auto real_value = parse_one(
      domain, materializations, context, "0.5 == 0.5"_view, real_errors);
  auto flag_value = parse_one(
      domain, materializations, context, "true == true"_view, flag_errors);
  auto bytes_value = parse_one(
      domain, materializations, context, "\"ab\" == \"ab\""_view, bytes_errors);
  auto bytes_difference = parse_one(
      domain, materializations, context, "\"ab\" == \"ac\""_view,
      bytes_difference_errors);
  auto left_precedence = parse_one(
      domain, materializations, context, "1 < 2 == true"_view,
      left_precedence_errors);
  auto right_precedence = parse_one(
      domain, materializations, context, "true == 1 < 2"_view,
      right_precedence_errors);
  auto association = parse_one(
      domain, materializations, context, "1 == 1 == true"_view,
      association_errors);
  View::Bytes mismatch = render_rejection(
      domain, materializations, context, "1 == true"_view, rendering);
  View::Bytes bytes_mismatch = render_rejection(
      domain, materializations, context, "\"a\" == \"aa\""_view, rendering);
  View::Bytes ordered_result_mismatch = render_rejection(
      domain, materializations, context, "1 == 1 < 2"_view, rendering);

  ASSERT(unsigned_value);
  ASSERT(signed_value);
  ASSERT(real_value);
  ASSERT(flag_value);
  ASSERT(bytes_value);
  ASSERT(bytes_difference);
  ASSERT(left_precedence);
  ASSERT(right_precedence);
  ASSERT(association);
  EXPECT(unsigned_value->is<Library::Language::Constants::True>());
  EXPECT(signed_value->is<Library::Language::Constants::True>());
  EXPECT(real_value->is<Library::Language::Constants::True>());
  EXPECT(flag_value->is<Library::Language::Constants::True>());
  EXPECT(bytes_value->is<Library::Language::Constants::True>());
  EXPECT(bytes_difference->is<Library::Language::Constants::False>());
  EXPECT(left_precedence->is<Library::Language::Constants::True>());
  EXPECT(right_precedence->is<Library::Language::Constants::True>());
  EXPECT(association->is<Library::Language::Constants::True>());
  EXPECT(&unsigned_value->get_type() == &Library::Dialect::get_bool());
  EXPECT(&bytes_value->get_type() == &Library::Dialect::get_bool());
  EXPECT(contains(mismatch, "left Type `Unsigned_64`"_view));
  EXPECT(contains(mismatch, "right Type `Bool`"_view));
  EXPECT(contains(bytes_mismatch, "left Type `Fixed[Unsigned_8,1]`"_view));
  EXPECT(contains(bytes_mismatch, "right Type `Fixed[Unsigned_8,2]`"_view));
  EXPECT(contains(ordered_result_mismatch, "left Type `Unsigned_64`"_view));
  EXPECT(contains(ordered_result_mismatch, "right Type `Bool`"_view));
  EXPECT(rejects(domain, materializations, context, "1 =="_view, "=="_view));
  EXPECT(
      rejects(domain, materializations, context, "1 == true"_view, "=="_view));
  EXPECT(unsigned_errors.is_empty());
  EXPECT(signed_errors.is_empty());
  EXPECT(real_errors.is_empty());
  EXPECT(flag_errors.is_empty());
  EXPECT(bytes_errors.is_empty());
  EXPECT(bytes_difference_errors.is_empty());
  EXPECT(left_precedence_errors.is_empty());
  EXPECT(right_precedence_errors.is_empty());
  EXPECT(association_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, not_equal_domains_and_precedence) {
  Allocator::Arena domain;
  Allocator::Arena rendering;
  Library::Language::Materializations materializations(domain);
  ExpressionParserContext context(domain);
  Errors unsigned_errors;
  Errors signed_errors;
  Errors real_errors;
  Errors flag_errors;
  Errors bytes_errors;
  Errors bytes_difference_errors;
  Errors left_precedence_errors;
  Errors right_precedence_errors;
  Errors association_errors;
  Errors mixed_errors;

  auto unsigned_value = parse_one(
      domain, materializations, context, "8 != 9"_view, unsigned_errors);
  auto signed_value = parse_one(
      domain, materializations, context, "-8 != -8"_view, signed_errors);
  auto real_value = parse_one(
      domain, materializations, context, "0.5 != 1.0"_view, real_errors);
  auto flag_value = parse_one(
      domain, materializations, context, "true != false"_view, flag_errors);
  auto bytes_value = parse_one(
      domain, materializations, context, "\"ab\" != \"ab\""_view, bytes_errors);
  auto bytes_difference = parse_one(
      domain, materializations, context, "\"ab\" != \"ac\""_view,
      bytes_difference_errors);
  auto left_precedence = parse_one(
      domain, materializations, context, "1 < 2 != false"_view,
      left_precedence_errors);
  auto right_precedence = parse_one(
      domain, materializations, context, "false != 1 < 2"_view,
      right_precedence_errors);
  auto association = parse_one(
      domain, materializations, context, "1 != 2 != false"_view,
      association_errors);
  auto mixed = parse_one(
      domain, materializations, context, "1 == 1 != false"_view, mixed_errors);
  View::Bytes mismatch = render_rejection(
      domain, materializations, context, "1 != true"_view, rendering);
  View::Bytes bytes_mismatch = render_rejection(
      domain, materializations, context, "\"a\" != \"aa\""_view, rendering);
  View::Bytes ordered_result_mismatch = render_rejection(
      domain, materializations, context, "1 != 2 < 3"_view, rendering);

  ASSERT(unsigned_value);
  ASSERT(signed_value);
  ASSERT(real_value);
  ASSERT(flag_value);
  ASSERT(bytes_value);
  ASSERT(bytes_difference);
  ASSERT(left_precedence);
  ASSERT(right_precedence);
  ASSERT(association);
  ASSERT(mixed);
  EXPECT(unsigned_value->is<Library::Language::Constants::True>());
  EXPECT(signed_value->is<Library::Language::Constants::False>());
  EXPECT(real_value->is<Library::Language::Constants::True>());
  EXPECT(flag_value->is<Library::Language::Constants::True>());
  EXPECT(bytes_value->is<Library::Language::Constants::False>());
  EXPECT(bytes_difference->is<Library::Language::Constants::True>());
  EXPECT(left_precedence->is<Library::Language::Constants::True>());
  EXPECT(right_precedence->is<Library::Language::Constants::True>());
  EXPECT(association->is<Library::Language::Constants::True>());
  EXPECT(mixed->is<Library::Language::Constants::True>());
  EXPECT(&unsigned_value->get_type() == &Library::Dialect::get_bool());
  EXPECT(&bytes_value->get_type() == &Library::Dialect::get_bool());
  EXPECT(contains(mismatch, "left Type `Unsigned_64`"_view));
  EXPECT(contains(mismatch, "right Type `Bool`"_view));
  EXPECT(contains(bytes_mismatch, "left Type `Fixed[Unsigned_8,1]`"_view));
  EXPECT(contains(bytes_mismatch, "right Type `Fixed[Unsigned_8,2]`"_view));
  EXPECT(contains(ordered_result_mismatch, "left Type `Unsigned_64`"_view));
  EXPECT(contains(ordered_result_mismatch, "right Type `Bool`"_view));
  EXPECT(rejects(domain, materializations, context, "1 !="_view, "!="_view));
  EXPECT(
      rejects(domain, materializations, context, "1 != true"_view, "!="_view));
  EXPECT(unsigned_errors.is_empty());
  EXPECT(signed_errors.is_empty());
  EXPECT(real_errors.is_empty());
  EXPECT(flag_errors.is_empty());
  EXPECT(bytes_errors.is_empty());
  EXPECT(bytes_difference_errors.is_empty());
  EXPECT(left_precedence_errors.is_empty());
  EXPECT(right_precedence_errors.is_empty());
  EXPECT(association_errors.is_empty());
  EXPECT(mixed_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, less_equal_precedence_and_failure) {
  Allocator::Arena domain;
  Allocator::Arena rendering;
  Library::Language::Materializations materializations(domain);
  ExpressionParserContext context(domain);
  Errors precedence_errors;
  Errors false_errors;
  Errors arithmetic_errors;
  Errors slice_errors;

  auto precedence = parse_one(
      domain, materializations, context, "10 - 2 * 3 <= 4"_view,
      precedence_errors);
  auto false_value =
      parse_one(domain, materializations, context, "5 <= 4"_view, false_errors);
  auto arithmetic = parse_one(
      domain, materializations, context, "5 <= 2 * 3"_view, arithmetic_errors);
  auto sliced = parse_one(
      domain, materializations, context, "0x[01]:[0] <= 0x[01]:[0]"_view,
      slice_errors);
  View::Bytes mismatch = render_rejection(
      domain, materializations, context, "1 <= true"_view, rendering);
  View::Bytes chained = render_rejection(
      domain, materializations, context, "1 <= 2 <= 3"_view, rendering);
  View::Bytes less_chain = render_rejection(
      domain, materializations, context, "1 < 2 <= 3"_view, rendering);
  View::Bytes greater_chain = render_rejection(
      domain, materializations, context, "3 > 2 <= 3"_view, rendering);
  View::Bytes then_less = render_rejection(
      domain, materializations, context, "1 <= 2 < 3"_view, rendering);
  View::Bytes then_greater = render_rejection(
      domain, materializations, context, "1 <= 2 > 0"_view, rendering);

  ASSERT(precedence && false_value && arithmetic && sliced);
  EXPECT(precedence->is<Library::Language::Constants::True>());
  EXPECT(false_value->is<Library::Language::Constants::False>());
  EXPECT(arithmetic->is<Library::Language::Constants::True>());
  EXPECT(sliced->is<Library::Language::Constants::True>());
  EXPECT(&precedence->get_type() == &Library::Dialect::get_bool());
  EXPECT(&false_value->get_type() == &Library::Dialect::get_bool());
  EXPECT(&sliced->get_type() == &Library::Dialect::get_bool());
  EXPECT(contains(mismatch, "left Type `Unsigned_64`"_view));
  EXPECT(contains(mismatch, "right Type `Bool`"_view));
  EXPECT(contains(chained, "left Type `Bool`"_view));
  EXPECT(contains(less_chain, "left Type `Bool`"_view));
  EXPECT(contains(greater_chain, "left Type `Bool`"_view));
  EXPECT(contains(then_less, "left Type `Bool`"_view));
  EXPECT(contains(then_greater, "left Type `Bool`"_view));
  EXPECT(rejects(domain, materializations, context, "1 <="_view, "<="_view));
  EXPECT(
      rejects(domain, materializations, context, "1 <= true"_view, "<="_view));
  EXPECT(precedence_errors.is_empty());
  EXPECT(false_errors.is_empty());
  EXPECT(arithmetic_errors.is_empty());
  EXPECT(slice_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(
    ExpressionParserTests,
    greater_equal_precedence_and_failure) {
  Allocator::Arena domain;
  Allocator::Arena rendering;
  Library::Language::Materializations materializations(domain);
  ExpressionParserContext context(domain);
  Errors precedence_errors;
  Errors false_errors;
  Errors arithmetic_errors;
  Errors slice_errors;

  auto precedence = parse_one(
      domain, materializations, context, "10 - 2 * 3 >= 4"_view,
      precedence_errors);
  auto false_value =
      parse_one(domain, materializations, context, "4 >= 5"_view, false_errors);
  auto arithmetic = parse_one(
      domain, materializations, context, "6 >= 2 * 3"_view, arithmetic_errors);
  auto sliced = parse_one(
      domain, materializations, context, "0x[01]:[0] >= 0x[01]:[0]"_view,
      slice_errors);
  View::Bytes mismatch = render_rejection(
      domain, materializations, context, "1 >= true"_view, rendering);
  View::Bytes chained = render_rejection(
      domain, materializations, context, "2 >= 1 >= 0"_view, rendering);
  View::Bytes less_chain = render_rejection(
      domain, materializations, context, "1 < 2 >= 1"_view, rendering);
  View::Bytes greater_chain = render_rejection(
      domain, materializations, context, "3 > 2 >= 1"_view, rendering);
  View::Bytes less_equal_chain = render_rejection(
      domain, materializations, context, "1 <= 2 >= 1"_view, rendering);
  View::Bytes then_less = render_rejection(
      domain, materializations, context, "2 >= 1 < 3"_view, rendering);
  View::Bytes then_greater = render_rejection(
      domain, materializations, context, "2 >= 1 > 0"_view, rendering);
  View::Bytes then_less_equal = render_rejection(
      domain, materializations, context, "2 >= 1 <= 3"_view, rendering);

  ASSERT(precedence && false_value && arithmetic && sliced);
  EXPECT(precedence->is<Library::Language::Constants::True>());
  EXPECT(false_value->is<Library::Language::Constants::False>());
  EXPECT(arithmetic->is<Library::Language::Constants::True>());
  EXPECT(sliced->is<Library::Language::Constants::True>());
  EXPECT(&precedence->get_type() == &Library::Dialect::get_bool());
  EXPECT(&false_value->get_type() == &Library::Dialect::get_bool());
  EXPECT(&sliced->get_type() == &Library::Dialect::get_bool());
  EXPECT(contains(mismatch, "left Type `Unsigned_64`"_view));
  EXPECT(contains(mismatch, "right Type `Bool`"_view));
  EXPECT(contains(chained, "left Type `Bool`"_view));
  EXPECT(contains(less_chain, "left Type `Bool`"_view));
  EXPECT(contains(greater_chain, "left Type `Bool`"_view));
  EXPECT(contains(less_equal_chain, "left Type `Bool`"_view));
  EXPECT(contains(then_less, "left Type `Bool`"_view));
  EXPECT(contains(then_greater, "left Type `Bool`"_view));
  EXPECT(contains(then_less_equal, "left Type `Bool`"_view));
  EXPECT(rejects(domain, materializations, context, "1 >="_view, ">="_view));
  EXPECT(
      rejects(domain, materializations, context, "1 >= true"_view, ">="_view));
  EXPECT(precedence_errors.is_empty());
  EXPECT(false_errors.is_empty());
  EXPECT(arithmetic_errors.is_empty());
  EXPECT(slice_errors.is_empty());
}
