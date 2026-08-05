// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/multiply.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

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
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;

static Harness LibraryMultiply = {
  .name = "Tetrodotoxin::Library::Language::Operations::Multiply"_view,
};

class MultiplyExpression : public Expression {
 public:
  MultiplyExpression(View::Bytes name, const Abstract& type)
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

class MultiplyFoldInput : public Operation {
 public:
  MultiplyFoldInput(
      Allocator::Arena& domain,
      const Expression& input,
      const Expression& result,
      const Ttx::Model::Type& type)
      : Operation(domain, Static::Vector<Reference<Expression>, 1>{{input}}),
        result(result),
        type(type) {}

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
    return result;
  }

 private:
  const Expression& result;
  const Ttx::Model::Type& type;
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
    const Operations::Multiply& multiply,
    Count index,
    const Expression& expected) -> Bool {
  return multiply.get_inputs().get_abstract(index).visit(
      []() { return False; },
      [&](const Abstract& expression) {
        return &expression == &expected ? True : False;
      });
}

PERIMORTEM_UNIT_TEST(LibraryMultiply, type_selection) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Unsigned_8 unsigned_8;
  Types::Unsigned_16 unsigned_16;
  Types::Unsigned_64 unsigned_64;
  Types::Boolean boolean;
  Types::Fixed bytes_type(
      "Fixed[Unsigned_8,1]"_view,
      Tetrodotoxin::Library::Dialect::get_unsigned_8(), 1);
  MultiplyExpression left("left"_view, unsigned_8);
  MultiplyExpression same("same"_view, unsigned_8);
  MultiplyExpression other("other"_view, unsigned_16);
  MultiplyExpression unresolved("unresolved"_view, Invalid::get_invalid());
  Constants::Unsigned wide_constant(unsigned_64, 12);
  Constants::Unsigned other_constant(unsigned_16, 12);
  Constants::True truth(boolean);
  Constants::Bytes bytes(bytes_type, "x"_view);
  Operations::Multiply exact(domain, left, same);
  Operations::Multiply mixed_left(domain, wide_constant, left);
  Operations::Multiply mismatch(domain, left, other);
  Operations::Multiply mixed_constants(domain, wide_constant, other_constant);
  Operations::Multiply flags(domain, truth, truth);
  Operations::Multiply byte_values(domain, bytes, bytes);
  Operations::Multiply invalid(domain, unresolved, same);
  auto exact_result = selected(exact.attempt_fold(domain, materializations));

  EXPECT(&exact.get_type() == &unsigned_8);
  EXPECT(mixed_left.get_type().resolve().is<Invalid>());
  EXPECT(exact_result && &*exact_result == &exact);
  EXPECT(mismatch.get_type().resolve().is<Invalid>());
  EXPECT(mixed_constants.get_type().resolve().is<Invalid>());
  EXPECT(flags.get_type().resolve().is<Invalid>());
  EXPECT(byte_values.get_type().resolve().is<Invalid>());
  EXPECT(invalid.get_type().resolve().is<Invalid>());
  EXPECT(input_is(exact, 0, left));
  EXPECT(input_is(exact, 1, same));
}

PERIMORTEM_UNIT_TEST(LibraryMultiply, checked_integer_widths) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Unsigned_8 unsigned_type;
  Types::Signed_8 signed_type;
  Constants::Unsigned fifteen(unsigned_type, 15);
  Constants::Unsigned seventeen(unsigned_type, 17);
  Constants::Unsigned sixteen(unsigned_type, 16);
  Constants::Unsigned zero(unsigned_type, 0);
  Constants::Signed negative_twelve(signed_type, -12);
  Constants::Signed negative_ten(signed_type, -10);
  Constants::Signed minimum(signed_type, -128);
  Constants::Signed negative_one(signed_type, -1);
  Constants::Signed one(signed_type, 1);
  Operations::Multiply unsigned_success(domain, fifteen, seventeen);
  Operations::Multiply unsigned_overflow(domain, sixteen, sixteen);
  Operations::Multiply zero_product(domain, zero, seventeen);
  Operations::Multiply signed_success(domain, negative_twelve, negative_ten);
  Operations::Multiply endpoint(domain, minimum, one);
  Operations::Multiply signed_overflow(domain, minimum, negative_one);

  auto unsigned_value =
      selected(unsigned_success.attempt_fold(domain, materializations));
  auto zero_value =
      selected(zero_product.attempt_fold(domain, materializations));
  auto signed_value =
      selected(signed_success.attempt_fold(domain, materializations));
  auto endpoint_value =
      selected(endpoint.attempt_fold(domain, materializations));
  auto unsigned_number =
      unsigned_value ? get_unsigned(*unsigned_value) : Option<Unsigned_64>();
  auto zero_number =
      zero_value ? get_unsigned(*zero_value) : Option<Unsigned_64>();
  auto signed_number =
      signed_value ? get_signed(*signed_value) : Option<Signed_64>();
  auto endpoint_number =
      endpoint_value ? get_signed(*endpoint_value) : Option<Signed_64>();

  ASSERT(unsigned_value && zero_value && signed_value && endpoint_value);
  EXPECT(&unsigned_value->get_type() == &unsigned_type);
  EXPECT(unsigned_number && *unsigned_number == 255);
  EXPECT(zero_number && *zero_number == 0);
  EXPECT(&signed_value->get_type() == &signed_type);
  EXPECT(signed_number && *signed_number == 120);
  EXPECT(endpoint_number && *endpoint_number == -128);
  EXPECT(reports(
      unsigned_overflow.attempt_fold(domain, materializations),
      FoldError::Type::ArithmeticOverflow, unsigned_overflow));
  EXPECT(reports(
      signed_overflow.attempt_fold(domain, materializations),
      FoldError::Type::ArithmeticOverflow, signed_overflow));
}

PERIMORTEM_UNIT_TEST(LibraryMultiply, ieee_real_domains) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Real_32 real_32;
  Types::Real_64 real_64;
  Constants::Real narrow_left(real_32, Real_64(1.1));
  Constants::Real narrow_right(real_32, Real_64(3.0));
  Constants::Real wide_left(real_64, Real_64(-2.5));
  Constants::Real wide_right(real_64, Real_64(4.0));
  Constants::Real infinity(real_64, __builtin_inf());
  Constants::Real nan(real_64, __builtin_nan(""));
  Constants::Real one(real_64, Real_64(1.0));
  Operations::Multiply narrow(domain, narrow_left, narrow_right);
  Operations::Multiply wide(domain, wide_left, wide_right);
  Operations::Multiply infinite(domain, infinity, one);
  Operations::Multiply unordered(domain, nan, one);

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
  EXPECT(&narrow_value->get_type() == &real_32);
  EXPECT(
      narrow_number && *narrow_number == Real_64(Real_32(1.1) * Real_32(3.0)));
  EXPECT(&wide_value->get_type() == &real_64);
  EXPECT(wide_number && *wide_number == Real_64(-10.0));
  EXPECT(infinite_number && __builtin_isinf(*infinite_number));
  EXPECT(unordered_number && __builtin_isnan(*unordered_number));
}

PERIMORTEM_UNIT_TEST(LibraryMultiply, recursive_exact_is_idempotent) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Unsigned_8 selected_type;
  Constants::Unsigned input(selected_type, 1);
  Constants::Unsigned folded(selected_type, 4);
  Constants::Unsigned factor(selected_type, 3);
  MultiplyFoldInput child(domain, input, folded, selected_type);
  Operations::Multiply multiply(domain, factor, child);

  auto first = selected(multiply.attempt_fold(domain, materializations));
  auto second = selected(multiply.attempt_fold(domain, materializations));
  auto value = first ? get_unsigned(*first) : Option<Unsigned_64>();

  ASSERT(first && second);
  EXPECT(&*first == &*second);
  EXPECT(first->is<Constants::Unsigned>());
  EXPECT(&first->get_type() == &selected_type);
  EXPECT(value && *value == 12);
  EXPECT(input_is(multiply, 0, factor));
  EXPECT(input_is(multiply, 1, folded));
  EXPECT(child.get_evaluations() == 1);
}
