// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/multiply.hpp"

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

class MultiplyMonograph : public Tetrodotoxin::Language::Monograph {
 public:
  MultiplyMonograph(Allocator::Arena& domain)
      : Tetrodotoxin::Language::Monograph(domain, Documentation::get_empty()) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "MultiplyMonograph"_view;
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

static auto link_operation(
    Operation& operation,
    MultiplyMonograph& source,
    Materializations& materializations) -> Bool {
  return operation.link(source, Invalid::get_invalid(), materializations);
}

class MultiplyExpression : public Expression {
 public:
  MultiplyExpression(View::Bytes name, const Abstract& type)
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

class MultiplyFoldInput : public Operation {
 public:
  MultiplyFoldInput(
      Allocator::Arena& domain,
      Materializations& materializations,
      Expression& input,
      Constant& result,
      const Ttx::Model::Type& type)
      : Operation(
            domain,
            materializations,
            Static::Vector<Reference<Expression>, 1>{{input}},
            {}),
        result(result),
        type(type) {}

  auto get_name() const -> View::Bytes override { return "Fold input"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_evaluations() const -> Count { return evaluations; }

 protected:
  auto evaluate_constants(Allocator::Arena&, Materializations&)
      -> Result<Option<Constant&>, Expression::Error> override {
    evaluations++;
    return result;
  }

  auto select_type(Materializations&) const
      -> Option<const Ttx::Model::Type&> override {
    return type;
  }

 private:
  Constant& result;
  const Ttx::Model::Type& type;
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
  MultiplyMonograph source(domain);
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
  auto& wide_constant =
      Constants::Unsigned::create_synthetic(domain, unsigned_64, 12);
  auto& other_constant =
      Constants::Unsigned::create_synthetic(domain, unsigned_16, 12);
  auto& truth = Constants::True::create_synthetic(domain, boolean);
  auto& bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "x"_view);
  auto& exact = Operations::Multiply::create_synthetic(
      domain, materializations, left, same);
  auto& mixed_left = Operations::Multiply::create_synthetic(
      domain, materializations, wide_constant, left);
  auto& mismatch = Operations::Multiply::create_synthetic(
      domain, materializations, left, other);
  auto& mixed_constants = Operations::Multiply::create_synthetic(
      domain, materializations, wide_constant, other_constant);
  auto& flags = Operations::Multiply::create_synthetic(
      domain, materializations, truth, truth);
  auto& byte_values = Operations::Multiply::create_synthetic(
      domain, materializations, bytes, bytes);
  auto& invalid = Operations::Multiply::create_synthetic(
      domain, materializations, unresolved, same);

  EXPECT(exact.get_type().resolve().is<Invalid>());
  EXPECT_NOT(exact.get_anchor());
  EXPECT(link_operation(exact, source, materializations));
  EXPECT(!link_operation(mixed_left, source, materializations));
  EXPECT(!link_operation(mismatch, source, materializations));
  EXPECT(!link_operation(mixed_constants, source, materializations));
  EXPECT(!link_operation(flags, source, materializations));
  EXPECT(!link_operation(byte_values, source, materializations));
  EXPECT(!link_operation(invalid, source, materializations));

  auto exact_result = selected(exact.fold());

  EXPECT(&exact.get_type() == &unsigned_8);
  EXPECT(mixed_left.get_type().resolve().is<Invalid>());
  EXPECT_NOT(exact_result);
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
  MultiplyMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 unsigned_type;
  Types::Signed_8 signed_type;
  auto& fifteen =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 15);
  auto& seventeen =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 17);
  auto& sixteen =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 16);
  auto& zero = Constants::Unsigned::create_synthetic(domain, unsigned_type, 0);
  auto& negative_twelve =
      Constants::Signed::create_synthetic(domain, signed_type, -12);
  auto& negative_ten =
      Constants::Signed::create_synthetic(domain, signed_type, -10);
  auto& minimum =
      Constants::Signed::create_synthetic(domain, signed_type, -128);
  auto& negative_one =
      Constants::Signed::create_synthetic(domain, signed_type, -1);
  auto& one = Constants::Signed::create_synthetic(domain, signed_type, 1);
  auto& unsigned_success = Operations::Multiply::create_synthetic(
      domain, materializations, fifteen, seventeen);
  auto& unsigned_overflow = Operations::Multiply::create_synthetic(
      domain, materializations, sixteen, sixteen);
  auto& zero_product = Operations::Multiply::create_synthetic(
      domain, materializations, zero, seventeen);
  auto& signed_success = Operations::Multiply::create_synthetic(
      domain, materializations, negative_twelve, negative_ten);
  auto& endpoint = Operations::Multiply::create_synthetic(
      domain, materializations, minimum, one);
  auto& signed_overflow = Operations::Multiply::create_synthetic(
      domain, materializations, minimum, negative_one);

  EXPECT(unsigned_success.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(unsigned_success, source, materializations));
  EXPECT(link_operation(unsigned_overflow, source, materializations));
  EXPECT(link_operation(zero_product, source, materializations));
  EXPECT(link_operation(signed_success, source, materializations));
  EXPECT(link_operation(endpoint, source, materializations));
  EXPECT(link_operation(signed_overflow, source, materializations));

  auto unsigned_value = selected(unsigned_success.fold());
  auto zero_value = selected(zero_product.fold());
  auto signed_value = selected(signed_success.fold());
  auto endpoint_value = selected(endpoint.fold());
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
      unsigned_overflow.fold(), Expression::Error::Type::ArithmeticOverflow,
      unsigned_overflow));
  EXPECT(reports(
      signed_overflow.fold(), Expression::Error::Type::ArithmeticOverflow,
      signed_overflow));
}

PERIMORTEM_UNIT_TEST(LibraryMultiply, ieee_real_domains) {
  Allocator::Arena domain;
  MultiplyMonograph source(domain);
  Materializations materializations(domain);
  Types::Real_32 real_32;
  Types::Real_64 real_64;
  auto& narrow_left =
      Constants::Real::create_synthetic(domain, real_32, Real_64(1.1));
  auto& narrow_right =
      Constants::Real::create_synthetic(domain, real_32, Real_64(3.0));
  auto& wide_left =
      Constants::Real::create_synthetic(domain, real_64, Real_64(-2.5));
  auto& wide_right =
      Constants::Real::create_synthetic(domain, real_64, Real_64(4.0));
  auto& infinity =
      Constants::Real::create_synthetic(domain, real_64, __builtin_inf());
  auto& nan =
      Constants::Real::create_synthetic(domain, real_64, __builtin_nan(""));
  auto& one = Constants::Real::create_synthetic(domain, real_64, Real_64(1.0));
  auto& narrow = Operations::Multiply::create_synthetic(
      domain, materializations, narrow_left, narrow_right);
  auto& wide = Operations::Multiply::create_synthetic(
      domain, materializations, wide_left, wide_right);
  auto& infinite = Operations::Multiply::create_synthetic(
      domain, materializations, infinity, one);
  auto& unordered = Operations::Multiply::create_synthetic(
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
  MultiplyMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 selected_type;
  auto& input = Constants::Unsigned::create_synthetic(domain, selected_type, 1);
  auto& folded =
      Constants::Unsigned::create_synthetic(domain, selected_type, 4);
  auto& factor =
      Constants::Unsigned::create_synthetic(domain, selected_type, 3);
  MultiplyFoldInput child(
      domain, materializations, input, folded, selected_type);
  auto& multiply = Operations::Multiply::create_synthetic(
      domain, materializations, factor, child);

  EXPECT(multiply.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(multiply, source, materializations));
  EXPECT(link_operation(multiply, source, materializations));
  EXPECT(&multiply.get_type() == &selected_type);

  auto first = selected(multiply.fold());
  auto second = selected(multiply.fold());
  auto value = first ? get_unsigned(*first) : Option<Unsigned_64>();

  ASSERT(first && second);
  EXPECT(&*first == &*second);
  EXPECT(first->is<Constants::Unsigned>());
  EXPECT(&first->get_type() == &selected_type);
  EXPECT(value && *value == 12);
  EXPECT(input_is(multiply, 0, factor));
  EXPECT(input_is(multiply, 1, child));
  EXPECT(child.get_evaluations() == 1);
}
