// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/divide.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

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

class DivideExpression : public Expression {
 public:
  DivideExpression(View::Bytes name, const Abstract& type)
      : name(name), type(type) {}

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
      const Expression& input,
      const Expression& result,
      const Ttx::Model::Type& type,
      Bool fails = False)
      : Operation(domain, Static::Vector<Reference<Expression>, 1>{{input}}),
        result(result),
        type(type),
        fails(fails) {}

  auto get_name() const -> View::Bytes override { return "Fold input"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Ttx::Model::Type& override { return type; }
  auto get_evaluations() const -> Count { return evaluations; }

 protected:
  auto evaluate_constants(Allocator::Arena&, Materializations&) const
      -> Result<const Expression&, FoldError> override {
    evaluations++;
    if (fails) {
      return FoldError(FoldError::Type::InvalidConstant, *this);
    }

    return result;
  }

 private:
  const Expression& result;
  const Ttx::Model::Type& type;
  Bool fails;
  mutable Count evaluations = 0;
};

static auto selected(const Result<const Expression&, FoldError>& result)
    -> Option<const Expression&> {
  return result.visit(
      [](const Expression& expression) -> Option<const Expression&> {
        return expression;
      },
      [](const FoldError&) -> Option<const Expression&> { return {}; });
}

static auto reports(
    const Result<const Expression&, FoldError>& result,
    FoldError::Type expected,
    const Expression& origin) -> Bool {
  return result.visit(
      [](const Expression&) { return False; },
      [&](const FoldError& error) {
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

static auto matches_token(const Cursor& cursor, Token expected) -> Bool {
  Token current = cursor.current();
  return current.get_offset() == expected.get_offset() &&
         current.get_code() == expected.get_code();
}

static auto span_width(View::Bytes rendered) -> Count {
  Count caret = Algorithm::search(rendered, "^"_view);
  if (caret == Count(-1)) {
    return 0;
  }

  Count width = 1;
  while (caret + width < rendered.get_size() &&
         rendered[caret + width] == '-') {
    width++;
  }

  return width;
}

PERIMORTEM_UNIT_TEST(LibraryDivide, type_selection) {
  Allocator::Arena domain;
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
  Constants::True truth(boolean);
  Constants::Bytes bytes(bytes_type, "x"_view);
  Operations::Divide signed_exact(domain, signed_left, signed_right);
  Operations::Divide unsigned_exact(domain, unsigned_left, unsigned_right);
  Operations::Divide real_exact(domain, real_left, real_right);
  Operations::Divide mismatch(domain, unsigned_left, other);
  Operations::Divide unresolved_pair(domain, unresolved, unresolved);
  Operations::Divide invalid_pair(domain, invalid, invalid);
  Operations::Divide flags(domain, truth, truth);
  Operations::Divide byte_values(domain, bytes, bytes);
  auto retained =
      selected(unsigned_exact.attempt_fold(domain, materializations));

  EXPECT(&signed_exact.get_type() == &signed_8);
  EXPECT(&unsigned_exact.get_type() == &unsigned_8);
  EXPECT(&real_exact.get_type() == &real_32);
  EXPECT(retained && &*retained == &unsigned_exact);
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
  Materializations materializations(domain);
  Types::Signed_8 signed_type;
  Types::Unsigned_8 unsigned_type;
  Constants::Signed positive(signed_type, 7);
  Constants::Signed negative(signed_type, -7);
  Constants::Signed two(signed_type, 2);
  Constants::Signed negative_two(signed_type, -2);
  Constants::Signed signed_zero(signed_type, 0);
  Constants::Signed signed_one(signed_type, 1);
  Constants::Signed negative_one(signed_type, -1);
  Constants::Signed minimum(signed_type, -128);
  Constants::Signed invalid_signed(signed_type, 128);
  Constants::Unsigned seven(unsigned_type, 7);
  Constants::Unsigned unsigned_two(unsigned_type, 2);
  Constants::Unsigned unsigned_zero(unsigned_type, 0);
  Constants::Unsigned unsigned_one(unsigned_type, 1);
  Constants::Unsigned maximum(unsigned_type, 255);
  Constants::Unsigned invalid_unsigned(unsigned_type, 256);
  Operations::Divide positive_result(domain, positive, two);
  Operations::Divide negative_result(domain, negative, two);
  Operations::Divide opposite_sign(domain, positive, negative_two);
  Operations::Divide signed_zero_result(domain, signed_zero, signed_one);
  Operations::Divide minimum_result(domain, minimum, signed_one);
  Operations::Divide signed_zero_divisor(domain, positive, signed_zero);
  Operations::Divide endpoint_overflow(domain, minimum, negative_one);
  Operations::Divide signed_width(domain, invalid_signed, signed_one);
  Operations::Divide unsigned_result(domain, seven, unsigned_two);
  Operations::Divide unsigned_zero_result(domain, unsigned_zero, unsigned_one);
  Operations::Divide unsigned_endpoint(domain, maximum, unsigned_one);
  Operations::Divide unsigned_zero_divisor(domain, maximum, unsigned_zero);
  Operations::Divide unsigned_width(domain, invalid_unsigned, unsigned_one);
  auto positive_fold =
      selected(positive_result.attempt_fold(domain, materializations));
  auto negative_fold =
      selected(negative_result.attempt_fold(domain, materializations));
  auto opposite_fold =
      selected(opposite_sign.attempt_fold(domain, materializations));
  auto signed_zero_fold =
      selected(signed_zero_result.attempt_fold(domain, materializations));
  auto minimum_fold =
      selected(minimum_result.attempt_fold(domain, materializations));
  auto unsigned_fold =
      selected(unsigned_result.attempt_fold(domain, materializations));
  auto unsigned_zero_fold =
      selected(unsigned_zero_result.attempt_fold(domain, materializations));
  auto unsigned_endpoint_fold =
      selected(unsigned_endpoint.attempt_fold(domain, materializations));

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
      signed_zero_divisor.attempt_fold(domain, materializations),
      FoldError::Type::DivisionByZero, signed_zero_divisor));
  EXPECT(reports(
      unsigned_zero_divisor.attempt_fold(domain, materializations),
      FoldError::Type::DivisionByZero, unsigned_zero_divisor));
  EXPECT(reports(
      endpoint_overflow.attempt_fold(domain, materializations),
      FoldError::Type::ArithmeticOverflow, endpoint_overflow));
  EXPECT(reports(
      signed_width.attempt_fold(domain, materializations),
      FoldError::Type::ArithmeticOverflow, signed_width));
  EXPECT(reports(
      unsigned_width.attempt_fold(domain, materializations),
      FoldError::Type::ArithmeticOverflow, unsigned_width));
}

PERIMORTEM_UNIT_TEST(LibraryDivide, ieee_domains) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Real_32 real_32;
  Types::Real_64 real_64;
  Constants::Real narrow_seven(real_32, 7.0);
  Constants::Real narrow_two(real_32, 2.0);
  Constants::Real narrow_zero(real_32, 0.0);
  Constants::Real narrow_negative_zero(real_32, -0.0);
  Constants::Real wide_seven(real_64, 7.0);
  Constants::Real wide_two(real_64, 2.0);
  Constants::Real wide_zero(real_64, 0.0);
  Constants::Real wide_negative_zero(real_64, -0.0);
  Operations::Divide narrow_finite(domain, narrow_seven, narrow_two);
  Operations::Divide narrow_signed_zero(
      domain, narrow_negative_zero, narrow_two);
  Operations::Divide narrow_infinity(
      domain, narrow_seven, narrow_negative_zero);
  Operations::Divide narrow_nan(domain, narrow_zero, narrow_zero);
  Operations::Divide wide_finite(domain, wide_seven, wide_two);
  Operations::Divide wide_signed_zero(domain, wide_negative_zero, wide_two);
  Operations::Divide wide_infinity(domain, wide_seven, wide_negative_zero);
  Operations::Divide wide_nan(domain, wide_zero, wide_zero);
  auto narrow_value =
      selected(narrow_finite.attempt_fold(domain, materializations));
  auto narrow_zero_value =
      selected(narrow_signed_zero.attempt_fold(domain, materializations));
  auto narrow_infinite_value =
      selected(narrow_infinity.attempt_fold(domain, materializations));
  auto narrow_nan_value =
      selected(narrow_nan.attempt_fold(domain, materializations));
  auto wide_value =
      selected(wide_finite.attempt_fold(domain, materializations));
  auto wide_zero_value =
      selected(wide_signed_zero.attempt_fold(domain, materializations));
  auto wide_infinite_value =
      selected(wide_infinity.attempt_fold(domain, materializations));
  auto wide_nan_value =
      selected(wide_nan.attempt_fold(domain, materializations));

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
  Allocator::Arena rendering;
  Materializations materializations(domain);
  Types::Unsigned_8 selected_type;
  Constants::Unsigned input(selected_type, 1);
  Constants::Unsigned folded(selected_type, 12);
  Constants::Unsigned divisor(selected_type, 3);
  DivideFoldInput child(domain, input, folded, selected_type);
  DivideFoldInput failing(domain, input, folded, selected_type, True);
  Operations::Divide divide(domain, child, divisor);
  Operations::Divide failure(domain, failing, divisor);
  auto first = selected(divide.attempt_fold(domain, materializations));
  auto second = selected(divide.attempt_fold(domain, materializations));

  ASSERT(first && second);
  EXPECT(&*first == &*second);
  EXPECT(value_is<Constants::Unsigned>(*first, Unsigned_64(4)));
  EXPECT(input_is(divide, 0, folded));
  EXPECT(child.get_evaluations() == 1);
  EXPECT(reports(
      failure.attempt_fold(domain, materializations),
      FoldError::Type::InvalidConstant, failing));

  const auto& parser_type = Tetrodotoxin::Library::Dialect::get_unsigned_64();
  Constants::Unsigned left(parser_type, 24);
  Errors success_errors;
  Tokenizer success_tokens(domain, "/ 2"_view, "divide.ttx"_view);
  Cursor success_cursor(success_tokens, success_errors);
  auto parsed = Operations::Divide::parse(
      domain, materializations, success_cursor, Invalid::get_invalid(), left);
  auto parsed_value = parsed
                          ? get_value<Constants::Unsigned, Unsigned_64>(*parsed)
                          : Option<Unsigned_64>();
  Errors failure_errors;
  Tokenizer failure_tokens(domain, "/ true"_view, "divide.ttx"_view);
  Cursor failure_cursor(failure_tokens, failure_errors);
  Token opening = failure_cursor.current();
  auto rejected = Operations::Divide::parse(
      domain, materializations, failure_cursor, Invalid::get_invalid(), left);

  ASSERT(parsed && parsed_value);
  EXPECT(*parsed_value == 12);
  EXPECT(success_cursor.matches(Code::Type::Terminal));
  EXPECT(success_errors.is_empty());
  EXPECT_NOT(rejected);
  EXPECT(matches_token(failure_cursor, opening));
  ASSERT(failure_errors.get_size() == 1);
  View::Bytes rendered = failure_errors.render_message(rendering, 0);

  EXPECT(span_width(rendered) == Count(6));
}
