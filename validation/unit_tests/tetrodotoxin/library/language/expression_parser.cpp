// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
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
    View::Bytes source) -> Bool {
  Errors errors;
  Tokenizer tokenizer(domain, source, "rejected-expression.ttx"_view);
  Cursor cursor(tokenizer, errors);
  Token start = cursor.current();
  auto parsed = Library::Language::Parser::Expression::parse(
      domain, materializations, cursor, context);
  if (parsed || !matches_token(cursor, start) || errors.get_size() != 1) {
    return False;
  }

  Count postfix = Algorithm::search(source, ":["_view);
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

  ASSERT(unsliced && same_type && first && last && signed_zero);
  EXPECT(unsliced->is<Library::Language::Constant>());
  EXPECT(unsliced->is<Library::Language::Constants::Bytes>());
  EXPECT(unsliced->get_inputs().is_empty());
  EXPECT(&unsliced->get_type() == &same_type->get_type());
  EXPECT_TEXT(
      static_cast<const Library::Language::Constants::Bytes&>(*unsliced)
          .get_value(),
      "abcd"_view);
  EXPECT(
      static_cast<const Library::Language::Constants::Unsigned&>(*first)
          .get_value() == Unsigned_64('A'));
  EXPECT(
      static_cast<const Library::Language::Constants::Unsigned&>(*last)
          .get_value() == Unsigned_64(255));
  EXPECT(
      static_cast<const Library::Language::Constants::Unsigned&>(*signed_zero)
          .get_value() == Unsigned_64(0x7F));
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

  ASSERT(full && interior && empty && chained && embedded && recursive);
  EXPECT_TEXT(
      static_cast<const Library::Language::Constants::Bytes&>(*full)
          .get_value(),
      "abcd"_view);
  View::Bytes interior_value =
      static_cast<const Library::Language::Constants::Bytes&>(*interior)
          .get_value();
  ASSERT_EQ(interior_value.get_size(), Count(2));
  EXPECT(interior_value[0] == 1);
  EXPECT(interior_value[1] == 2);
  EXPECT(
      static_cast<const Library::Language::Constants::Bytes&>(*empty)
          .get_value()
          .is_empty());
  EXPECT_TEXT(
      static_cast<const Library::Language::Constants::Bytes&>(*chained)
          .get_value(),
      "cd"_view);
  EXPECT_TEXT(
      static_cast<const Library::Language::Constants::Bytes&>(*embedded)
          .get_value(),
      "2345"_view);
  EXPECT_TEXT(
      static_cast<const Library::Language::Constants::Bytes&>(*recursive)
          .get_value(),
      "bc"_view);
  EXPECT(context.table_seen);
  View::Bytes embedded_value =
      static_cast<const Library::Language::Constants::Bytes&>(*embedded)
          .get_value();
  EXPECT(embedded_value.get_data() == context.get_table_value().get_data() + 2);
  EXPECT(embedded->get_inputs().is_empty());

  const auto& full_type =
      static_cast<const Library::Language::Types::Fixed&>(full->get_type());
  const auto& empty_type =
      static_cast<const Library::Language::Types::Fixed&>(empty->get_type());
  const auto& chained_type =
      static_cast<const Library::Language::Types::Fixed&>(chained->get_type());
  EXPECT(full_type.get_extent() == 4);
  EXPECT(empty_type.get_extent() == 0);
  EXPECT(chained_type.get_extent() == 2);
  EXPECT(&full_type.get_element_type() == &Library::Dialect::get_unsigned_8());
  EXPECT(&empty_type.get_element_type() == &Library::Dialect::get_unsigned_8());
  EXPECT(
      &chained_type.get_element_type() == &Library::Dialect::get_unsigned_8());
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
  EXPECT(contains(index, "index 3"_view));
  EXPECT(contains(index, "containing 3 bytes"_view));
  EXPECT(contains(start, "start 4"_view));
  EXPECT(contains(start, "containing 3 bytes"_view));
  EXPECT(contains(size, "size 2"_view));
  EXPECT(contains(size, "remaining 1 bytes after start 2"_view));
  EXPECT(contains(syntax, "malformed index or range operands"_view));
  EXPECT(contains(real, "operand `1.0` has Real_64 Type"_view));
  EXPECT(rejects(domain, materializations, context, "true:[0]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[true]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[-1]"_view));
  EXPECT(rejects(
      domain, materializations, context,
      "\"abc\":[18446744073709551616]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[3]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[4, 0]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[2, 2]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[1,]"_view));
  EXPECT(rejects(domain, materializations, context, "\"abc\":[1.0]"_view));
}
