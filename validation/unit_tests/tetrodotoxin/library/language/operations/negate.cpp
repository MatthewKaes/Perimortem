// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/negate.hpp"

#include "validation/unit_test.hpp"

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
#include "tetrodotoxin/library/language/types/signed_64.hpp"
#include "tetrodotoxin/library/language/types/signed_8.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;

static Harness LibraryNegate = {
  .name = "Tetrodotoxin::Library::Language::Operations::Negate"_view,
};

class NegateExpression : public Expression {
 public:
  NegateExpression(View::Bytes name, const Abstract& type)
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
    const Operations::Negate& negate,
    const Expression& expected) -> Bool {
  return negate.get_inputs().get_abstract(0).visit(
      []() { return False; },
      [&](const Abstract& expression) {
        return &expression == &expected ? True : False;
      });
}

PERIMORTEM_UNIT_TEST(LibraryNegate, type_selection_and_partial) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Signed_8 signed_8;
  Types::Real_32 real_32;
  Types::Unsigned_8 unsigned_8;
  Types::Boolean boolean;
  Types::Fixed bytes_type(
      "Fixed[Unsigned_8,1]"_view,
      Tetrodotoxin::Library::Dialect::get_unsigned_8(), 1);
  NegateExpression signed_value("signed"_view, signed_8);
  NegateExpression real_value("real"_view, real_32);
  NegateExpression unsigned_value("unsigned"_view, unsigned_8);
  NegateExpression unresolved("unresolved"_view, Invalid::get_invalid());
  Constants::True truth(boolean);
  Constants::Bytes bytes(bytes_type, "x"_view);
  Operations::Negate signed_negate(domain, signed_value);
  Operations::Negate real_negate(domain, real_value);
  Operations::Negate unsigned_negate(domain, unsigned_value);
  Operations::Negate flag_negate(domain, truth);
  Operations::Negate bytes_negate(domain, bytes);
  Operations::Negate invalid_negate(domain, unresolved);
  auto signed_result =
      selected(signed_negate.attempt_fold(domain, materializations));
  auto real_result =
      selected(real_negate.attempt_fold(domain, materializations));

  ASSERT(signed_result && real_result);
  EXPECT(&signed_negate.get_type() == &signed_8);
  EXPECT(&real_negate.get_type() == &real_32);
  EXPECT(&*signed_result == &signed_negate);
  EXPECT(&*real_result == &real_negate);
  EXPECT(unsigned_negate.get_type().resolve().is<Invalid>());
  EXPECT(flag_negate.get_type().resolve().is<Invalid>());
  EXPECT(bytes_negate.get_type().resolve().is<Invalid>());
  EXPECT(invalid_negate.get_type().resolve().is<Invalid>());
  EXPECT(input_is(signed_negate, signed_value));
  EXPECT(signed_negate.get_inputs().get_size() == 1);
}

PERIMORTEM_UNIT_TEST(LibraryNegate, checked_signed_widths) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Signed_8 signed_8;
  Types::Signed_64 signed_64;
  Constants::Signed positive(signed_8, 127);
  Constants::Signed negative(signed_8, -127);
  Constants::Signed zero(signed_8, 0);
  Constants::Signed minimum(signed_8, -128);
  Constants::Signed wide_minimum(
      signed_64, Signed_64(-9223372036854775807) - 1);
  Operations::Negate positive_negate(domain, positive);
  Operations::Negate negative_negate(domain, negative);
  Operations::Negate zero_negate(domain, zero);
  Operations::Negate minimum_negate(domain, minimum);
  Operations::Negate wide_minimum_negate(domain, wide_minimum);
  auto positive_result =
      selected(positive_negate.attempt_fold(domain, materializations));
  auto negative_result =
      selected(negative_negate.attempt_fold(domain, materializations));
  auto zero_result =
      selected(zero_negate.attempt_fold(domain, materializations));
  auto minimum_result = minimum_negate.attempt_fold(domain, materializations);
  auto wide_minimum_result =
      wide_minimum_negate.attempt_fold(domain, materializations);
  auto positive_value =
      positive_result ? get_signed(*positive_result) : Option<Signed_64>();
  auto negative_value =
      negative_result ? get_signed(*negative_result) : Option<Signed_64>();
  auto zero_value =
      zero_result ? get_signed(*zero_result) : Option<Signed_64>();

  ASSERT(positive_result && negative_result && zero_result);
  EXPECT(positive_value && *positive_value == -127);
  EXPECT(negative_value && *negative_value == 127);
  EXPECT(zero_value && *zero_value == 0);
  EXPECT(&positive_result->get_type() == &signed_8);
  EXPECT(reports(
      minimum_result, FoldError::Type::ArithmeticOverflow, minimum_negate));
  EXPECT(reports(
      wide_minimum_result, FoldError::Type::ArithmeticOverflow,
      wide_minimum_negate));
}

PERIMORTEM_UNIT_TEST(LibraryNegate, ieee_real_domains) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Real_32 real_32;
  Types::Real_64 real_64;
  Constants::Real finite_32(real_32, 3.25);
  Constants::Real finite_64(real_64, -9.5);
  Constants::Real infinity(real_64, __builtin_inf());
  Constants::Real nan(real_64, __builtin_nan(""));
  Constants::Real positive_zero(real_32, 0.0);
  Constants::Real negative_zero(real_64, -0.0);
  Operations::Negate finite_32_negate(domain, finite_32);
  Operations::Negate finite_64_negate(domain, finite_64);
  Operations::Negate infinity_negate(domain, infinity);
  Operations::Negate nan_negate(domain, nan);
  Operations::Negate positive_zero_negate(domain, positive_zero);
  Operations::Negate negative_zero_negate(domain, negative_zero);
  auto finite_32_result =
      selected(finite_32_negate.attempt_fold(domain, materializations));
  auto finite_64_result =
      selected(finite_64_negate.attempt_fold(domain, materializations));
  auto infinity_result =
      selected(infinity_negate.attempt_fold(domain, materializations));
  auto nan_result = selected(nan_negate.attempt_fold(domain, materializations));
  auto positive_zero_result =
      selected(positive_zero_negate.attempt_fold(domain, materializations));
  auto negative_zero_result =
      selected(negative_zero_negate.attempt_fold(domain, materializations));
  auto finite_32_value =
      finite_32_result ? get_real(*finite_32_result) : Option<Real_64>();
  auto finite_64_value =
      finite_64_result ? get_real(*finite_64_result) : Option<Real_64>();
  auto infinity_value =
      infinity_result ? get_real(*infinity_result) : Option<Real_64>();
  auto nan_value = nan_result ? get_real(*nan_result) : Option<Real_64>();
  auto positive_zero_value = positive_zero_result
                                 ? get_real(*positive_zero_result)
                                 : Option<Real_64>();
  auto negative_zero_value = negative_zero_result
                                 ? get_real(*negative_zero_result)
                                 : Option<Real_64>();

  ASSERT(
      finite_32_value && finite_64_value && infinity_value && nan_value &&
      positive_zero_value && negative_zero_value);
  EXPECT(*finite_32_value == -3.25);
  EXPECT(*finite_64_value == 9.5);
  EXPECT(__builtin_isinf(*infinity_value) && *infinity_value < 0.0);
  EXPECT(__builtin_isnan(*nan_value));
  EXPECT(
      *positive_zero_value == 0.0 && __builtin_signbit(*positive_zero_value));
  EXPECT(
      *negative_zero_value == 0.0 && !__builtin_signbit(*negative_zero_value));
  EXPECT(&finite_32_result->get_type() == &real_32);
  EXPECT(&finite_64_result->get_type() == &real_64);
}

PERIMORTEM_UNIT_TEST(LibraryNegate, recursive_fold_and_provenance) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Signed_8 signed_8;
  Constants::Signed one(signed_8, 1);
  Constants::Signed minimum(signed_8, -128);
  Operations::Negate child(domain, one);
  Operations::Negate parent(domain, child);
  Operations::Negate failing_child(domain, minimum);
  Operations::Negate failing_parent(domain, failing_child);
  auto parent_result = selected(parent.attempt_fold(domain, materializations));
  auto repeated_result =
      selected(parent.attempt_fold(domain, materializations));
  auto failing_result = failing_parent.attempt_fold(domain, materializations);
  auto parent_value =
      parent_result ? get_signed(*parent_result) : Option<Signed_64>();

  ASSERT(parent_result && repeated_result && parent_value);
  EXPECT(*parent_value == 1);
  EXPECT(&*parent_result == &*repeated_result);
  EXPECT(reports(
      failing_result, FoldError::Type::ArithmeticOverflow, failing_child));
}
