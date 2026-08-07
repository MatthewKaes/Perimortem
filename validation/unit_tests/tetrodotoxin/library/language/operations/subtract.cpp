// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/subtract.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/language/monograph.hpp"
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

class SubtractMonograph : public Tetrodotoxin::Language::Monograph {
 public:
  SubtractMonograph(Allocator::Arena& domain)
      : Tetrodotoxin::Language::Monograph(domain, Documentation::get_empty()) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "SubtractMonograph"_view;
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

static auto link_operation(
    Operation& operation,
    SubtractMonograph& source,
    Materializations& materializations) -> Bool {
  return operation.link(source, Invalid::get_invalid(), materializations);
}

class SubtractExpression : public Expression {
 public:
  SubtractExpression(View::Bytes name, const Abstract& type)
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
  SubtractMonograph source(domain);
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
  auto& wide_constant =
      Constants::Unsigned::create_synthetic(domain, unsigned_64, 12);
  auto& other_constant =
      Constants::Unsigned::create_synthetic(domain, unsigned_16, 12);
  auto& truth = Constants::True::create_synthetic(domain, boolean);
  auto& bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "x"_view);
  auto& exact = Operations::Subtract::create_synthetic(
      domain, materializations, left, same);
  auto& mixed_left = Operations::Subtract::create_synthetic(
      domain, materializations, wide_constant, left);
  auto& mismatch = Operations::Subtract::create_synthetic(
      domain, materializations, left, other);
  auto& mixed_constants = Operations::Subtract::create_synthetic(
      domain, materializations, wide_constant, other_constant);
  auto& signed_exact = Operations::Subtract::create_synthetic(
      domain, materializations, signed_left, signed_right);
  auto& real_exact = Operations::Subtract::create_synthetic(
      domain, materializations, real_left, real_right);
  auto& flags = Operations::Subtract::create_synthetic(
      domain, materializations, truth, truth);
  auto& byte_values = Operations::Subtract::create_synthetic(
      domain, materializations, bytes, bytes);
  auto& invalid = Operations::Subtract::create_synthetic(
      domain, materializations, unresolved, same);

  EXPECT(exact.get_type().resolve().is<Invalid>());
  EXPECT_NOT(exact.get_anchor());
  EXPECT(link_operation(exact, source, materializations));
  EXPECT(!link_operation(mixed_left, source, materializations));
  EXPECT(!link_operation(mismatch, source, materializations));
  EXPECT(!link_operation(mixed_constants, source, materializations));
  EXPECT(link_operation(signed_exact, source, materializations));
  EXPECT(link_operation(real_exact, source, materializations));
  EXPECT(!link_operation(flags, source, materializations));
  EXPECT(!link_operation(byte_values, source, materializations));
  EXPECT(!link_operation(invalid, source, materializations));

  auto exact_result = selected(exact.fold());

  EXPECT(&exact.get_type() == &unsigned_8);
  EXPECT(mixed_left.get_type().resolve().is<Invalid>());
  EXPECT(&signed_exact.get_type() == &signed_8);
  EXPECT(&real_exact.get_type() == &real_32);
  EXPECT_NOT(exact_result);
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
  SubtractMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 unsigned_type;
  Types::Signed_8 signed_type;
  auto& maximum =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 255);
  auto& one_unsigned =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 1);
  auto& zero_unsigned =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 0);
  auto& maximum_signed =
      Constants::Signed::create_synthetic(domain, signed_type, 127);
  auto& minimum_signed =
      Constants::Signed::create_synthetic(domain, signed_type, -128);
  auto& negative_one =
      Constants::Signed::create_synthetic(domain, signed_type, -1);
  auto& one_signed =
      Constants::Signed::create_synthetic(domain, signed_type, 1);
  auto& zero_signed =
      Constants::Signed::create_synthetic(domain, signed_type, 0);
  auto& negative_twelve =
      Constants::Signed::create_synthetic(domain, signed_type, -12);
  auto& negative_ten =
      Constants::Signed::create_synthetic(domain, signed_type, -10);
  auto& unsigned_success = Operations::Subtract::create_synthetic(
      domain, materializations, maximum, one_unsigned);
  auto& unsigned_underflow = Operations::Subtract::create_synthetic(
      domain, materializations, zero_unsigned, one_unsigned);
  auto& signed_difference = Operations::Subtract::create_synthetic(
      domain, materializations, negative_twelve, negative_ten);
  auto& upper_endpoint = Operations::Subtract::create_synthetic(
      domain, materializations, maximum_signed, zero_signed);
  auto& lower_endpoint = Operations::Subtract::create_synthetic(
      domain, materializations, minimum_signed, zero_signed);
  auto& signed_overflow = Operations::Subtract::create_synthetic(
      domain, materializations, maximum_signed, negative_one);
  auto& signed_underflow = Operations::Subtract::create_synthetic(
      domain, materializations, minimum_signed, one_signed);

  EXPECT(unsigned_success.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(unsigned_success, source, materializations));
  EXPECT(link_operation(unsigned_underflow, source, materializations));
  EXPECT(link_operation(signed_difference, source, materializations));
  EXPECT(link_operation(upper_endpoint, source, materializations));
  EXPECT(link_operation(lower_endpoint, source, materializations));
  EXPECT(link_operation(signed_overflow, source, materializations));
  EXPECT(link_operation(signed_underflow, source, materializations));

  auto unsigned_value = selected(unsigned_success.fold());
  auto signed_value = selected(signed_difference.fold());
  auto upper_value = selected(upper_endpoint.fold());
  auto lower_value = selected(lower_endpoint.fold());
  auto unsigned_error = unsigned_underflow.fold();
  auto overflow_error = signed_overflow.fold();
  auto underflow_error = signed_underflow.fold();
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
      unsigned_error, Expression::Error::Type::ArithmeticOverflow,
      unsigned_underflow));
  EXPECT(reports(
      overflow_error, Expression::Error::Type::ArithmeticOverflow,
      signed_overflow));
  EXPECT(reports(
      underflow_error, Expression::Error::Type::ArithmeticOverflow,
      signed_underflow));
}

PERIMORTEM_UNIT_TEST(LibrarySubtract, ieee_real_domains) {
  Allocator::Arena domain;
  SubtractMonograph source(domain);
  Materializations materializations(domain);
  Types::Real_32 real_32;
  Types::Real_64 real_64;
  auto& narrow_left =
      Constants::Real::create_synthetic(domain, real_32, Real_64(4.4));
  auto& narrow_right =
      Constants::Real::create_synthetic(domain, real_32, Real_64(1.1));
  auto& wide_left =
      Constants::Real::create_synthetic(domain, real_64, Real_64(-2.5));
  auto& wide_right =
      Constants::Real::create_synthetic(domain, real_64, Real_64(4.0));
  auto& infinity =
      Constants::Real::create_synthetic(domain, real_64, __builtin_inf());
  auto& negative_infinity =
      Constants::Real::create_synthetic(domain, real_64, -__builtin_inf());
  auto& nan =
      Constants::Real::create_synthetic(domain, real_64, __builtin_nan(""));
  auto& one = Constants::Real::create_synthetic(domain, real_64, Real_64(1.0));
  auto& narrow = Operations::Subtract::create_synthetic(
      domain, materializations, narrow_left, narrow_right);
  auto& wide = Operations::Subtract::create_synthetic(
      domain, materializations, wide_left, wide_right);
  auto& infinite = Operations::Subtract::create_synthetic(
      domain, materializations, infinity, negative_infinity);
  auto& unordered = Operations::Subtract::create_synthetic(
      domain, materializations, nan, one);

  EXPECT(narrow.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(narrow, source, materializations));
  EXPECT(link_operation(wide, source, materializations));
  EXPECT(link_operation(infinite, source, materializations));
  EXPECT(link_operation(unordered, source, materializations));

  auto narrow_value = selected(narrow.fold());
  auto wide_value = selected(wide.fold());
  auto infinite_value = selected(infinite.fold());
  auto unordered_value = selected(unordered.fold());
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
  SubtractMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 selected_type;
  auto& two = Constants::Unsigned::create_synthetic(domain, selected_type, 2);
  auto& twelve =
      Constants::Unsigned::create_synthetic(domain, selected_type, 12);
  auto& child = Operations::Multiply::create_synthetic(
      domain, materializations, two, two);
  auto& subtract = Operations::Subtract::create_synthetic(
      domain, materializations, twelve, child);

  EXPECT(subtract.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(subtract, source, materializations));
  EXPECT(link_operation(subtract, source, materializations));
  EXPECT(&subtract.get_type() == &selected_type);

  auto first = selected(subtract.fold());
  auto second = selected(subtract.fold());
  auto child_result = selected(child.fold());
  auto value = first ? get_unsigned(*first) : Option<Unsigned_64>();

  ASSERT(first && second && child_result);
  EXPECT(&*first == &*second);
  EXPECT(first->is<Constants::Unsigned>());
  EXPECT(&first->get_type() == &selected_type);
  EXPECT(value && *value == 8);
  EXPECT(input_is(subtract, 0, twelve));
  EXPECT(input_is(subtract, 1, child));
}
