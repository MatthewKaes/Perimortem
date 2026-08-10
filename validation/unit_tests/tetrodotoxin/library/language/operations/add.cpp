// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/add.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
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

static Harness LibraryAdd = {
  .name = "Tetrodotoxin::Library::Language::Operations::Add"_view,
};

class AddMonograph : public Tetrodotoxin::Language::Monograph {
 public:
  AddMonograph(Allocator::Arena& domain)
      : Tetrodotoxin::Language::Monograph(domain, Documentation::get_empty()) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "AddMonograph"_view;
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

class AddExpression : public Expression {
 public:
  AddExpression(View::Bytes name, const Abstract& type)
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

static auto link_operation(
    Operation& operation,
    AddMonograph& source,
    Materializations& materializations) -> Bool {
  return operation.link(source, Invalid::get_invalid(), materializations);
}

static auto selected(
    const Result<Option<Expression&>, Expression::Error>& result)
    -> Option<Expression&> {
  return result.visit(
      [](const Option<Expression&>& folded) -> Option<Expression&> {
        return folded;
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
      [](const Constants::Unsigned& value) -> Option<Unsigned_64> {
        return value.get_value();
      },
      [](const Abstract&) -> Option<Unsigned_64> { return {}; });
}

static auto get_signed(const Expression& expression) -> Option<Signed_64> {
  return expression.visit<Constants::Signed>(
      [](const Constants::Signed& value) -> Option<Signed_64> {
        return value.get_value();
      },
      [](const Abstract&) -> Option<Signed_64> { return {}; });
}

static auto get_real(const Expression& expression) -> Option<Real_64> {
  return expression.visit<Constants::Real>(
      [](const Constants::Real& value) -> Option<Real_64> {
        return value.get_value();
      },
      [](const Abstract&) -> Option<Real_64> { return {}; });
}

static auto input_is(
    const Operations::Add& add,
    Count index,
    const Expression& expected) -> Bool {
  return add.get_inputs().get_abstract(index).visit(
      []() { return False; },
      [&](const Abstract& expression) {
        return &expression == &expected ? True : False;
      });
}

PERIMORTEM_UNIT_TEST(LibraryAdd, exact_type_selection_and_partial) {
  Allocator::Arena domain;
  AddMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 unsigned_8;
  Types::Unsigned_16 unsigned_16;
  Types::Signed_8 signed_8;
  Types::Real_32 real_32;
  AddExpression unsigned_left("unsigned left"_view, unsigned_8);
  AddExpression unsigned_right("unsigned right"_view, unsigned_8);
  AddExpression other_width("other width"_view, unsigned_16);
  AddExpression signed_left("signed left"_view, signed_8);
  AddExpression signed_right("signed right"_view, signed_8);
  AddExpression real_left("real left"_view, real_32);
  AddExpression real_right("real right"_view, real_32);
  auto& unsigned_add = Operations::Add::create_synthetic(
      domain, materializations, unsigned_left, unsigned_right);
  auto& signed_add = Operations::Add::create_synthetic(
      domain, materializations, signed_left, signed_right);
  auto& real_add = Operations::Add::create_synthetic(
      domain, materializations, real_left, real_right);
  auto& mismatched = Operations::Add::create_synthetic(
      domain, materializations, unsigned_left, other_width);

  EXPECT(link_operation(unsigned_add, source, materializations));
  EXPECT(link_operation(signed_add, source, materializations));
  EXPECT(link_operation(real_add, source, materializations));
  EXPECT_NOT(link_operation(mismatched, source, materializations));
  EXPECT(&unsigned_add.get_type() == &unsigned_8);
  EXPECT(&signed_add.get_type() == &signed_8);
  EXPECT(&real_add.get_type() == &real_32);
  EXPECT_NOT(selected(unsigned_add.fold()));
  EXPECT(input_is(unsigned_add, 0, unsigned_left));
  EXPECT(input_is(unsigned_add, 1, unsigned_right));
}

PERIMORTEM_UNIT_TEST(LibraryAdd, integer_width_and_host_overflow) {
  Allocator::Arena domain;
  AddMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 unsigned_8;
  Types::Unsigned_64 unsigned_64;
  Types::Signed_8 signed_8;
  auto& unsigned_max =
      Constants::Unsigned::create_synthetic(domain, unsigned_8, 255);
  auto& unsigned_one =
      Constants::Unsigned::create_synthetic(domain, unsigned_8, 1);
  auto& host_max = Constants::Unsigned::create_synthetic(
      domain, unsigned_64, Unsigned_64(-1));
  auto& host_one =
      Constants::Unsigned::create_synthetic(domain, unsigned_64, 1);
  auto& signed_max = Constants::Signed::create_synthetic(domain, signed_8, 127);
  auto& signed_min =
      Constants::Signed::create_synthetic(domain, signed_8, -128);
  auto& signed_one = Constants::Signed::create_synthetic(domain, signed_8, 1);
  auto& signed_negative_one =
      Constants::Signed::create_synthetic(domain, signed_8, -1);
  auto& width_overflow = Operations::Add::create_synthetic(
      domain, materializations, unsigned_max, unsigned_one);
  auto& host_overflow = Operations::Add::create_synthetic(
      domain, materializations, host_max, host_one);
  auto& signed_overflow = Operations::Add::create_synthetic(
      domain, materializations, signed_max, signed_one);
  auto& signed_underflow = Operations::Add::create_synthetic(
      domain, materializations, signed_min, signed_negative_one);

  ASSERT(link_operation(width_overflow, source, materializations));
  ASSERT(link_operation(host_overflow, source, materializations));
  ASSERT(link_operation(signed_overflow, source, materializations));
  ASSERT(link_operation(signed_underflow, source, materializations));
  EXPECT(reports(
      width_overflow.fold(), Expression::Error::Type::ArithmeticOverflow,
      width_overflow));
  EXPECT(reports(
      host_overflow.fold(), Expression::Error::Type::ArithmeticOverflow,
      host_overflow));
  EXPECT(reports(
      signed_overflow.fold(), Expression::Error::Type::ArithmeticOverflow,
      signed_overflow));
  EXPECT(reports(
      signed_underflow.fold(), Expression::Error::Type::ArithmeticOverflow,
      signed_underflow));
}

PERIMORTEM_UNIT_TEST(LibraryAdd, integer_results_retain_type) {
  Allocator::Arena domain;
  AddMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 unsigned_type;
  Types::Signed_8 signed_type;
  auto& unsigned_left =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 12);
  auto& unsigned_right =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 30);
  auto& signed_left =
      Constants::Signed::create_synthetic(domain, signed_type, -5);
  auto& signed_right =
      Constants::Signed::create_synthetic(domain, signed_type, 2);
  auto& unsigned_add = Operations::Add::create_synthetic(
      domain, materializations, unsigned_left, unsigned_right);
  auto& signed_add = Operations::Add::create_synthetic(
      domain, materializations, signed_left, signed_right);

  ASSERT(link_operation(unsigned_add, source, materializations));
  ASSERT(link_operation(signed_add, source, materializations));
  auto unsigned_result = selected(unsigned_add.fold());
  auto signed_result = selected(signed_add.fold());
  ASSERT(unsigned_result && signed_result);
  EXPECT(&unsigned_result->get_type() == &unsigned_type);
  EXPECT(&signed_result->get_type() == &signed_type);
  EXPECT(get_unsigned(*unsigned_result) == Option<Unsigned_64>(42));
  EXPECT(get_signed(*signed_result) == Option<Signed_64>(-3));
}

PERIMORTEM_UNIT_TEST(LibraryAdd, ieee_real_domains) {
  Allocator::Arena domain;
  AddMonograph source(domain);
  Materializations materializations(domain);
  Types::Real_32 real_32;
  Types::Real_64 real_64;
  auto& narrow_left =
      Constants::Real::create_synthetic(domain, real_32, Real_64(1.1));
  auto& narrow_right =
      Constants::Real::create_synthetic(domain, real_32, Real_64(2.2));
  auto& infinity =
      Constants::Real::create_synthetic(domain, real_64, __builtin_inf());
  auto& one = Constants::Real::create_synthetic(domain, real_64, Real_64(1));
  auto& nan =
      Constants::Real::create_synthetic(domain, real_64, __builtin_nan(""));
  auto& narrow = Operations::Add::create_synthetic(
      domain, materializations, narrow_left, narrow_right);
  auto& infinite = Operations::Add::create_synthetic(
      domain, materializations, infinity, one);
  auto& unordered =
      Operations::Add::create_synthetic(domain, materializations, nan, one);

  ASSERT(link_operation(narrow, source, materializations));
  ASSERT(link_operation(infinite, source, materializations));
  ASSERT(link_operation(unordered, source, materializations));
  auto narrow_result = selected(narrow.fold());
  auto infinite_result = selected(infinite.fold());
  auto unordered_result = selected(unordered.fold());
  ASSERT(narrow_result && infinite_result && unordered_result);
  auto narrow_value = get_real(*narrow_result);
  auto infinite_value = get_real(*infinite_result);
  auto unordered_value = get_real(*unordered_result);
  EXPECT(narrow_value && *narrow_value == Real_64(Real_32(1.1) + Real_32(2.2)));
  EXPECT(&narrow_result->get_type() == &real_32);
  EXPECT(infinite_value && __builtin_isinf(*infinite_value));
  EXPECT(unordered_value && __builtin_isnan(*unordered_value));
}

PERIMORTEM_UNIT_TEST(LibraryAdd, recursive_fold_is_idempotent) {
  Allocator::Arena domain;
  AddMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 type;
  auto& one = Constants::Unsigned::create_synthetic(domain, type, 1);
  auto& two = Constants::Unsigned::create_synthetic(domain, type, 2);
  auto& three = Constants::Unsigned::create_synthetic(domain, type, 3);
  auto& child =
      Operations::Add::create_synthetic(domain, materializations, one, two);
  auto& root =
      Operations::Add::create_synthetic(domain, materializations, child, three);

  ASSERT(link_operation(root, source, materializations));
  auto first = selected(root.fold());
  auto second = selected(root.fold());
  auto child_result = selected(child.fold());
  ASSERT(first && second && child_result);
  EXPECT(&*first == &*second);
  EXPECT(get_unsigned(*first) == Option<Unsigned_64>(6));
  EXPECT(input_is(root, 0, child));
  EXPECT(input_is(root, 1, three));
}

PERIMORTEM_UNIT_TEST(LibraryAdd, recursive_error_keeps_child_origin) {
  Allocator::Arena domain;
  AddMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 type;
  auto& maximum = Constants::Unsigned::create_synthetic(domain, type, 255);
  auto& one = Constants::Unsigned::create_synthetic(domain, type, 1);
  auto& child =
      Operations::Add::create_synthetic(domain, materializations, maximum, one);
  auto& root =
      Operations::Add::create_synthetic(domain, materializations, child, one);

  ASSERT(link_operation(root, source, materializations));
  EXPECT(
      reports(root.fold(), Expression::Error::Type::ArithmeticOverflow, child));
}

PERIMORTEM_UNIT_TEST(LibraryAdd, invalid_constant_keeps_operand_origin) {
  Allocator::Arena domain;
  AddMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 type;
  auto& wrong = Constants::Bytes::create_synthetic(domain, type, {});
  auto& valid = Constants::Unsigned::create_synthetic(domain, type, 1);
  auto& add =
      Operations::Add::create_synthetic(domain, materializations, wrong, valid);

  ASSERT(link_operation(add, source, materializations));
  EXPECT(reports(add.fold(), Expression::Error::Type::InvalidConstant, wrong));
}

PERIMORTEM_UNIT_TEST(LibraryAdd, rejects_nonnumeric_and_mixed_domains) {
  Allocator::Arena domain;
  AddMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 unsigned_8;
  Types::Signed_8 signed_8;
  Types::Real_32 real_32;
  Types::Fixed bytes_type(
      "Fixed[Unsigned_8,1]"_view,
      Tetrodotoxin::Library::Dialect::get_unsigned_8(), 1);
  auto& truth = Constants::True::create_synthetic(
      domain, Tetrodotoxin::Library::Dialect::get_bool());
  auto& bytes = Constants::Bytes::create_synthetic(domain, bytes_type, {});
  auto& unsigned_value =
      Constants::Unsigned::create_synthetic(domain, unsigned_8, 1);
  auto& signed_value = Constants::Signed::create_synthetic(domain, signed_8, 1);
  auto& real_value = Constants::Real::create_synthetic(domain, real_32, 1.0);
  auto& bool_add =
      Operations::Add::create_synthetic(domain, materializations, truth, truth);
  auto& bytes_add =
      Operations::Add::create_synthetic(domain, materializations, bytes, bytes);
  auto& mixed_integer = Operations::Add::create_synthetic(
      domain, materializations, unsigned_value, signed_value);
  auto& mixed_domain = Operations::Add::create_synthetic(
      domain, materializations, unsigned_value, real_value);

  EXPECT_NOT(link_operation(bool_add, source, materializations));
  EXPECT_NOT(link_operation(bytes_add, source, materializations));
  EXPECT_NOT(link_operation(mixed_integer, source, materializations));
  EXPECT_NOT(link_operation(mixed_domain, source, materializations));
}
