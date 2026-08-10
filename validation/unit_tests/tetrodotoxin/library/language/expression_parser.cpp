// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/access/value.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/identifier.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "tetrodotoxin/library/language/operations/add.hpp"
#include "tetrodotoxin/library/language/operations/and.hpp"
#include "tetrodotoxin/library/language/operations/divide.hpp"
#include "tetrodotoxin/library/language/operations/equal.hpp"
#include "tetrodotoxin/library/language/operations/greater.hpp"
#include "tetrodotoxin/library/language/operations/greater_equal.hpp"
#include "tetrodotoxin/library/language/operations/less.hpp"
#include "tetrodotoxin/library/language/operations/less_equal.hpp"
#include "tetrodotoxin/library/language/operations/modulo.hpp"
#include "tetrodotoxin/library/language/operations/multiply.hpp"
#include "tetrodotoxin/library/language/operations/negate.hpp"
#include "tetrodotoxin/library/language/operations/not.hpp"
#include "tetrodotoxin/library/language/operations/not_equal.hpp"
#include "tetrodotoxin/library/language/operations/or.hpp"
#include "tetrodotoxin/library/language/operations/range.hpp"
#include "tetrodotoxin/library/language/operations/subtract.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
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

struct ExpressionParserObservations {
  Bool table_seen = False;
};

class ExpressionParserContext : public Abstract {
 public:
  ExpressionParserContext(
      Allocator::Arena& domain,
      ExpressionParserObservations& observations)
      : observations(observations), table(domain, "0123456789"_view) {}

  auto get_name() const -> View::Bytes override { return "Context"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    if (route == "$[table]"_view) {
      observations.table_seen = True;
      return table;
    }

    return Invalid::get_invalid();
  }

  ExpressionParserObservations& observations;
  ExpressionParserResource table;
};

class ExpressionParserMonograph : public Tetrodotoxin::Language::Monograph {
 public:
  ExpressionParserMonograph(Allocator::Arena& domain)
      : Tetrodotoxin::Language::Monograph(domain, Documentation::get_empty()) {}

  auto get_name() const -> View::Bytes override { return "Expressions"_view; }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

static auto parse_one(
    Allocator::Arena& domain,
    Library::Language::Materializations& materializations,
    const Abstract& context,
    View::Bytes source,
    Errors& errors) -> Option<Library::Language::Expression&> {
  Tokenizer tokenizer(domain, source, "expression-parser.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto parsed = Library::Language::Parser::Expression::parse(
      domain, materializations, cursor, context);
  if (parsed && !cursor.matches(Code::Type::Terminal)) {
    return {};
  }

  return parsed;
}

static auto rejects_grammar(
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
  Token current = cursor.current();
  return !parsed && current.get_offset() == start.get_offset() &&
         current.get_code() == start.get_code() && !errors.is_empty();
}

template <typename selected_type>
static auto select(const Library::Language::Expression& expression)
    -> Option<const selected_type&> {
  return expression.visit<selected_type>(
      [](const selected_type& selected) -> Option<const selected_type&> {
        return selected;
      },
      [](const Abstract&) -> Option<const selected_type&> { return {}; });
}

static auto get_input(
    const Library::Language::Expression& expression,
    Count index) -> Option<const Library::Language::Expression&> {
  return expression.get_inputs().get_abstract(index).visit(
      []() -> Option<const Library::Language::Expression&> { return {}; },
      [](const Abstract& selected) {
        return selected.visit<Library::Language::Expression>(
            [](const Library::Language::Expression& input)
                -> Option<const Library::Language::Expression&> {
              return input;
            },
            [](const Abstract&)
                -> Option<const Library::Language::Expression&> { return {}; });
      });
}

static auto matches_anchor(
    const Library::Language::Expression& expression,
    View::Bytes source,
    View::Bytes focus,
    View::Bytes span) -> Bool {
  return expression.get_anchor().visit(
      []() { return False; },
      [&](const Anchor& anchor) -> Bool {
        return anchor.get_token().caculate_text(source) == focus &&
                       anchor.get_span().caculate_text(source) == span
                   ? True
                   : False;
      });
}

template <typename root_type, typename left_type>
static auto has_left_shape(const Library::Language::Expression& expression)
    -> Bool {
  auto root = select<root_type>(expression);
  if (!root) {
    return False;
  }

  auto left = get_input(*root, 0);
  return left && left->template is<left_type>();
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, original_operation_and_link) {
  static constexpr View::Bytes source = "2 * 3"_view;
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  ExpressionParserMonograph monograph(domain);
  Errors errors;
  auto parsed = parse_one(domain, materializations, context, source, errors);
  ASSERT(parsed && parsed->is<Library::Language::Operations::Multiply>());
  const Library::Language::Expression* identity = &*parsed;

  EXPECT(&parsed->get_type() == &Invalid::get_invalid());
  EXPECT(matches_anchor(*parsed, source, "*"_view, source));
  ASSERT_EQ(parsed->get_inputs().get_size(), Count(2));
  ASSERT(parsed->link(monograph, context, materializations));
  ASSERT(parsed->link(monograph, context, materializations));
  EXPECT(&*parsed == identity);
  EXPECT(&parsed->get_type() == &Library::Dialect::get_unsigned_64());
  EXPECT(monograph.get_diagnostics().is_empty());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, type_mismatch_waits_for_link) {
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  ExpressionParserMonograph mismatch_graph(domain);
  Errors mismatch_errors;
  auto mismatch = parse_one(
      domain, materializations, context, "2 * true"_view, mismatch_errors);
  ASSERT(mismatch);
  EXPECT(mismatch->is<Library::Language::Operations::Multiply>());
  EXPECT_NOT(mismatch->link(mismatch_graph, context, materializations));
  ASSERT_EQ(mismatch_graph.get_diagnostics().get_size(), Count(1));
  ASSERT(mismatch_graph.get_diagnostics().get_data()[0].get_anchor());
  EXPECT_TEXT(
      mismatch_graph.get_diagnostics()
          .get_data()[0]
          .get_anchor()
          ->get_span()
          .caculate_text("2 * true"_view),
      "2 * true"_view);
  EXPECT(mismatch_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(
    ExpressionParserTests,
    value_access_legality_waits_for_link) {
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  ExpressionParserMonograph bounds_graph(domain);
  ExpressionParserMonograph real_graph(domain);
  Errors bounds_errors;
  Errors real_errors;
  auto bounds = parse_one(
      domain, materializations, context, "\"abc\":[9]"_view, bounds_errors);
  auto real = parse_one(
      domain, materializations, context, "\"abc\":[1.0]"_view, real_errors);
  ASSERT(bounds && real);
  EXPECT(bounds->is<Library::Language::Access::Value>());
  EXPECT(real->is<Library::Language::Access::Value>());
  EXPECT(bounds->link(bounds_graph, context, materializations));
  EXPECT_NOT(real->link(real_graph, context, materializations));
  ASSERT_EQ(real_graph.get_diagnostics().get_size(), Count(1));
  EXPECT(bounds_errors.is_empty());
  EXPECT(real_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, postfix_span) {
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Errors slice_errors;
  auto value = parse_one(
      domain, materializations, context, "\"abcd\":[1, 2]"_view, slice_errors);
  ASSERT(value);
  EXPECT(value->is<Library::Language::Access::Value>());
  EXPECT(matches_anchor(
      *value, "\"abcd\":[1, 2]"_view, ":["_view, "\"abcd\":[1, 2]"_view));
  EXPECT(slice_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, prefix_spans) {
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Errors negate_errors;
  Errors not_errors;
  auto negate =
      parse_one(domain, materializations, context, "--8"_view, negate_errors);
  auto logical =
      parse_one(domain, materializations, context, "!true"_view, not_errors);
  ASSERT(negate && logical);
  EXPECT(negate->is<Library::Language::Operations::Negate>());
  EXPECT(matches_anchor(*negate, "--8"_view, "-"_view, "--8"_view));
  EXPECT(logical->is<Library::Language::Operations::Not>());
  EXPECT(matches_anchor(*logical, "!true"_view, "!"_view, "!true"_view));
  EXPECT(negate_errors.is_empty());
  EXPECT(not_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, authored_operation_anchors) {
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Errors errors;

  auto divide =
      parse_one(domain, materializations, context, "8 / 2"_view, errors);
  auto add = parse_one(domain, materializations, context, "1 + 2"_view, errors);
  auto logical_and =
      parse_one(domain, materializations, context, "true & false"_view, errors);
  auto logical_or =
      parse_one(domain, materializations, context, "false | true"_view, errors);
  auto equal =
      parse_one(domain, materializations, context, "1 == 1"_view, errors);
  auto greater =
      parse_one(domain, materializations, context, "2 > 1"_view, errors);
  auto greater_equal =
      parse_one(domain, materializations, context, "2 >= 1"_view, errors);
  auto less =
      parse_one(domain, materializations, context, "1 < 2"_view, errors);
  auto less_equal =
      parse_one(domain, materializations, context, "1 <= 2"_view, errors);
  auto modulo =
      parse_one(domain, materializations, context, "8 % 3"_view, errors);
  auto multiply =
      parse_one(domain, materializations, context, "2 * 3"_view, errors);
  auto negate = parse_one(domain, materializations, context, "-8"_view, errors);
  auto logical =
      parse_one(domain, materializations, context, "!true"_view, errors);
  auto not_equal =
      parse_one(domain, materializations, context, "1 != 2"_view, errors);
  auto value =
      parse_one(domain, materializations, context, "\"a\":[0]"_view, errors);
  auto subtract =
      parse_one(domain, materializations, context, "2 - 1"_view, errors);
  auto range =
      parse_one(domain, materializations, context, "1...4"_view, errors);

  ASSERT(
      divide && add && logical_and && logical_or && equal && greater &&
      greater_equal && less && less_equal);
  ASSERT(modulo && multiply && negate && logical && not_equal && value);
  ASSERT(subtract && range);
  EXPECT(matches_anchor(*divide, "8 / 2"_view, "/"_view, "8 / 2"_view));
  EXPECT(matches_anchor(*add, "1 + 2"_view, "+"_view, "1 + 2"_view));
  EXPECT(matches_anchor(
      *logical_and, "true & false"_view, "&"_view, "true & false"_view));
  EXPECT(matches_anchor(
      *logical_or, "false | true"_view, "|"_view, "false | true"_view));
  EXPECT(matches_anchor(*equal, "1 == 1"_view, "=="_view, "1 == 1"_view));
  EXPECT(matches_anchor(*greater, "2 > 1"_view, ">"_view, "2 > 1"_view));
  EXPECT(
      matches_anchor(*greater_equal, "2 >= 1"_view, ">="_view, "2 >= 1"_view));
  EXPECT(matches_anchor(*less, "1 < 2"_view, "<"_view, "1 < 2"_view));
  EXPECT(matches_anchor(*less_equal, "1 <= 2"_view, "<="_view, "1 <= 2"_view));
  EXPECT(matches_anchor(*modulo, "8 % 3"_view, "%"_view, "8 % 3"_view));
  EXPECT(matches_anchor(*multiply, "2 * 3"_view, "*"_view, "2 * 3"_view));
  EXPECT(matches_anchor(*negate, "-8"_view, "-"_view, "-8"_view));
  EXPECT(matches_anchor(*logical, "!true"_view, "!"_view, "!true"_view));
  EXPECT(matches_anchor(*not_equal, "1 != 2"_view, "!="_view, "1 != 2"_view));
  EXPECT(matches_anchor(*value, "\"a\":[0]"_view, ":["_view, "\"a\":[0]"_view));
  EXPECT(matches_anchor(*subtract, "2 - 1"_view, "-"_view, "2 - 1"_view));
  EXPECT(matches_anchor(*range, "1...4"_view, "..."_view, "1...4"_view));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, range_rhs_rollback) {
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);

  EXPECT(rejects_grammar(domain, materializations, context, "1..."_view));
  EXPECT(rejects_grammar(domain, materializations, context, "1...2...3"_view));
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, nested_precedence_spans) {
  static constexpr View::Bytes source = "2 * 3 - 4 == 2"_view;
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Errors errors;
  auto parsed = parse_one(domain, materializations, context, source, errors);
  ASSERT(parsed);
  auto equal = select<Library::Language::Operations::Equal>(*parsed);
  ASSERT(equal);
  auto subtract_expression = get_input(*equal, 0);
  ASSERT(subtract_expression);
  auto subtract =
      select<Library::Language::Operations::Subtract>(*subtract_expression);
  ASSERT(subtract);
  auto multiply_expression = get_input(*subtract, 0);
  ASSERT(multiply_expression);
  auto multiply =
      select<Library::Language::Operations::Multiply>(*multiply_expression);
  ASSERT(multiply);

  auto multiply_left = get_input(*multiply, 0);
  auto multiply_right = get_input(*multiply, 1);
  auto subtract_right = get_input(*subtract, 1);
  auto equal_right = get_input(*equal, 1);
  ASSERT(multiply_left && multiply_right && subtract_right && equal_right);
  EXPECT(matches_anchor(*equal, source, "=="_view, source));
  EXPECT(matches_anchor(*subtract, source, "-"_view, "2 * 3 - 4"_view));
  EXPECT(matches_anchor(*multiply, source, "*"_view, "2 * 3"_view));
  EXPECT(matches_anchor(*multiply_left, source, "2"_view, "2"_view));
  EXPECT(matches_anchor(*multiply_right, source, "3"_view, "3"_view));
  EXPECT(matches_anchor(*subtract_right, source, "4"_view, "4"_view));
  EXPECT(matches_anchor(*equal_right, source, "2"_view, "2"_view));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, ordered_chain_links_after_grammar) {
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  ExpressionParserMonograph invalid_chain_graph(domain);
  Errors errors;
  auto comparison_chain =
      parse_one(domain, materializations, context, "1 < 2 <= 3"_view, errors);
  ASSERT(comparison_chain);
  EXPECT((has_left_shape<
          Library::Language::Operations::LessEqual,
          Library::Language::Operations::Less>(*comparison_chain)));
  EXPECT_NOT(
      comparison_chain->link(invalid_chain_graph, context, materializations));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, embedded_source_stays_operation) {
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  ExpressionParserMonograph monograph(domain);
  Errors errors;
  auto parsed = parse_one(
      domain, materializations, context, "$[table]:[2, 4]"_view, errors);
  ASSERT(parsed && parsed->is<Library::Language::Access::Value>());
  EXPECT(observations.table_seen);
  EXPECT(matches_anchor(
      *parsed, "$[table]:[2, 4]"_view, ":["_view, "$[table]:[2, 4]"_view));
  EXPECT(parsed->link(monograph, context, materializations));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, address_chain_and_anchor) {
  static constexpr View::Bytes source = "receiver.member.tail"_view;
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Errors errors;
  auto parsed = parse_one(domain, materializations, context, source, errors);
  ASSERT(parsed);
  auto outer = select<Library::Language::Access::Address>(*parsed);
  ASSERT(outer);
  auto inner =
      select<Library::Language::Access::Address>(outer->get_receiver());
  ASSERT(inner);

  EXPECT_TEXT(outer->get_name(), "tail"_view);
  EXPECT_TEXT(inner->get_name(), "member"_view);
  EXPECT(matches_anchor(*outer, source, "tail"_view, source));
  EXPECT(matches_anchor(*inner, source, "member"_view, "receiver.member"_view));
  EXPECT(inner->get_receiver().is<Library::Language::Identifier>());
  EXPECT_NOT(outer->get_addressable());
  EXPECT_NOT(inner->get_addressable());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, malformed_grammar_is_atomic) {
  Allocator::Arena domain;
  Library::Language::Materializations materializations(domain);
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);

  EXPECT(rejects_grammar(domain, materializations, context, "2 *"_view));
  EXPECT(rejects_grammar(domain, materializations, context, "2 +"_view));
  EXPECT(rejects_grammar(domain, materializations, context, "true &"_view));
  EXPECT(rejects_grammar(domain, materializations, context, "false |"_view));
  EXPECT(rejects_grammar(domain, materializations, context, "!"_view));
  EXPECT(rejects_grammar(domain, materializations, context, "receiver."_view));
  EXPECT(rejects_grammar(domain, materializations, context, "\"abc\":[]"_view));
  EXPECT(
      rejects_grammar(domain, materializations, context, "\"abc\":[1,]"_view));
  EXPECT(
      rejects_grammar(domain, materializations, context, "\"abc\":[1, 1"_view));
  EXPECT(
      rejects_grammar(domain, materializations, context, "\"abc\":[1 1]"_view));
}
