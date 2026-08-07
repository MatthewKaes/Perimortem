// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/divide.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/real_32.hpp"
#include "tetrodotoxin/library/language/types/real_64.hpp"
#include "tetrodotoxin/library/language/types/signed_8.hpp"
#include "tetrodotoxin/library/language/types/unsigned_16.hpp"
#include "tetrodotoxin/library/language/types/unsigned_64.hpp"
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

static Harness LibraryDivide = {
  .name = "Tetrodotoxin::Library::Language::Operations::Divide"_view,
};

class DivideMonograph : public Tetrodotoxin::Language::Monograph {
 public:
  DivideMonograph(Allocator::Arena& domain)
      : Tetrodotoxin::Language::Monograph(domain, Documentation::get_empty()) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "DivideMonograph"_view;
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

static auto link_operation(
    Operation& operation,
    DivideMonograph& source,
    Materializations& materializations) -> Bool {
  return operation.link(source, Invalid::get_invalid(), materializations);
}

class DivideExpression : public Expression {
 public:
  DivideExpression(View::Bytes name, const Abstract& type)
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

class DivideUnresolvedType : public Ttx::Model::Type {
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

class DivideFoldInput : public Operation {
 public:
  DivideFoldInput(
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

template <typename constant_type, typename value_type>
static auto get_value(const Expression& expression) -> Option<value_type> {
  return expression.visit<constant_type>(
      [](const constant_type& constant) -> Option<value_type> {
        return constant.get_value();
      },
      [](const Abstract&) -> Option<value_type> { return {}; });
}

template <typename constant_type, typename value_type>
static auto value_is(const Expression& expression, value_type expected)
    -> Bool {
  auto value = get_value<constant_type, value_type>(expression);
  return value && *value == expected ? True : False;
}

static auto input_is(
    const Operations::Divide& divide,
    Count index,
    const Expression& expected) -> Bool {
  return divide.get_inputs().get_abstract(index).visit(
      []() { return False; },
      [&](const Abstract& expression) {
        return &expression == &expected ? True : False;
      });
}

PERIMORTEM_UNIT_TEST(LibraryDivide, type_selection) {
  Allocator::Arena domain;
  DivideMonograph source(domain);
  Materializations materializations(domain);
  Types::Signed_8 signed_8;
  Types::Unsigned_8 unsigned_8;
  Types::Unsigned_16 unsigned_16;
  Types::Real_32 real_32;
  Types::Boolean boolean;
  Types::Fixed bytes_type(
      "Fixed[Unsigned_8,1]"_view,
      Tetrodotoxin::Library::Dialect::get_unsigned_8(), 1);
  DivideUnresolvedType unresolved_type;
  DivideExpression signed_left("signed left"_view, signed_8);
  DivideExpression signed_right("signed right"_view, signed_8);
  DivideExpression unsigned_left("unsigned left"_view, unsigned_8);
  DivideExpression unsigned_right("unsigned right"_view, unsigned_8);
  DivideExpression real_left("real left"_view, real_32);
  DivideExpression real_right("real right"_view, real_32);
  DivideExpression other("other"_view, unsigned_16);
  DivideExpression unresolved("unresolved"_view, unresolved_type);
  DivideExpression invalid("invalid"_view, Invalid::get_invalid());
  auto& truth = Constants::True::create_synthetic(domain, boolean);
  auto& bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "x"_view);
  auto& signed_exact = Operations::Divide::create_synthetic(
      domain, materializations, signed_left, signed_right);
  auto& unsigned_exact = Operations::Divide::create_synthetic(
      domain, materializations, unsigned_left, unsigned_right);
  auto& real_exact = Operations::Divide::create_synthetic(
      domain, materializations, real_left, real_right);
  auto& mismatch = Operations::Divide::create_synthetic(
      domain, materializations, unsigned_left, other);
  auto& unresolved_pair = Operations::Divide::create_synthetic(
      domain, materializations, unresolved, unresolved);
  auto& invalid_pair = Operations::Divide::create_synthetic(
      domain, materializations, invalid, invalid);
  auto& flags = Operations::Divide::create_synthetic(
      domain, materializations, truth, truth);
  auto& byte_values = Operations::Divide::create_synthetic(
      domain, materializations, bytes, bytes);

  EXPECT(signed_exact.get_type().resolve().is<Invalid>());
  EXPECT_NOT(signed_exact.get_anchor());
  EXPECT(link_operation(signed_exact, source, materializations));
  EXPECT(link_operation(unsigned_exact, source, materializations));
  EXPECT(link_operation(real_exact, source, materializations));
  EXPECT(!link_operation(mismatch, source, materializations));
  EXPECT(!link_operation(unresolved_pair, source, materializations));
  EXPECT(!link_operation(invalid_pair, source, materializations));
  EXPECT(!link_operation(flags, source, materializations));
  EXPECT(!link_operation(byte_values, source, materializations));

  auto retained = selected(unsigned_exact.fold());

  EXPECT(&signed_exact.get_type() == &signed_8);
  EXPECT(&unsigned_exact.get_type() == &unsigned_8);
  EXPECT(&real_exact.get_type() == &real_32);
  EXPECT_NOT(retained);
  EXPECT(input_is(unsigned_exact, 0, unsigned_left));
  EXPECT(input_is(unsigned_exact, 1, unsigned_right));
  EXPECT(mismatch.get_type().resolve().is<Invalid>());
  EXPECT(unresolved_pair.get_type().resolve().is<Invalid>());
  EXPECT(invalid_pair.get_type().resolve().is<Invalid>());
  EXPECT(flags.get_type().resolve().is<Invalid>());
  EXPECT(byte_values.get_type().resolve().is<Invalid>());
}

PERIMORTEM_UNIT_TEST(LibraryDivide, integer_quotients) {
  Allocator::Arena domain;
  DivideMonograph source(domain);
  Materializations materializations(domain);
  Types::Signed_8 signed_type;
  Types::Unsigned_8 unsigned_type;
  auto& positive = Constants::Signed::create_synthetic(domain, signed_type, 7);
  auto& negative = Constants::Signed::create_synthetic(domain, signed_type, -7);
  auto& two = Constants::Signed::create_synthetic(domain, signed_type, 2);
  auto& negative_two =
      Constants::Signed::create_synthetic(domain, signed_type, -2);
  auto& signed_zero =
      Constants::Signed::create_synthetic(domain, signed_type, 0);
  auto& signed_one =
      Constants::Signed::create_synthetic(domain, signed_type, 1);
  auto& negative_one =
      Constants::Signed::create_synthetic(domain, signed_type, -1);
  auto& minimum =
      Constants::Signed::create_synthetic(domain, signed_type, -128);
  auto& invalid_signed =
      Constants::Signed::create_synthetic(domain, signed_type, 128);
  auto& seven = Constants::Unsigned::create_synthetic(domain, unsigned_type, 7);
  auto& unsigned_two =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 2);
  auto& unsigned_zero =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 0);
  auto& unsigned_one =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 1);
  auto& maximum =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 255);
  auto& invalid_unsigned =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 256);
  auto& positive_result = Operations::Divide::create_synthetic(
      domain, materializations, positive, two);
  auto& negative_result = Operations::Divide::create_synthetic(
      domain, materializations, negative, two);
  auto& opposite_sign = Operations::Divide::create_synthetic(
      domain, materializations, positive, negative_two);
  auto& signed_zero_result = Operations::Divide::create_synthetic(
      domain, materializations, signed_zero, signed_one);
  auto& minimum_result = Operations::Divide::create_synthetic(
      domain, materializations, minimum, signed_one);
  auto& signed_zero_divisor = Operations::Divide::create_synthetic(
      domain, materializations, positive, signed_zero);
  auto& endpoint_overflow = Operations::Divide::create_synthetic(
      domain, materializations, minimum, negative_one);
  auto& signed_width = Operations::Divide::create_synthetic(
      domain, materializations, invalid_signed, signed_one);
  auto& unsigned_result = Operations::Divide::create_synthetic(
      domain, materializations, seven, unsigned_two);
  auto& unsigned_zero_result = Operations::Divide::create_synthetic(
      domain, materializations, unsigned_zero, unsigned_one);
  auto& unsigned_endpoint = Operations::Divide::create_synthetic(
      domain, materializations, maximum, unsigned_one);
  auto& unsigned_zero_divisor = Operations::Divide::create_synthetic(
      domain, materializations, maximum, unsigned_zero);
  auto& unsigned_width = Operations::Divide::create_synthetic(
      domain, materializations, invalid_unsigned, unsigned_one);

  EXPECT(positive_result.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(positive_result, source, materializations));
  EXPECT(link_operation(negative_result, source, materializations));
  EXPECT(link_operation(opposite_sign, source, materializations));
  EXPECT(link_operation(signed_zero_result, source, materializations));
  EXPECT(link_operation(minimum_result, source, materializations));
  EXPECT(link_operation(signed_zero_divisor, source, materializations));
  EXPECT(link_operation(endpoint_overflow, source, materializations));
  EXPECT(link_operation(signed_width, source, materializations));
  EXPECT(link_operation(unsigned_result, source, materializations));
  EXPECT(link_operation(unsigned_zero_result, source, materializations));
  EXPECT(link_operation(unsigned_endpoint, source, materializations));
  EXPECT(link_operation(unsigned_zero_divisor, source, materializations));
  EXPECT(link_operation(unsigned_width, source, materializations));

  auto positive_fold = selected(positive_result.fold());
  auto negative_fold = selected(negative_result.fold());
  auto opposite_fold = selected(opposite_sign.fold());
  auto signed_zero_fold = selected(signed_zero_result.fold());
  auto minimum_fold = selected(minimum_result.fold());
  auto unsigned_fold = selected(unsigned_result.fold());
  auto unsigned_zero_fold = selected(unsigned_zero_result.fold());
  auto unsigned_endpoint_fold = selected(unsigned_endpoint.fold());

  ASSERT(
      positive_fold && negative_fold && opposite_fold && signed_zero_fold &&
      minimum_fold && unsigned_fold && unsigned_zero_fold &&
      unsigned_endpoint_fold);
  EXPECT(value_is<Constants::Signed>(*positive_fold, Signed_64(3)));
  EXPECT(value_is<Constants::Signed>(*negative_fold, Signed_64(-3)));
  EXPECT(value_is<Constants::Signed>(*opposite_fold, Signed_64(-3)));
  EXPECT(value_is<Constants::Signed>(*signed_zero_fold, Signed_64(0)));
  EXPECT(value_is<Constants::Signed>(*minimum_fold, Signed_64(-128)));
  EXPECT(value_is<Constants::Unsigned>(*unsigned_fold, Unsigned_64(3)));
  EXPECT(value_is<Constants::Unsigned>(*unsigned_zero_fold, Unsigned_64(0)));
  EXPECT(
      value_is<Constants::Unsigned>(*unsigned_endpoint_fold, Unsigned_64(255)));
  EXPECT(&positive_fold->get_type() == &signed_type);
  EXPECT(&unsigned_endpoint_fold->get_type() == &unsigned_type);
  EXPECT(reports(
      signed_zero_divisor.fold(), Expression::Error::Type::DivisionByZero,
      signed_zero_divisor));
  EXPECT(reports(
      unsigned_zero_divisor.fold(), Expression::Error::Type::DivisionByZero,
      unsigned_zero_divisor));
  EXPECT(reports(
      endpoint_overflow.fold(), Expression::Error::Type::ArithmeticOverflow,
      endpoint_overflow));
  EXPECT(reports(
      signed_width.fold(), Expression::Error::Type::ArithmeticOverflow,
      signed_width));
  EXPECT(reports(
      unsigned_width.fold(), Expression::Error::Type::ArithmeticOverflow,
      unsigned_width));
}

PERIMORTEM_UNIT_TEST(LibraryDivide, ieee_domains) {
  Allocator::Arena domain;
  DivideMonograph source(domain);
  Materializations materializations(domain);
  Types::Real_32 real_32;
  Types::Real_64 real_64;
  auto& narrow_seven = Constants::Real::create_synthetic(domain, real_32, 7.0);
  auto& narrow_two = Constants::Real::create_synthetic(domain, real_32, 2.0);
  auto& narrow_zero = Constants::Real::create_synthetic(domain, real_32, 0.0);
  auto& narrow_negative_zero =
      Constants::Real::create_synthetic(domain, real_32, -0.0);
  auto& wide_seven = Constants::Real::create_synthetic(domain, real_64, 7.0);
  auto& wide_two = Constants::Real::create_synthetic(domain, real_64, 2.0);
  auto& wide_zero = Constants::Real::create_synthetic(domain, real_64, 0.0);
  auto& wide_negative_zero =
      Constants::Real::create_synthetic(domain, real_64, -0.0);
  auto& narrow_finite = Operations::Divide::create_synthetic(
      domain, materializations, narrow_seven, narrow_two);
  auto& narrow_signed_zero = Operations::Divide::create_synthetic(
      domain, materializations, narrow_negative_zero, narrow_two);
  auto& narrow_infinity = Operations::Divide::create_synthetic(
      domain, materializations, narrow_seven, narrow_negative_zero);
  auto& narrow_nan = Operations::Divide::create_synthetic(
      domain, materializations, narrow_zero, narrow_zero);
  auto& wide_finite = Operations::Divide::create_synthetic(
      domain, materializations, wide_seven, wide_two);
  auto& wide_signed_zero = Operations::Divide::create_synthetic(
      domain, materializations, wide_negative_zero, wide_two);
  auto& wide_infinity = Operations::Divide::create_synthetic(
      domain, materializations, wide_seven, wide_negative_zero);
  auto& wide_nan = Operations::Divide::create_synthetic(
      domain, materializations, wide_zero, wide_zero);

  EXPECT(narrow_finite.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(narrow_finite, source, materializations));
  EXPECT(link_operation(narrow_signed_zero, source, materializations));
  EXPECT(link_operation(narrow_infinity, source, materializations));
  EXPECT(link_operation(narrow_nan, source, materializations));
  EXPECT(link_operation(wide_finite, source, materializations));
  EXPECT(link_operation(wide_signed_zero, source, materializations));
  EXPECT(link_operation(wide_infinity, source, materializations));
  EXPECT(link_operation(wide_nan, source, materializations));

  auto narrow_value = selected(narrow_finite.fold());
  auto narrow_zero_value = selected(narrow_signed_zero.fold());
  auto narrow_infinite_value = selected(narrow_infinity.fold());
  auto narrow_nan_value = selected(narrow_nan.fold());
  auto wide_value = selected(wide_finite.fold());
  auto wide_zero_value = selected(wide_signed_zero.fold());
  auto wide_infinite_value = selected(wide_infinity.fold());
  auto wide_nan_value = selected(wide_nan.fold());

  ASSERT(
      narrow_value && narrow_zero_value && narrow_infinite_value &&
      narrow_nan_value && wide_value && wide_zero_value &&
      wide_infinite_value && wide_nan_value);
  auto narrow = get_value<Constants::Real, Real_64>(*narrow_value);
  auto narrow_zero_result =
      get_value<Constants::Real, Real_64>(*narrow_zero_value);
  auto narrow_infinite =
      get_value<Constants::Real, Real_64>(*narrow_infinite_value);
  auto narrow_nan_result =
      get_value<Constants::Real, Real_64>(*narrow_nan_value);
  auto wide = get_value<Constants::Real, Real_64>(*wide_value);
  auto wide_zero_result = get_value<Constants::Real, Real_64>(*wide_zero_value);
  auto wide_infinite =
      get_value<Constants::Real, Real_64>(*wide_infinite_value);
  auto wide_nan_result = get_value<Constants::Real, Real_64>(*wide_nan_value);

  ASSERT(
      narrow && narrow_zero_result && narrow_infinite && narrow_nan_result &&
      wide && wide_zero_result && wide_infinite && wide_nan_result);
  EXPECT(*narrow == Real_64(Real_32(7.0) / Real_32(2.0)));
  EXPECT(*wide == 3.5);
  EXPECT(*narrow_zero_result == 0.0 && __builtin_signbit(*narrow_zero_result));
  EXPECT(*wide_zero_result == 0.0 && __builtin_signbit(*wide_zero_result));
  EXPECT(
      __builtin_isinf(*narrow_infinite) && __builtin_signbit(*narrow_infinite));
  EXPECT(__builtin_isinf(*wide_infinite) && __builtin_signbit(*wide_infinite));
  EXPECT(__builtin_isnan(*narrow_nan_result));
  EXPECT(__builtin_isnan(*wide_nan_result));
  EXPECT(&narrow_value->get_type() == &real_32);
  EXPECT(&wide_value->get_type() == &real_64);
}

PERIMORTEM_UNIT_TEST(LibraryDivide, recursive_and_atomic) {
  Allocator::Arena domain;
  DivideMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 selected_type;
  auto& input = Constants::Unsigned::create_synthetic(domain, selected_type, 1);
  auto& folded =
      Constants::Unsigned::create_synthetic(domain, selected_type, 12);
  auto& divisor =
      Constants::Unsigned::create_synthetic(domain, selected_type, 3);
  DivideFoldInput child(domain, materializations, input, folded, selected_type);
  DivideFoldInput failing(
      domain, materializations, input, folded, selected_type, True);
  auto& divide = Operations::Divide::create_synthetic(
      domain, materializations, child, divisor);
  auto& failure = Operations::Divide::create_synthetic(
      domain, materializations, failing, divisor);

  EXPECT(divide.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(divide, source, materializations));
  EXPECT(link_operation(divide, source, materializations));
  EXPECT(link_operation(failure, source, materializations));

  auto first = selected(divide.fold());
  auto second = selected(divide.fold());

  ASSERT(first && second);
  EXPECT(&*first == &*second);
  EXPECT(value_is<Constants::Unsigned>(*first, Unsigned_64(4)));
  EXPECT(input_is(divide, 0, child));
  EXPECT(child.get_evaluations() == 1);
  EXPECT(reports(
      failure.fold(), Expression::Error::Type::InvalidConstant, failing));

  const auto& parser_type = Tetrodotoxin::Library::Dialect::get_unsigned_64();
  Errors success_errors;
  Tokenizer success_tokens(domain, "24 / 2"_view, "divide.ttx"_view);
  Cursor success_cursor(success_tokens, success_errors);
  Token success_left_token = success_cursor.consume();
  auto success_left_anchor =
      Anchor::create(success_left_token, Span(success_left_token));
  auto& success_left = Constants::Unsigned::create_authored(
      domain, parser_type, 24, success_left_anchor);
  auto parsed = Operations::Divide::parse(
      domain, materializations, success_cursor, Invalid::get_invalid(),
      success_left);
  Errors failure_errors;
  Tokenizer failure_tokens(domain, "24 / true"_view, "divide.ttx"_view);
  Cursor failure_cursor(failure_tokens, failure_errors);
  Token failure_left_token = failure_cursor.consume();
  auto failure_left_anchor =
      Anchor::create(failure_left_token, Span(failure_left_token));
  auto& failure_left = Constants::Unsigned::create_authored(
      domain, parser_type, 24, failure_left_anchor);
  auto rejected = Operations::Divide::parse(
      domain, materializations, failure_cursor, Invalid::get_invalid(),
      failure_left);

  ASSERT(parsed);
  EXPECT(parsed->is<Operations::Divide>());
  EXPECT(parsed->get_type().resolve().is<Invalid>());
  EXPECT(success_cursor.matches(Code::Type::Terminal));
  EXPECT(success_errors.is_empty());
  EXPECT(parsed->link(source, Invalid::get_invalid(), materializations));

  auto parsed_fold = parsed->visit<Operation>(
      [&](Operation& operation) { return selected(operation.fold()); },
      [](Abstract&) -> Option<Expression&> { return {}; });
  auto parsed_value =
      parsed_fold ? get_value<Constants::Unsigned, Unsigned_64>(*parsed_fold)
                  : Option<Unsigned_64>();

  ASSERT(parsed_value);
  EXPECT(*parsed_value == 12);
  EXPECT(&parsed->get_type() == &parser_type);

  ASSERT(rejected);
  EXPECT(rejected->is<Operations::Divide>());
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
