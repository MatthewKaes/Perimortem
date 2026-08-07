// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/less.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/language/monograph.hpp"
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

class LessMonograph : public Tetrodotoxin::Language::Monograph {
 public:
  LessMonograph(Allocator::Arena& domain)
      : Tetrodotoxin::Language::Monograph(domain, Documentation::get_empty()) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "LessMonograph"_view;
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

static auto link_operation(
    Operation& operation,
    LessMonograph& source,
    Materializations& materializations) -> Bool {
  return operation.link(source, Invalid::get_invalid(), materializations);
}

class LessExpression : public Expression {
 public:
  LessExpression(View::Bytes name, const Abstract& type)
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
  LessMonograph source(domain);
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
  auto& wide_constant =
      Constants::Unsigned::create_synthetic(domain, unsigned_64, 12);
  auto& other_constant =
      Constants::Unsigned::create_synthetic(domain, unsigned_16, 12);
  auto& truth = Constants::True::create_synthetic(
      domain, Tetrodotoxin::Library::Dialect::get_bool());
  auto& bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "x"_view);
  auto& exact =
      Operations::Less::create_synthetic(domain, materializations, left, same);
  auto& mixed_left = Operations::Less::create_synthetic(
      domain, materializations, wide_constant, left);
  auto& mismatch =
      Operations::Less::create_synthetic(domain, materializations, left, other);
  auto& mixed_constants = Operations::Less::create_synthetic(
      domain, materializations, wide_constant, other_constant);
  auto& signed_exact = Operations::Less::create_synthetic(
      domain, materializations, signed_left, signed_right);
  auto& real_exact = Operations::Less::create_synthetic(
      domain, materializations, real_left, real_right);
  auto& flags = Operations::Less::create_synthetic(
      domain, materializations, truth, truth);
  auto& byte_values = Operations::Less::create_synthetic(
      domain, materializations, bytes, bytes);
  auto& invalid = Operations::Less::create_synthetic(
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

  EXPECT(&exact.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(mixed_left.get_type().resolve().is<Invalid>());
  EXPECT(
      &signed_exact.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(&real_exact.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT_NOT(exact_result);
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
  LessMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 unsigned_type;
  Types::Signed_8 signed_type;
  auto& zero = Constants::Unsigned::create_synthetic(domain, unsigned_type, 0);
  auto& one = Constants::Unsigned::create_synthetic(domain, unsigned_type, 1);
  auto& minimum =
      Constants::Signed::create_synthetic(domain, signed_type, -128);
  auto& maximum = Constants::Signed::create_synthetic(domain, signed_type, 127);
  auto& unsigned_true =
      Operations::Less::create_synthetic(domain, materializations, zero, one);
  auto& unsigned_false =
      Operations::Less::create_synthetic(domain, materializations, one, zero);
  auto& signed_true = Operations::Less::create_synthetic(
      domain, materializations, minimum, maximum);
  auto& signed_false = Operations::Less::create_synthetic(
      domain, materializations, maximum, minimum);

  EXPECT(unsigned_true.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(unsigned_true, source, materializations));
  EXPECT(link_operation(unsigned_false, source, materializations));
  EXPECT(link_operation(signed_true, source, materializations));
  EXPECT(link_operation(signed_false, source, materializations));

  auto unsigned_yes = selected(unsigned_true.fold());
  auto unsigned_no = selected(unsigned_false.fold());
  auto signed_yes = selected(signed_true.fold());
  auto signed_no = selected(signed_false.fold());

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
  LessMonograph source(domain);
  Materializations materializations(domain);
  Types::Real_32 real_32;
  Types::Real_64 real_64;
  auto& narrow_left =
      Constants::Real::create_synthetic(domain, real_32, Real_64(1.1));
  auto& narrow_right =
      Constants::Real::create_synthetic(domain, real_32, Real_64(2.2));
  auto& finite =
      Constants::Real::create_synthetic(domain, real_64, Real_64(4.0));
  auto& infinity =
      Constants::Real::create_synthetic(domain, real_64, __builtin_inf());
  auto& nan =
      Constants::Real::create_synthetic(domain, real_64, __builtin_nan(""));
  auto& narrow = Operations::Less::create_synthetic(
      domain, materializations, narrow_left, narrow_right);
  auto& infinite = Operations::Less::create_synthetic(
      domain, materializations, infinity, finite);
  auto& unordered =
      Operations::Less::create_synthetic(domain, materializations, nan, finite);

  EXPECT(narrow.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(narrow, source, materializations));
  EXPECT(link_operation(infinite, source, materializations));
  EXPECT(link_operation(unordered, source, materializations));

  auto narrow_result = selected(narrow.fold());
  auto infinite_result = selected(infinite.fold());
  auto unordered_result = selected(unordered.fold());

  ASSERT(narrow_result && infinite_result && unordered_result);
  EXPECT(narrow_result->is<Constants::True>());
  EXPECT(infinite_result->is<Constants::False>());
  EXPECT(unordered_result->is<Constants::False>());
}

PERIMORTEM_UNIT_TEST(LibraryLess, recursive_exact_is_idempotent) {
  Allocator::Arena domain;
  LessMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 selected_type;
  auto& two = Constants::Unsigned::create_synthetic(domain, selected_type, 2);
  auto& five = Constants::Unsigned::create_synthetic(domain, selected_type, 5);
  auto& child = Operations::Multiply::create_synthetic(
      domain, materializations, two, two);
  auto& less =
      Operations::Less::create_synthetic(domain, materializations, child, five);

  EXPECT(less.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(less, source, materializations));
  EXPECT(link_operation(less, source, materializations));
  EXPECT(&less.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());

  auto first = selected(less.fold());
  auto second = selected(less.fold());
  auto child_result = selected(child.fold());

  ASSERT(first && second && child_result);
  EXPECT(&*first == &*second);
  EXPECT(first->is<Constants::True>());
  EXPECT(&first->get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(input_is(less, 0, child));
  EXPECT(input_is(less, 1, five));
}
