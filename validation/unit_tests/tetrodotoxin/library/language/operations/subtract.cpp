// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/subtract.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/operations/multiply.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/real_32.hpp"
#include "tetrodotoxin/library/language/types/real_64.hpp"
#include "tetrodotoxin/library/language/types/signed_8.hpp"
#include "tetrodotoxin/library/language/types/unsigned_16.hpp"
#include "tetrodotoxin/library/language/types/unsigned_64.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;

static Harness LibrarySubtract = {
  .name = "Tetrodotoxin::Library::Language::Operations::Subtract"_view,
};

class SubtractExpression : public Expression {
 public:
  SubtractExpression(View::Bytes name, const Abstract& type)
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

static auto get_unsigned(const Expression& expression) -> Option<Unsigned_64> {
  return expression.visit<Constants::Unsigned>(
      [](const Constants::Unsigned& selected) -> Option<Unsigned_64> {
        return selected.get_value();
      },
      [](const Abstract&) -> Option<Unsigned_64> { return {}; });
}

static auto get_signed(const Expression& expression) -> Option<Signed_64> {
  return expression.visit<Constants::Signed>(
      [](const Constants::Signed& selected) -> Option<Signed_64> {
        return selected.get_value();
      },
      [](const Abstract&) -> Option<Signed_64> { return {}; });
}

static auto get_real(const Expression& expression) -> Option<Real_64> {
  return expression.visit<Constants::Real>(
      [](const Constants::Real& selected) -> Option<Real_64> {
        return selected.get_value();
      },
      [](const Abstract&) -> Option<Real_64> { return {}; });
}

static auto input_is(
    const Operations::Subtract& subtract,
    Count index,
    const Expression& expected) -> Bool {
  return subtract.get_inputs().get_abstract(index).visit(
      []() { return False; },
      [&](const Abstract& expression) {
        return &expression == &expected ? True : False;
      });
}

PERIMORTEM_UNIT_TEST(LibrarySubtract, type_selection_and_partial) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Unsigned_8 unsigned_8;
  Types::Unsigned_16 unsigned_16;
  Types::Unsigned_64 unsigned_64;
  Types::Signed_8 signed_8;
  Types::Real_32 real_32;
  Types::Boolean boolean;
  Types::Fixed bytes_type(
      "Fixed[Unsigned_8,1]"_view,
      Tetrodotoxin::Library::Dialect::get_unsigned_8(), 1);
  SubtractExpression left("left"_view, unsigned_8);
  SubtractExpression same("same"_view, unsigned_8);
  SubtractExpression other("other"_view, unsigned_16);
  SubtractExpression signed_left("signed"_view, signed_8);
  SubtractExpression signed_right("signed right"_view, signed_8);
  SubtractExpression real_left("real"_view, real_32);
  SubtractExpression real_right("real right"_view, real_32);
  SubtractExpression unresolved("unresolved"_view, Invalid::get_invalid());
  Constants::Unsigned wide_constant(unsigned_64, 12);
  Constants::Unsigned other_constant(unsigned_16, 12);
  Constants::True truth(boolean);
  Constants::Bytes bytes(bytes_type, "x"_view);
  Operations::Subtract exact(domain, left, same);
  Operations::Subtract mixed_left(domain, wide_constant, left);
  Operations::Subtract mismatch(domain, left, other);
  Operations::Subtract mixed_constants(domain, wide_constant, other_constant);
  Operations::Subtract signed_exact(domain, signed_left, signed_right);
  Operations::Subtract real_exact(domain, real_left, real_right);
  Operations::Subtract flags(domain, truth, truth);
  Operations::Subtract byte_values(domain, bytes, bytes);
  Operations::Subtract invalid(domain, unresolved, same);
  auto exact_result = selected(exact.attempt_fold(domain, materializations));

  EXPECT(&exact.get_type() == &unsigned_8);
  EXPECT(mixed_left.get_type().resolve().is<Invalid>());
  EXPECT(&signed_exact.get_type() == &signed_8);
  EXPECT(&real_exact.get_type() == &real_32);
  EXPECT(exact_result && &*exact_result == &exact);
  EXPECT(mismatch.get_type().resolve().is<Invalid>());
  EXPECT(mixed_constants.get_type().resolve().is<Invalid>());
  EXPECT(flags.get_type().resolve().is<Invalid>());
  EXPECT(byte_values.get_type().resolve().is<Invalid>());
  EXPECT(invalid.get_type().resolve().is<Invalid>());
  EXPECT(input_is(exact, 0, left));
  EXPECT(input_is(exact, 1, same));
}

PERIMORTEM_UNIT_TEST(LibrarySubtract, checked_integer_widths) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Unsigned_8 unsigned_type;
  Types::Signed_8 signed_type;
  Constants::Unsigned maximum(unsigned_type, 255);
  Constants::Unsigned one_unsigned(unsigned_type, 1);
  Constants::Unsigned zero_unsigned(unsigned_type, 0);
  Constants::Signed maximum_signed(signed_type, 127);
  Constants::Signed minimum_signed(signed_type, -128);
  Constants::Signed negative_one(signed_type, -1);
  Constants::Signed one_signed(signed_type, 1);
  Constants::Signed zero_signed(signed_type, 0);
  Constants::Signed negative_twelve(signed_type, -12);
  Constants::Signed negative_ten(signed_type, -10);
  Operations::Subtract unsigned_success(domain, maximum, one_unsigned);
  Operations::Subtract unsigned_underflow(domain, zero_unsigned, one_unsigned);
  Operations::Subtract signed_difference(domain, negative_twelve, negative_ten);
  Operations::Subtract upper_endpoint(domain, maximum_signed, zero_signed);
  Operations::Subtract lower_endpoint(domain, minimum_signed, zero_signed);
  Operations::Subtract signed_overflow(domain, maximum_signed, negative_one);
  Operations::Subtract signed_underflow(domain, minimum_signed, one_signed);
  auto unsigned_value =
      selected(unsigned_success.attempt_fold(domain, materializations));
  auto signed_value =
      selected(signed_difference.attempt_fold(domain, materializations));
  auto upper_value =
      selected(upper_endpoint.attempt_fold(domain, materializations));
  auto lower_value =
      selected(lower_endpoint.attempt_fold(domain, materializations));
  auto unsigned_error =
      unsigned_underflow.attempt_fold(domain, materializations);
  auto overflow_error = signed_overflow.attempt_fold(domain, materializations);
  auto underflow_error =
      signed_underflow.attempt_fold(domain, materializations);
  auto unsigned_number =
      unsigned_value ? get_unsigned(*unsigned_value) : Option<Unsigned_64>();
  auto signed_number =
      signed_value ? get_signed(*signed_value) : Option<Signed_64>();
  auto upper_number =
      upper_value ? get_signed(*upper_value) : Option<Signed_64>();
  auto lower_number =
      lower_value ? get_signed(*lower_value) : Option<Signed_64>();

  ASSERT(unsigned_value && signed_value && upper_value && lower_value);
  EXPECT(unsigned_number && *unsigned_number == 254);
  EXPECT(&unsigned_value->get_type() == &unsigned_type);
  EXPECT(signed_number && *signed_number == -2);
  EXPECT(upper_number && *upper_number == 127);
  EXPECT(lower_number && *lower_number == -128);
  EXPECT(reports(
      unsigned_error, FoldError::Type::ArithmeticOverflow, unsigned_underflow));
  EXPECT(reports(
      overflow_error, FoldError::Type::ArithmeticOverflow, signed_overflow));
  EXPECT(reports(
      underflow_error, FoldError::Type::ArithmeticOverflow, signed_underflow));
}

PERIMORTEM_UNIT_TEST(LibrarySubtract, ieee_real_domains) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Real_32 real_32;
  Types::Real_64 real_64;
  Constants::Real narrow_left(real_32, Real_64(4.4));
  Constants::Real narrow_right(real_32, Real_64(1.1));
  Constants::Real wide_left(real_64, Real_64(-2.5));
  Constants::Real wide_right(real_64, Real_64(4.0));
  Constants::Real infinity(real_64, __builtin_inf());
  Constants::Real negative_infinity(real_64, -__builtin_inf());
  Constants::Real nan(real_64, __builtin_nan(""));
  Constants::Real one(real_64, Real_64(1.0));
  Operations::Subtract narrow(domain, narrow_left, narrow_right);
  Operations::Subtract wide(domain, wide_left, wide_right);
  Operations::Subtract infinite(domain, infinity, negative_infinity);
  Operations::Subtract unordered(domain, nan, one);
  auto narrow_value = selected(narrow.attempt_fold(domain, materializations));
  auto wide_value = selected(wide.attempt_fold(domain, materializations));
  auto infinite_value =
      selected(infinite.attempt_fold(domain, materializations));
  auto unordered_value =
      selected(unordered.attempt_fold(domain, materializations));
  auto narrow_number =
      narrow_value ? get_real(*narrow_value) : Option<Real_64>();
  auto wide_number = wide_value ? get_real(*wide_value) : Option<Real_64>();
  auto infinite_number =
      infinite_value ? get_real(*infinite_value) : Option<Real_64>();
  auto unordered_number =
      unordered_value ? get_real(*unordered_value) : Option<Real_64>();

  ASSERT(narrow_value && wide_value && infinite_value && unordered_value);
  EXPECT(
      narrow_number && *narrow_number == Real_64(Real_32(4.4) - Real_32(1.1)));
  EXPECT(&narrow_value->get_type() == &real_32);
  EXPECT(wide_number && *wide_number == Real_64(-6.5));
  EXPECT(infinite_number && __builtin_isinf(*infinite_number));
  EXPECT(unordered_number && __builtin_isnan(*unordered_number));
}

PERIMORTEM_UNIT_TEST(LibrarySubtract, recursive_exact_is_idempotent) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Unsigned_8 selected_type;
  Constants::Unsigned two(selected_type, 2);
  Constants::Unsigned twelve(selected_type, 12);
  Operations::Multiply child(domain, two, two);
  Operations::Subtract subtract(domain, twelve, child);
  auto first = selected(subtract.attempt_fold(domain, materializations));
  auto second = selected(subtract.attempt_fold(domain, materializations));
  auto child_result = selected(child.attempt_fold(domain, materializations));
  auto value = first ? get_unsigned(*first) : Option<Unsigned_64>();

  ASSERT(first && second && child_result);
  EXPECT(&*first == &*second);
  EXPECT(first->is<Constants::Unsigned>());
  EXPECT(&first->get_type() == &selected_type);
  EXPECT(value && *value == 8);
  EXPECT(input_is(subtract, 0, twelve));
  EXPECT(input_is(subtract, 1, *child_result));
}
