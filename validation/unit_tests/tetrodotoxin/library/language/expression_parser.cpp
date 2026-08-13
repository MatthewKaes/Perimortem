// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/access/slice.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
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

static auto create_monograph(
    Allocator::Arena& domain,
    Library::Dialect& dialect,
    Abstract& context) -> Option<Library::Language::Monograph&> {
  Errors errors;
  Tokenizer tokenizer(domain, ""_view, "expression-source.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto interpreted = dialect.interpret(
      domain, cursor, Documentation::get_empty(), Anchor::create(Span()),
      context);
  if (!interpreted || !errors.is_empty() ||
      !interpreted->is<Library::Language::Monograph>()) {
    return {};
  }

  return static_cast<Library::Language::Monograph&>(*interpreted);
}

static auto parse_one(
    Allocator::Arena& domain,
    Library::Language::Monograph& monograph,
    View::Bytes source,
    Errors& errors) -> Option<Library::Language::Model::Pack&> {
  Tokenizer tokenizer(domain, source, "expression-parser.ttx"_view);
  Cursor cursor(tokenizer, errors);
  auto parsed =
      Library::Language::Parser::Expression::parse(domain, monograph, cursor);
  if (parsed && !cursor.matches(Code::Type::Terminal)) {
    return {};
  }

  return parsed;
}

static auto rejects_grammar(
    Allocator::Arena& domain,
    Library::Language::Monograph& monograph,
    View::Bytes source) -> Bool {
  Errors errors;
  Tokenizer tokenizer(domain, source, "rejected-expression.ttx"_view);
  Cursor cursor(tokenizer, errors);
  Token start = cursor.current();
  auto parsed =
      Library::Language::Parser::Expression::parse(domain, monograph, cursor);
  Token current = cursor.current();
  return !parsed && current.get_offset() == start.get_offset() &&
         current.get_code() == start.get_code() && !errors.is_empty();
}

template <typename selected_type>
static auto select(const Abstract& abstract) -> Option<const selected_type&> {
  return abstract.visit<selected_type>(
      [](const selected_type& selected) -> Option<const selected_type&> {
        return selected;
      },
      [](const Abstract&) -> Option<const selected_type&> { return {}; });
}

static auto matches_anchor(
    const Abstract& abstract,
    View::Bytes source,
    View::Bytes focus,
    View::Bytes span) -> Bool {
  auto expression = select<Library::Language::Expression>(abstract);
  BAIL_IF(!expression);
  return expression->get_anchor().visit(
      []() { return False; },
      [&](const Anchor& anchor) -> Bool {
        return anchor.get_token().caculate_text(source) == focus &&
                       anchor.get_span().caculate_text(source) == span
                   ? True
                   : False;
      });
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, original_operation_and_link) {
  static constexpr View::Bytes source = "2 * 3"_view;
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  Errors errors;
  auto parsed = parse_one(domain, *monograph, source, errors);
  ASSERT(parsed && parsed->is<Library::Language::Operations::Multiply>());
  auto expression = select<Library::Language::Expression>(*parsed);
  ASSERT(expression);
  const Library::Language::Expression* identity = &*expression;

  EXPECT(&parsed->get_type() == &Invalid::get_invalid());
  EXPECT(matches_anchor(*parsed, source, "*"_view, source));
  ASSERT(parsed->link(*monograph, context));
  ASSERT(parsed->link(*monograph, context));
  EXPECT(&*expression == identity);
  EXPECT(&parsed->get_type() == &Library::Dialect::get_unsigned_64());
  EXPECT(monograph->get_diagnostics().is_empty());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, type_mismatch_waits_for_link) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto mismatch_graph = create_monograph(domain, dialect, context);
  ASSERT(mismatch_graph);
  Errors mismatch_errors;
  auto mismatch =
      parse_one(domain, *mismatch_graph, "2 * true"_view, mismatch_errors);
  ASSERT(mismatch);
  EXPECT(mismatch->is<Library::Language::Operations::Multiply>());
  EXPECT_NOT(mismatch->link(*mismatch_graph, context));
  ASSERT_EQ(mismatch_graph->get_diagnostics().get_size(), Count(1));
  ASSERT(mismatch_graph->get_diagnostics().get_data()[0].get_anchor());
  EXPECT_TEXT(
      mismatch_graph->get_diagnostics()
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
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto bounds_graph = create_monograph(domain, dialect, context);
  auto real_graph = create_monograph(domain, dialect, context);
  ASSERT(bounds_graph && real_graph);
  Errors bounds_errors;
  Errors real_errors;
  auto bounds =
      parse_one(domain, *bounds_graph, "\"abc\":[9]"_view, bounds_errors);
  auto real = parse_one(domain, *real_graph, "\"abc\":[1.0]"_view, real_errors);
  ASSERT(bounds && real);
  EXPECT(bounds->is<Library::Language::Access::Slice>());
  EXPECT(real->is<Library::Language::Access::Slice>());
  EXPECT(bounds->link(*bounds_graph, context));
  EXPECT_NOT(real->link(*real_graph, context));
  ASSERT_EQ(real_graph->get_diagnostics().get_size(), Count(1));
  EXPECT(bounds_errors.is_empty());
  EXPECT(real_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, postfix_span) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  Errors slice_errors;
  auto value =
      parse_one(domain, *monograph, "\"abcd\":[1, 2]"_view, slice_errors);
  ASSERT(value);
  EXPECT(value->is<Library::Language::Access::Slice>());
  EXPECT(matches_anchor(
      *value, "\"abcd\":[1, 2]"_view, ":["_view, "\"abcd\":[1, 2]"_view));
  EXPECT(slice_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, prefix_spans) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  Errors negate_errors;
  Errors not_errors;
  auto negate = parse_one(domain, *monograph, "--8"_view, negate_errors);
  auto logical = parse_one(domain, *monograph, "!true"_view, not_errors);
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
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  Errors errors;

  auto divide = parse_one(domain, *monograph, "8 / 2"_view, errors);
  auto add = parse_one(domain, *monograph, "1 + 2"_view, errors);
  auto logical_and =
      parse_one(domain, *monograph, "true and false"_view, errors);
  auto logical_or = parse_one(domain, *monograph, "false or true"_view, errors);
  auto equal = parse_one(domain, *monograph, "1 == 1"_view, errors);
  auto greater = parse_one(domain, *monograph, "2 > 1"_view, errors);
  auto greater_equal = parse_one(domain, *monograph, "2 >= 1"_view, errors);
  auto less = parse_one(domain, *monograph, "1 < 2"_view, errors);
  auto less_equal = parse_one(domain, *monograph, "1 <= 2"_view, errors);
  auto modulo = parse_one(domain, *monograph, "8 % 3"_view, errors);
  auto multiply = parse_one(domain, *monograph, "2 * 3"_view, errors);
  auto negate = parse_one(domain, *monograph, "-8"_view, errors);
  auto logical = parse_one(domain, *monograph, "!true"_view, errors);
  auto not_equal = parse_one(domain, *monograph, "1 != 2"_view, errors);
  auto value = parse_one(domain, *monograph, "\"a\":[0]"_view, errors);
  auto subtract = parse_one(domain, *monograph, "2 - 1"_view, errors);
  auto range = parse_one(domain, *monograph, "1...4"_view, errors);

  ASSERT(
      divide && add && logical_and && logical_or && equal && greater &&
      greater_equal && less && less_equal);
  ASSERT(modulo && multiply && negate && logical && not_equal && value);
  ASSERT(subtract && range);
  EXPECT(matches_anchor(*divide, "8 / 2"_view, "/"_view, "8 / 2"_view));
  EXPECT(matches_anchor(*add, "1 + 2"_view, "+"_view, "1 + 2"_view));
  EXPECT(matches_anchor(
      *logical_and, "true and false"_view, "and"_view, "true and false"_view));
  EXPECT(matches_anchor(
      *logical_or, "false or true"_view, "or"_view, "false or true"_view));
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
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);

  EXPECT(rejects_grammar(domain, *monograph, "1..."_view));
  EXPECT(rejects_grammar(domain, *monograph, "1...2...3"_view));
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, nested_precedence_evaluates) {
  static constexpr View::Bytes source = "2 * 3 - 4 == 2"_view;
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  Errors errors;
  auto parsed = parse_one(domain, *monograph, source, errors);
  ASSERT(parsed);
  auto equal = select<Library::Language::Operations::Equal>(*parsed);
  ASSERT(equal);
  EXPECT(matches_anchor(*equal, source, "=="_view, source));
  ASSERT(parsed->link(*monograph, context));
  parsed->finalize();
  auto folded = equal->get_folded();
  ASSERT(folded);
  EXPECT(folded->is<Library::Language::Constants::True>());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, ordered_chain_links_after_grammar) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto invalid_chain_graph = create_monograph(domain, dialect, context);
  ASSERT(invalid_chain_graph);
  Errors errors;
  auto comparison_chain =
      parse_one(domain, *invalid_chain_graph, "1 < 2 <= 3"_view, errors);
  ASSERT(comparison_chain);
  EXPECT(comparison_chain->is<Library::Language::Operations::LessEqual>());
  EXPECT_NOT(comparison_chain->link(*invalid_chain_graph, context));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, embedded_source_stays_slice) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  Errors errors;
  auto parsed = parse_one(domain, *monograph, "$[table]:[2, 4]"_view, errors);
  ASSERT(parsed && parsed->is<Library::Language::Access::Slice>());
  EXPECT(observations.table_seen);
  EXPECT(matches_anchor(
      *parsed, "$[table]:[2, 4]"_view, ":["_view, "$[table]:[2, 4]"_view));
  EXPECT(parsed->link(*monograph, context));
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, address_chain_and_anchor) {
  static constexpr View::Bytes source = "receiver.member.tail"_view;
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  Errors errors;
  auto parsed = parse_one(domain, *monograph, source, errors);
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
  EXPECT(
      inner->get_receiver().is<Library::Language::Expressions::Identifier>());
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(
    ExpressionParserTests,
    parenthesized_pack_preserves_real_value_flow) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  Errors errors;

  auto scalar = parse_one(domain, *monograph, "(2)"_view, errors);
  auto empty = parse_one(domain, *monograph, "()"_view, errors);
  auto positional = parse_one(domain, *monograph, "(1, true)"_view, errors);
  auto named =
      parse_one(domain, *monograph, "(.right = true, .left = 1)"_view, errors);
  ASSERT(scalar && empty && positional && named);

  // Parentheses do not manufacture a carrier around one positional value.
  // Empty, multiple, and named values remain complete Pack owners instead of
  // being laundered through an eager aggregate Type.
  EXPECT(scalar->is<Library::Language::Expression>());
  EXPECT_NOT(empty->is<Library::Language::Expression>());
  EXPECT_NOT(positional->is<Library::Language::Expression>());
  EXPECT_NOT(named->is<Library::Language::Expression>());
  EXPECT(empty->resolve().is<Invalid>());
  EXPECT(positional->resolve().is<Invalid>());
  EXPECT(named->resolve().is<Invalid>());

  ASSERT(scalar->link(*monograph, context));
  ASSERT(empty->link(*monograph, context));
  ASSERT(positional->link(*monograph, context));
  ASSERT(named->link(*monograph, context));
  EXPECT(&empty->resolve() == &*empty);
  EXPECT(&positional->resolve() == &*positional);
  EXPECT(&named->resolve() == &*named);
  EXPECT_EQ(empty->get_layout().get_size(), Count(0));
  EXPECT_EQ(positional->get_layout().get_size(), Count(2));
  EXPECT_EQ(named->get_layout().get_size(), Count(2));
  EXPECT(named->get_type().is<Invalid>());

  auto positional_first = positional->get_layout().get_abstract(0);
  auto positional_second = positional->get_layout().get_abstract(1);
  auto named_first = named->get_layout().get_abstract(0);
  auto named_second = named->get_layout().get_abstract(1);
  ASSERT(positional_first && positional_second && named_first && named_second);
  EXPECT(positional_first->is<Library::Language::Expression>());
  EXPECT(positional_second->is<Library::Language::Expression>());
  EXPECT(named_first->is<Library::Language::Expression>());
  EXPECT(named_second->is<Library::Language::Expression>());
  EXPECT_NOT(positional->get_layout().get_name(0));
  ASSERT(named->get_layout().get_name(0));
  ASSERT(named->get_layout().get_name(1));
  EXPECT_TEXT(*named->get_layout().get_name(0), "right"_view);
  EXPECT_TEXT(*named->get_layout().get_name(1), "left"_view);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(
    ExpressionParserTests,
    scalar_operations_accept_only_expression_packs) {
  static constexpr View::Bytes source = "(2) * (3)"_view;
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  Errors errors;

  auto parsed = parse_one(domain, *monograph, source, errors);
  ASSERT(parsed && parsed->is<Library::Language::Operations::Multiply>());
  EXPECT(matches_anchor(*parsed, source, "*"_view, source));
  EXPECT(parsed->link(*monograph, context));
  EXPECT(errors.is_empty());

  EXPECT(rejects_grammar(domain, *monograph, "(1, 2) + 3"_view));
  EXPECT(rejects_grammar(domain, *monograph, "1 + (2, 3)"_view));
  EXPECT(rejects_grammar(domain, *monograph, "(.x = 1) + 2"_view));
  EXPECT(rejects_grammar(domain, *monograph, "!(.x = true)"_view));
  EXPECT(rejects_grammar(domain, *monograph, "(1, 2):[0]"_view));
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, malformed_grammar_is_atomic) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);

  EXPECT(rejects_grammar(domain, *monograph, "2 *"_view));
  EXPECT(rejects_grammar(domain, *monograph, "2 +"_view));
  EXPECT(rejects_grammar(domain, *monograph, "true and"_view));
  EXPECT(rejects_grammar(domain, *monograph, "false or"_view));
  EXPECT(rejects_grammar(domain, *monograph, "!"_view));
  EXPECT(rejects_grammar(domain, *monograph, "receiver."_view));
  EXPECT(rejects_grammar(domain, *monograph, "\"abc\":[]"_view));
  EXPECT(rejects_grammar(domain, *monograph, "\"abc\":[1,]"_view));
  EXPECT(rejects_grammar(domain, *monograph, "\"abc\":[1, 1"_view));
  EXPECT(rejects_grammar(domain, *monograph, "\"abc\":[1 1]"_view));
}

PERIMORTEM_UNIT_TEST(ExpressionParserTests, bitwise_symbols_are_not_logical) {
  Allocator::Arena domain;
  ExpressionParserObservations observations;
  ExpressionParserContext context(domain, observations);
  Library::Dialect dialect;
  auto monograph = create_monograph(domain, dialect, context);
  ASSERT(monograph);
  Errors and_errors;
  Errors or_errors;

  EXPECT_NOT(parse_one(domain, *monograph, "true & false"_view, and_errors));
  EXPECT_NOT(parse_one(domain, *monograph, "false | true"_view, or_errors));
  EXPECT(and_errors.is_empty());
  EXPECT(or_errors.is_empty());
}
