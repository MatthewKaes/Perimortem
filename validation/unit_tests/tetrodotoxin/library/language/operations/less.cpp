// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/less.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/operations/multiply.hpp"
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

static Harness LibraryLess = {
  .name = "Tetrodotoxin::Library::Language::Operations::Less"_view,
};

class LessExpression : public Expression {
 public:
  LessExpression(View::Bytes name, const Abstract& type)
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

static auto input_is(
    const Operations::Less& less,
    Count index,
    const Expression& expected) -> Bool {
  return less.get_inputs().get_abstract(index).visit(
      []() { return False; },
      [&](const Abstract& expression) {
        return &expression == &expected ? True : False;
      });
}

PERIMORTEM_UNIT_TEST(LibraryLess, type_selection_and_partial) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Unsigned_8 unsigned_8;
  Types::Unsigned_16 unsigned_16;
  Types::Unsigned_64 unsigned_64;
  Types::Signed_8 signed_8;
  Types::Real_32 real_32;
  Types::Fixed bytes_type(
      "Fixed[Unsigned_8,1]"_view,
      Tetrodotoxin::Library::Dialect::get_unsigned_8(), 1);
  LessExpression left("left"_view, unsigned_8);
  LessExpression same("same"_view, unsigned_8);
  LessExpression other("other"_view, unsigned_16);
  LessExpression signed_left("signed"_view, signed_8);
  LessExpression signed_right("signed right"_view, signed_8);
  LessExpression real_left("real"_view, real_32);
  LessExpression real_right("real right"_view, real_32);
  LessExpression unresolved("unresolved"_view, Invalid::get_invalid());
  Constants::Unsigned wide_constant(unsigned_64, 12);
  Constants::Unsigned other_constant(unsigned_16, 12);
  Constants::True truth(Tetrodotoxin::Library::Dialect::get_bool());
  Constants::Bytes bytes(bytes_type, "x"_view);
  Operations::Less exact(domain, left, same);
  Operations::Less mixed_left(domain, wide_constant, left);
  Operations::Less mismatch(domain, left, other);
  Operations::Less mixed_constants(domain, wide_constant, other_constant);
  Operations::Less signed_exact(domain, signed_left, signed_right);
  Operations::Less real_exact(domain, real_left, real_right);
  Operations::Less flags(domain, truth, truth);
  Operations::Less byte_values(domain, bytes, bytes);
  Operations::Less invalid(domain, unresolved, same);
  auto exact_result = selected(exact.attempt_fold(domain, materializations));

  EXPECT(&exact.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(mixed_left.get_type().resolve().is<Invalid>());
  EXPECT(
      &signed_exact.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(&real_exact.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(exact_result && &*exact_result == &exact);
  EXPECT(mismatch.get_type().resolve().is<Invalid>());
  EXPECT(mixed_constants.get_type().resolve().is<Invalid>());
  EXPECT(flags.get_type().resolve().is<Invalid>());
  EXPECT(byte_values.get_type().resolve().is<Invalid>());
  EXPECT(invalid.get_type().resolve().is<Invalid>());
  EXPECT(input_is(exact, 0, left));
  EXPECT(input_is(exact, 1, same));
}

PERIMORTEM_UNIT_TEST(LibraryLess, integer_ordering) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Unsigned_8 unsigned_type;
  Types::Signed_8 signed_type;
  Constants::Unsigned zero(unsigned_type, 0);
  Constants::Unsigned one(unsigned_type, 1);
  Constants::Signed minimum(signed_type, -128);
  Constants::Signed maximum(signed_type, 127);
  Operations::Less unsigned_true(domain, zero, one);
  Operations::Less unsigned_false(domain, one, zero);
  Operations::Less signed_true(domain, minimum, maximum);
  Operations::Less signed_false(domain, maximum, minimum);
  auto unsigned_yes =
      selected(unsigned_true.attempt_fold(domain, materializations));
  auto unsigned_no =
      selected(unsigned_false.attempt_fold(domain, materializations));
  auto signed_yes =
      selected(signed_true.attempt_fold(domain, materializations));
  auto signed_no =
      selected(signed_false.attempt_fold(domain, materializations));

  ASSERT(unsigned_yes && unsigned_no && signed_yes && signed_no);
  EXPECT(unsigned_yes->is<Constants::True>());
  EXPECT(unsigned_no->is<Constants::False>());
  EXPECT(signed_yes->is<Constants::True>());
  EXPECT(signed_no->is<Constants::False>());
  EXPECT(
      &unsigned_yes->get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(&signed_no->get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
}

PERIMORTEM_UNIT_TEST(LibraryLess, ieee_ordering) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Real_32 real_32;
  Types::Real_64 real_64;
  Constants::Real narrow_left(real_32, Real_64(1.1));
  Constants::Real narrow_right(real_32, Real_64(2.2));
  Constants::Real finite(real_64, Real_64(4.0));
  Constants::Real infinity(real_64, __builtin_inf());
  Constants::Real nan(real_64, __builtin_nan(""));
  Operations::Less narrow(domain, narrow_left, narrow_right);
  Operations::Less infinite(domain, infinity, finite);
  Operations::Less unordered(domain, nan, finite);
  auto narrow_result = selected(narrow.attempt_fold(domain, materializations));
  auto infinite_result =
      selected(infinite.attempt_fold(domain, materializations));
  auto unordered_result =
      selected(unordered.attempt_fold(domain, materializations));

  ASSERT(narrow_result && infinite_result && unordered_result);
  EXPECT(narrow_result->is<Constants::True>());
  EXPECT(infinite_result->is<Constants::False>());
  EXPECT(unordered_result->is<Constants::False>());
}

PERIMORTEM_UNIT_TEST(LibraryLess, recursive_exact_is_idempotent) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Unsigned_8 selected_type;
  Constants::Unsigned two(selected_type, 2);
  Constants::Unsigned five(selected_type, 5);
  Operations::Multiply child(domain, two, two);
  Operations::Less less(domain, child, five);
  auto first = selected(less.attempt_fold(domain, materializations));
  auto second = selected(less.attempt_fold(domain, materializations));
  auto child_result = selected(child.attempt_fold(domain, materializations));

  ASSERT(first && second && child_result);
  EXPECT(&*first == &*second);
  EXPECT(first->is<Constants::True>());
  EXPECT(&first->get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(input_is(less, 0, *child_result));
  EXPECT(input_is(less, 1, five));
}
