// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/and.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/signed_8.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness LibraryAnd = {
  .name = "Tetrodotoxin::Library::Language::Operations::And"_view,
};

class AndMonograph : public Tetrodotoxin::Language::Monograph {
 public:
  AndMonograph(Allocator::Arena& domain)
      : Tetrodotoxin::Language::Monograph(domain, Documentation::get_empty()) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "AndMonograph"_view;
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

static auto link_operation(
    Operation& operation,
    Tetrodotoxin::Language::Monograph& source) -> Bool {
  return operation.link(source, Invalid::get_invalid());
}

class AndExpression : public Expression {
 public:
  AndExpression(View::Bytes name, const Abstract& type)
      : Expression({}), name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Abstract& override { return type; }

 private:
  View::Bytes name;
  const Abstract& type;
};

class AndFoldInput : public Operation {
 public:
  AndFoldInput(
      Allocator::Arena& domain,
      Expression& input,
      Constant& result,
      Bool fails = False)
      : Operation(
            domain,
            Static::Vector<Reference<Expression>, 1>{{input}},
            {}),
        result(result),
        fails(fails) {}

  auto get_name() const -> View::Bytes override { return "And input"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_evaluations() const -> Count { return evaluations; }

 protected:
  auto evaluate_constants(Allocator::Arena&)
      -> Result<Option<Constant&>, Expression::Error> override {
    evaluations++;
    if (fails) {
      return Expression::Error(Expression::Error::Type::InvalidConstant, *this);
    }

    return result;
  }

  auto select_type(Tetrodotoxin::Language::Monograph&) const
      -> Option<const Ttx::Model::Type&> override {
    return Tetrodotoxin::Library::Dialect::get_bool();
  }

 private:
  Constant& result;
  Bool fails;
  Count evaluations = 0;
};

static auto selected(
    const Result<Option<Expression&>, Expression::Error>& result)
    -> Option<Expression&> {
  return result.visit(
      [](const Option<Expression&>& folded) -> Option<Expression&> {
        return folded.visit(
            []() -> Option<Expression&> { return {}; },
            [](Expression& expression) -> Option<Expression&> {
              return expression;
            });
      },
      [](const Expression::Error&) -> Option<Expression&> { return {}; });
}

static auto reports(
    const Result<Option<Expression&>, Expression::Error>& result,
    Expression::Error::Type expected,
    const Expression& origin) -> Bool {
  return result.visit(
      [](const Option<Expression&>&) { return False; },
      [&](const Expression::Error& error) {
        return error.get_type() == expected &&
                       &error.get_expression() == &origin
                   ? True
                   : False;
      });
}

static auto is_dynamic(
    const Result<Option<Expression&>, Expression::Error>& result) -> Bool {
  return result.visit(
      [](const Option<Expression&>& folded) { return !folded ? True : False; },
      [](const Expression::Error&) { return False; });
}

static auto matches_anchor(
    const Expression& expression,
    View::Bytes source,
    View::Bytes focus,
    View::Bytes span) -> Bool {
  return expression.get_anchor().visit(
      []() { return False; },
      [&](const Anchor& anchor) {
        return anchor.get_token().caculate_text(source) == focus &&
                       anchor.get_span().caculate_text(source) == span
                   ? True
                   : False;
      });
}

PERIMORTEM_UNIT_TEST(LibraryAnd, exact_type_and_edges) {
  Allocator::Arena domain;
  AndMonograph source(domain);
  Types::Boolean distinct_bool;
  Types::Signed_8 signed_8;
  AndExpression canonical_left(
      "canonical left"_view, Tetrodotoxin::Library::Dialect::get_bool());
  AndExpression canonical_right(
      "canonical right"_view, Tetrodotoxin::Library::Dialect::get_bool());
  AndExpression distinct("distinct"_view, distinct_bool);
  AndExpression signed_value("signed"_view, signed_8);
  AndExpression invalid("invalid"_view, Invalid::get_invalid());
  auto& canonical = Operations::And::create_synthetic(
      domain, canonical_left, canonical_right);
  auto& distinct_left =
      Operations::And::create_synthetic(domain, distinct, canonical_right);
  auto& distinct_right =
      Operations::And::create_synthetic(domain, canonical_left, distinct);
  auto& signed_operation =
      Operations::And::create_synthetic(domain, canonical_left, signed_value);
  auto& invalid_operation =
      Operations::And::create_synthetic(domain, invalid, canonical_right);

  EXPECT(canonical.get_type().resolve().is<Invalid>());
  EXPECT_NOT(canonical.get_anchor());
  EXPECT(link_operation(canonical, source));
  EXPECT_NOT(link_operation(distinct_left, source));
  EXPECT_NOT(link_operation(distinct_right, source));
  EXPECT_NOT(link_operation(signed_operation, source));
  EXPECT_NOT(link_operation(invalid_operation, source));

  EXPECT(&canonical.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(is_dynamic(canonical.fold()));
  EXPECT(distinct_left.get_type().resolve().is<Invalid>());
  EXPECT(distinct_right.get_type().resolve().is<Invalid>());
  EXPECT(signed_operation.get_type().resolve().is<Invalid>());
  EXPECT(invalid_operation.get_type().resolve().is<Invalid>());
}

PERIMORTEM_UNIT_TEST(LibraryAnd, truth_table_and_repetition) {
  Allocator::Arena domain;
  AndMonograph source(domain);
  auto& true_value = Constants::True::create_synthetic(
      domain, Tetrodotoxin::Library::Dialect::get_bool());
  auto& false_value = Constants::False::create_synthetic(
      domain, Tetrodotoxin::Library::Dialect::get_bool());
  auto& true_true =
      Operations::And::create_synthetic(domain, true_value, true_value);
  auto& true_false =
      Operations::And::create_synthetic(domain, true_value, false_value);
  auto& false_true =
      Operations::And::create_synthetic(domain, false_value, true_value);
  auto& false_false =
      Operations::And::create_synthetic(domain, false_value, false_value);

  EXPECT(link_operation(true_true, source));
  EXPECT(link_operation(true_false, source));
  EXPECT(link_operation(false_true, source));
  EXPECT(link_operation(false_false, source));

  auto both = selected(true_true.fold());
  auto left = selected(true_false.fold());
  auto right = selected(false_true.fold());
  auto neither = selected(false_false.fold());
  auto repeated = selected(true_true.fold());

  ASSERT(both && left && right && neither && repeated);
  EXPECT(both->is<Constants::True>());
  EXPECT(left->is<Constants::False>());
  EXPECT(right->is<Constants::False>());
  EXPECT(neither->is<Constants::False>());
  EXPECT(&*both == &*repeated);
  EXPECT(&both->get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(&left->get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
}

PERIMORTEM_UNIT_TEST(LibraryAnd, ordered_reachability) {
  Allocator::Arena domain;
  AndMonograph source(domain);
  auto& true_value = Constants::True::create_synthetic(
      domain, Tetrodotoxin::Library::Dialect::get_bool());
  auto& false_value = Constants::False::create_synthetic(
      domain, Tetrodotoxin::Library::Dialect::get_bool());
  AndExpression dynamic(
      "dynamic"_view, Tetrodotoxin::Library::Dialect::get_bool());
  AndFoldInput skipped_failure(domain, true_value, true_value, True);
  AndFoldInput reached_failure(domain, true_value, true_value, True);
  AndFoldInput dynamic_failure(domain, true_value, true_value, True);
  auto& skipped =
      Operations::And::create_synthetic(domain, false_value, skipped_failure);
  auto& reached =
      Operations::And::create_synthetic(domain, true_value, reached_failure);
  auto& dynamic_left =
      Operations::And::create_synthetic(domain, dynamic, dynamic_failure);

  EXPECT(link_operation(skipped, source));
  EXPECT(link_operation(reached, source));
  EXPECT(link_operation(dynamic_left, source));

  auto skipped_result = selected(skipped.fold());

  ASSERT(skipped_result);
  EXPECT(skipped_result->is<Constants::False>());
  EXPECT(skipped_failure.get_evaluations() == 0);
  EXPECT(reports(
      reached.fold(), Expression::Error::Type::InvalidConstant,
      reached_failure));
  EXPECT(reached_failure.get_evaluations() == 1);
  EXPECT(reports(
      dynamic_left.fold(), Expression::Error::Type::InvalidConstant,
      dynamic_failure));
  EXPECT(dynamic_failure.get_evaluations() == 1);
}

PERIMORTEM_UNIT_TEST(LibraryAnd, authored_parse_and_atomic_failure) {
  static constexpr View::Bytes success_source = "true & false"_view;
  Allocator::Arena domain;
  AndMonograph context(domain);
  Tetrodotoxin::Library::Dialect dialect;
  Errors host_errors;
  Tokenizer host_tokens(domain, {}, "and-source.ttx"_view);
  Cursor host_cursor(host_tokens, host_errors);
  auto retained_source = dialect.interpret(
      domain, host_cursor, Documentation::get_empty(), Anchor::create(Span()),
      context);
  ASSERT(retained_source && retained_source->is<Monograph>());
  auto& source = static_cast<Monograph&>(*retained_source);
  Errors success_errors;
  Tokenizer success_tokens(domain, success_source, "and.ttx"_view);
  Cursor success_cursor(success_tokens, success_errors);
  Token success_left_token = success_cursor.consume();
  auto success_left_anchor =
      Anchor::create(success_left_token, Span(success_left_token));
  auto& success_left = Constants::True::create_authored(
      domain, Tetrodotoxin::Library::Dialect::get_bool(), success_left_anchor);
  auto parsed = Operations::And::parse(
      domain, source, success_cursor, success_left, Span(success_left_token));

  ASSERT(parsed && parsed->is<Operations::And>());
  EXPECT(parsed->get_type().resolve().is<Invalid>());
  EXPECT(matches_anchor(*parsed, success_source, "&"_view, success_source));
  EXPECT(success_cursor.matches(Code::Type::Terminal));
  EXPECT(success_errors.is_empty());
  EXPECT(parsed->link(source, Invalid::get_invalid()));

  Errors failure_errors;
  Tokenizer failure_tokens(domain, "true &"_view, "and.ttx"_view);
  Cursor failure_cursor(failure_tokens, failure_errors);
  Token failure_left_token = failure_cursor.consume();
  auto failure_left_anchor =
      Anchor::create(failure_left_token, Span(failure_left_token));
  auto& failure_left = Constants::True::create_authored(
      domain, Tetrodotoxin::Library::Dialect::get_bool(), failure_left_anchor);
  Token operation = failure_cursor.current();
  auto rejected = Operations::And::parse(
      domain, source, failure_cursor, failure_left, Span(failure_left_token));

  EXPECT_NOT(rejected);
  EXPECT(failure_cursor.current().get_offset() == operation.get_offset());
  EXPECT(failure_cursor.current().get_code() == operation.get_code());
  EXPECT_NOT(failure_errors.is_empty());

  Errors mismatch_errors;
  Tokenizer mismatch_tokens(domain, "true & 1"_view, "and.ttx"_view);
  Cursor mismatch_cursor(mismatch_tokens, mismatch_errors);
  auto mismatch = Parser::Expression::parse(domain, source, mismatch_cursor);

  ASSERT(mismatch && mismatch->is<Operations::And>());
  EXPECT(mismatch_errors.is_empty());
  EXPECT_NOT(mismatch->link(source, Invalid::get_invalid()));
  auto diagnostics = source.get_diagnostics();
  ASSERT(diagnostics.get_size() == 1);
  ASSERT(diagnostics.get_data()[0].get_anchor());
  EXPECT_TEXT(
      diagnostics.get_data()[0].get_anchor()->get_span().caculate_text(
          "true & 1"_view),
      "true & 1"_view);
}
