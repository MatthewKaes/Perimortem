// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/not_equal.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/real_64.hpp"
#include "tetrodotoxin/library/language/types/signed_8.hpp"
#include "tetrodotoxin/library/language/types/unsigned_16.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness LibraryNotEqual = {
  .name = "Tetrodotoxin::Library::Language::Operations::NotEqual"_view,
};

class NotEqualMonograph : public Tetrodotoxin::Language::Monograph {
 public:
  NotEqualMonograph(Allocator::Arena& domain)
      : Tetrodotoxin::Language::Monograph(domain, Documentation::get_empty()) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "NotEqualMonograph"_view;
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

static auto link_operation(
    Operation& operation,
    NotEqualMonograph& source,
    Materializations& materializations) -> Bool {
  return operation.link(source, Invalid::get_invalid(), materializations);
}

class NotEqualExpression : public Expression {
 public:
  NotEqualExpression(View::Bytes name, const Abstract& type)
      : Expression({}), name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Abstract& override { return type; }
  auto get_inputs() const -> const Layout& override { return inputs; }

 private:
  View::Bytes name;
  const Abstract& type;
  Ttx::Model::Layouts::Fluid inputs;
};

class NotEqualUnresolvedType : public Ttx::Model::Type {
 public:
  auto get_name() const -> View::Bytes override { return "Unresolved"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve() const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

class NotEqualFoldInput : public Operation {
 public:
  NotEqualFoldInput(
      Allocator::Arena& domain,
      Materializations& materializations,
      Expression& input,
      Constant& result,
      const Ttx::Model::Type& type,
      Bool fails = False)
      : Operation(
            domain,
            materializations,
            Static::Vector<Reference<Expression>, 1>{{input}},
            {}),
        result(result),
        type(type),
        fails(fails) {}

  auto get_name() const -> View::Bytes override { return "Fold input"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_evaluations() const -> Count { return evaluations; }

 protected:
  auto evaluate_constants(Allocator::Arena&, Materializations&)
      -> Result<Option<Constant&>, Expression::Error> override {
    evaluations++;
    if (fails) {
      return Expression::Error(Expression::Error::Type::InvalidConstant, *this);
    }

    return result;
  }

  auto select_type(Materializations&) const
      -> Option<const Ttx::Model::Type&> override {
    return type;
  }

 private:
  Constant& result;
  const Ttx::Model::Type& type;
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
            [](Expression& selected) -> Option<Expression&> {
              return selected;
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

static auto input_is(
    const Operations::NotEqual& not_equal,
    Count index,
    const Expression& expected) -> Bool {
  return not_equal.get_inputs().get_abstract(index).visit(
      []() { return False; },
      [&](const Abstract& expression) {
        return &expression == &expected ? True : False;
      });
}

PERIMORTEM_UNIT_TEST(LibraryNotEqual, type_selection_and_partial) {
  Allocator::Arena domain;
  NotEqualMonograph source(domain);
  Materializations materializations(domain);
  Types::Signed_8 signed_8;
  Types::Unsigned_8 unsigned_8;
  Types::Unsigned_16 unsigned_16;
  Types::Real_64 real_64;
  Types::Boolean boolean;
  Types::Fixed bytes_type(
      "Fixed[Unsigned_8,1]"_view,
      Tetrodotoxin::Library::Dialect::get_unsigned_8(), 1);
  Types::Fixed other_bytes_type(
      "Fixed[Unsigned_8,2]"_view,
      Tetrodotoxin::Library::Dialect::get_unsigned_8(), 2);
  NotEqualUnresolvedType unresolved_type;
  NotEqualExpression signed_left("signed left"_view, signed_8);
  NotEqualExpression signed_right("signed right"_view, signed_8);
  NotEqualExpression unsigned_left("unsigned left"_view, unsigned_8);
  NotEqualExpression unsigned_right("unsigned right"_view, unsigned_8);
  NotEqualExpression real_left("real left"_view, real_64);
  NotEqualExpression real_right("real right"_view, real_64);
  NotEqualExpression flag_left("flag left"_view, boolean);
  NotEqualExpression flag_right("flag right"_view, boolean);
  NotEqualExpression other("other"_view, unsigned_16);
  NotEqualExpression unresolved("unresolved"_view, unresolved_type);
  NotEqualExpression invalid("invalid"_view, Invalid::get_invalid());
  NotEqualExpression dynamic_bytes("dynamic bytes"_view, bytes_type);
  auto& bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "x"_view);
  auto& same_bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "x"_view);
  auto& other_bytes =
      Constants::Bytes::create_synthetic(domain, other_bytes_type, "xx"_view);
  auto& signed_exact = Operations::NotEqual::create_synthetic(
      domain, materializations, signed_left, signed_right);
  auto& unsigned_exact = Operations::NotEqual::create_synthetic(
      domain, materializations, unsigned_left, unsigned_right);
  auto& real_exact = Operations::NotEqual::create_synthetic(
      domain, materializations, real_left, real_right);
  auto& flag_exact = Operations::NotEqual::create_synthetic(
      domain, materializations, flag_left, flag_right);
  auto& byte_values = Operations::NotEqual::create_synthetic(
      domain, materializations, bytes, same_bytes);
  auto& mismatch = Operations::NotEqual::create_synthetic(
      domain, materializations, unsigned_left, other);
  auto& unresolved_pair = Operations::NotEqual::create_synthetic(
      domain, materializations, unresolved, unresolved);
  auto& invalid_pair = Operations::NotEqual::create_synthetic(
      domain, materializations, invalid, invalid);
  auto& incomplete_bytes = Operations::NotEqual::create_synthetic(
      domain, materializations, dynamic_bytes, bytes);
  auto& byte_mismatch = Operations::NotEqual::create_synthetic(
      domain, materializations, bytes, other_bytes);

  EXPECT(signed_exact.get_type().resolve().is<Invalid>());
  EXPECT_NOT(signed_exact.get_anchor());
  EXPECT(link_operation(signed_exact, source, materializations));
  EXPECT(link_operation(unsigned_exact, source, materializations));
  EXPECT(link_operation(real_exact, source, materializations));
  EXPECT(link_operation(flag_exact, source, materializations));
  EXPECT(link_operation(byte_values, source, materializations));
  EXPECT(!link_operation(mismatch, source, materializations));
  EXPECT(!link_operation(unresolved_pair, source, materializations));
  EXPECT(!link_operation(invalid_pair, source, materializations));
  EXPECT(!link_operation(incomplete_bytes, source, materializations));
  EXPECT(!link_operation(byte_mismatch, source, materializations));

  auto retained = selected(unsigned_exact.fold());

  EXPECT(
      &signed_exact.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(
      &unsigned_exact.get_type() ==
      &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(&real_exact.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(&flag_exact.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(
      &byte_values.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT_NOT(retained);
  EXPECT(input_is(unsigned_exact, 0, unsigned_left));
  EXPECT(input_is(unsigned_exact, 1, unsigned_right));
  EXPECT(mismatch.get_type().resolve().is<Invalid>());
  EXPECT(unresolved_pair.get_type().resolve().is<Invalid>());
  EXPECT(invalid_pair.get_type().resolve().is<Invalid>());
  EXPECT(incomplete_bytes.get_type().resolve().is<Invalid>());
  EXPECT(byte_mismatch.get_type().resolve().is<Invalid>());
}

PERIMORTEM_UNIT_TEST(LibraryNotEqual, complete_constant_domains) {
  Allocator::Arena domain;
  NotEqualMonograph source(domain);
  Materializations materializations(domain);
  Types::Signed_8 signed_type;
  Types::Unsigned_8 unsigned_type;
  Types::Real_64 real_type;
  Types::Boolean boolean;
  Types::Fixed bytes_type(
      "Fixed[Unsigned_8,2]"_view,
      Tetrodotoxin::Library::Dialect::get_unsigned_8(), 2);
  auto& signed_value =
      Constants::Signed::create_synthetic(domain, signed_type, -8);
  auto& same_signed =
      Constants::Signed::create_synthetic(domain, signed_type, -8);
  auto& other_signed =
      Constants::Signed::create_synthetic(domain, signed_type, 8);
  auto& unsigned_value =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 8);
  auto& same_unsigned =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 8);
  auto& other_unsigned =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 9);
  auto& real_value = Constants::Real::create_synthetic(domain, real_type, 0.5);
  auto& same_real = Constants::Real::create_synthetic(domain, real_type, 0.5);
  auto& other_real = Constants::Real::create_synthetic(domain, real_type, 1.0);
  auto& truth = Constants::True::create_synthetic(domain, boolean);
  auto& same_truth = Constants::True::create_synthetic(domain, boolean);
  auto& falsity = Constants::False::create_synthetic(domain, boolean);
  auto& bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "ab"_view);
  auto& same_bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "ab"_view);
  auto& other_bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "ac"_view);
  auto& signed_same = Operations::NotEqual::create_synthetic(
      domain, materializations, signed_value, same_signed);
  auto& signed_different = Operations::NotEqual::create_synthetic(
      domain, materializations, signed_value, other_signed);
  auto& unsigned_same = Operations::NotEqual::create_synthetic(
      domain, materializations, unsigned_value, same_unsigned);
  auto& unsigned_different = Operations::NotEqual::create_synthetic(
      domain, materializations, unsigned_value, other_unsigned);
  auto& real_same = Operations::NotEqual::create_synthetic(
      domain, materializations, real_value, same_real);
  auto& real_different = Operations::NotEqual::create_synthetic(
      domain, materializations, real_value, other_real);
  auto& flag_same = Operations::NotEqual::create_synthetic(
      domain, materializations, truth, same_truth);
  auto& flag_different = Operations::NotEqual::create_synthetic(
      domain, materializations, truth, falsity);
  auto& bytes_same = Operations::NotEqual::create_synthetic(
      domain, materializations, bytes, same_bytes);
  auto& bytes_different = Operations::NotEqual::create_synthetic(
      domain, materializations, bytes, other_bytes);

  EXPECT(signed_same.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(signed_same, source, materializations));
  EXPECT(link_operation(signed_different, source, materializations));
  EXPECT(link_operation(unsigned_same, source, materializations));
  EXPECT(link_operation(unsigned_different, source, materializations));
  EXPECT(link_operation(real_same, source, materializations));
  EXPECT(link_operation(real_different, source, materializations));
  EXPECT(link_operation(flag_same, source, materializations));
  EXPECT(link_operation(flag_different, source, materializations));
  EXPECT(link_operation(bytes_same, source, materializations));
  EXPECT(link_operation(bytes_different, source, materializations));

  auto signed_no = selected(signed_same.fold());
  auto signed_yes = selected(signed_different.fold());
  auto unsigned_no = selected(unsigned_same.fold());
  auto unsigned_yes = selected(unsigned_different.fold());
  auto real_no = selected(real_same.fold());
  auto real_yes = selected(real_different.fold());
  auto flag_no = selected(flag_same.fold());
  auto flag_yes = selected(flag_different.fold());
  auto bytes_no = selected(bytes_same.fold());
  auto bytes_yes = selected(bytes_different.fold());

  ASSERT(
      signed_no && signed_yes && unsigned_no && unsigned_yes && real_no &&
      real_yes && flag_no && flag_yes && bytes_no && bytes_yes);
  EXPECT(signed_no->is<Constants::False>());
  EXPECT(signed_yes->is<Constants::True>());
  EXPECT(unsigned_no->is<Constants::False>());
  EXPECT(unsigned_yes->is<Constants::True>());
  EXPECT(real_no->is<Constants::False>());
  EXPECT(real_yes->is<Constants::True>());
  EXPECT(flag_no->is<Constants::False>());
  EXPECT(flag_yes->is<Constants::True>());
  EXPECT(bytes_no->is<Constants::False>());
  EXPECT(bytes_yes->is<Constants::True>());
}

PERIMORTEM_UNIT_TEST(LibraryNotEqual, real_equivalence_inverse) {
  Allocator::Arena domain;
  NotEqualMonograph source(domain);
  Materializations materializations(domain);
  Types::Real_64 real_type;
  auto& left_nan = Constants::Real::create_synthetic(
      domain, real_type, __builtin_nan("left"));
  auto& right_nan = Constants::Real::create_synthetic(
      domain, real_type, __builtin_nan("right"));
  auto& finite = Constants::Real::create_synthetic(domain, real_type, 1.0);
  auto& positive_zero =
      Constants::Real::create_synthetic(domain, real_type, 0.0);
  auto& negative_zero =
      Constants::Real::create_synthetic(domain, real_type, -0.0);
  auto& nan_pair = Operations::NotEqual::create_synthetic(
      domain, materializations, left_nan, right_nan);
  auto& nan_finite = Operations::NotEqual::create_synthetic(
      domain, materializations, left_nan, finite);
  auto& signed_zero = Operations::NotEqual::create_synthetic(
      domain, materializations, positive_zero, negative_zero);

  EXPECT(nan_pair.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(nan_pair, source, materializations));
  EXPECT(link_operation(nan_finite, source, materializations));
  EXPECT(link_operation(signed_zero, source, materializations));

  auto nan_same = selected(nan_pair.fold());
  auto nan_other = selected(nan_finite.fold());
  auto zeros = selected(signed_zero.fold());

  ASSERT(nan_same && nan_other && zeros);
  EXPECT(nan_same->is<Constants::False>());
  EXPECT(nan_other->is<Constants::True>());
  EXPECT(zeros->is<Constants::False>());
}

PERIMORTEM_UNIT_TEST(LibraryNotEqual, recursive_provenance_and_atomicity) {
  Allocator::Arena domain;
  NotEqualMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 selected_type;
  auto& input = Constants::Unsigned::create_synthetic(domain, selected_type, 1);
  auto& folded =
      Constants::Unsigned::create_synthetic(domain, selected_type, 8);
  auto& right = Constants::Unsigned::create_synthetic(domain, selected_type, 7);
  auto& wrong_domain =
      Constants::Bytes::create_synthetic(domain, selected_type, "x"_view);
  NotEqualFoldInput child(
      domain, materializations, input, folded, selected_type);
  NotEqualFoldInput failing(
      domain, materializations, input, folded, selected_type, True);
  NotEqualFoldInput invalid_child(
      domain, materializations, input, wrong_domain, selected_type);
  auto& not_equal = Operations::NotEqual::create_synthetic(
      domain, materializations, child, right);
  auto& failure = Operations::NotEqual::create_synthetic(
      domain, materializations, failing, right);
  auto& invalid_constant = Operations::NotEqual::create_synthetic(
      domain, materializations, invalid_child, right);

  EXPECT(not_equal.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(not_equal, source, materializations));
  EXPECT(link_operation(not_equal, source, materializations));
  EXPECT(link_operation(failure, source, materializations));
  EXPECT(link_operation(invalid_constant, source, materializations));

  auto first = selected(not_equal.fold());
  auto second = selected(not_equal.fold());

  ASSERT(first && second);
  EXPECT(&*first == &*second);
  EXPECT(first->is<Constants::True>());
  EXPECT(input_is(not_equal, 0, child));
  EXPECT(child.get_evaluations() == 1);
  EXPECT(reports(
      failure.fold(), Expression::Error::Type::InvalidConstant, failing));
  EXPECT(reports(
      invalid_constant.fold(), Expression::Error::Type::InvalidConstant,
      invalid_child));

  const auto& parser_type = Tetrodotoxin::Library::Dialect::get_unsigned_64();
  Errors success_errors;
  Tokenizer success_tokens(domain, "2 != 2"_view, "not_equal.ttx"_view);
  Cursor success_cursor(success_tokens, success_errors);
  Token success_left_token = success_cursor.consume();
  auto success_left_anchor =
      Anchor::create(success_left_token, Span(success_left_token));
  auto& success_left = Constants::Unsigned::create_authored(
      domain, parser_type, 2, success_left_anchor);
  auto parsed = Operations::NotEqual::parse(
      domain, materializations, success_cursor, Invalid::get_invalid(),
      success_left);
  Errors failure_errors;
  Tokenizer failure_tokens(domain, "2 != true"_view, "not_equal.ttx"_view);
  Cursor failure_cursor(failure_tokens, failure_errors);
  Token failure_left_token = failure_cursor.consume();
  auto failure_left_anchor =
      Anchor::create(failure_left_token, Span(failure_left_token));
  auto& failure_left = Constants::Unsigned::create_authored(
      domain, parser_type, 2, failure_left_anchor);
  auto rejected = Operations::NotEqual::parse(
      domain, materializations, failure_cursor, Invalid::get_invalid(),
      failure_left);

  ASSERT(parsed);
  EXPECT(parsed->is<Operations::NotEqual>());
  EXPECT(parsed->get_type().resolve().is<Invalid>());
  EXPECT(success_cursor.matches(Code::Type::Terminal));
  EXPECT(success_errors.is_empty());
  EXPECT(parsed->link(source, Invalid::get_invalid(), materializations));

  auto parsed_fold = parsed->visit<Operation>(
      [&](Operation& operation) { return selected(operation.fold()); },
      [](Abstract&) -> Option<Expression&> { return {}; });

  ASSERT(parsed_fold);
  EXPECT(parsed_fold->is<Constants::False>());
  EXPECT(&parsed->get_type() == &Tetrodotoxin::Library::Dialect::get_bool());

  ASSERT(rejected);
  EXPECT(rejected->is<Operations::NotEqual>());
  EXPECT(rejected->get_type().resolve().is<Invalid>());
  EXPECT(failure_cursor.matches(Code::Type::Terminal));
  EXPECT(failure_errors.is_empty());
  EXPECT_NOT(rejected->link(source, Invalid::get_invalid(), materializations));
  EXPECT(rejected->get_type().resolve().is<Invalid>());

  auto diagnostics = source.get_diagnostics();
  ASSERT(diagnostics.get_size() == 1);
  ASSERT(diagnostics.get_data()[0].get_anchor());
  EXPECT(
      diagnostics.get_data()[0].get_anchor()->get_span().get_size() ==
      Count(9));
}
