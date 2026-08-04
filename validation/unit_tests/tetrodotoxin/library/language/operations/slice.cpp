// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/slice.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/option.hpp"
#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/types/access.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/signed_64.hpp"
#include "tetrodotoxin/library/language/types/unsigned_64.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;

static Harness LibrarySlice = {
  .name = "Tetrodotoxin::Library::Language::Operations::Slice"_view,
};

class SliceExpression : public Expression {
 public:
  SliceExpression(View::Bytes name, const Ttx::Model::Type& type)
      : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Ttx::Model::Type& override { return type; }
  auto get_inputs() const -> const Layout& override { return inputs; }

 private:
  View::Bytes name;
  const Ttx::Model::Type& type;
  Ttx::Model::Layouts::Fluid inputs;
};

class SliceFoldOperation : public Operation {
 public:
  SliceFoldOperation(
      Allocator::Arena& domain,
      const Expression& input,
      const Expression& result,
      const Ttx::Model::Type& type)
      : Operation(domain, Static::Vector<Reference<Expression>, 1>{{input}}),
        result(result),
        type(type) {}

  auto get_name() const -> View::Bytes override { return "Fold size"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Ttx::Model::Type& override { return type; }

 protected:
  auto evaluate_constants(Allocator::Arena&, Materializations&) const
      -> Result<const Expression&, FoldError> override {
    return result;
  }

 private:
  const Expression& result;
  const Ttx::Model::Type& type;
};

static auto selects(
    const Result<const Expression&, FoldError>& result,
    const Expression& expected) -> Bool {
  return result.visit(
      [&](const Expression& selected) {
        return &selected == &expected ? True : False;
      },
      [](FoldError) { return False; });
}

static auto reports(
    const Result<const Expression&, FoldError>& result,
    FoldError expected) -> Bool {
  return result.visit(
      [](const Expression&) { return False; },
      [&](FoldError selected) { return selected == expected ? True : False; });
}

static auto input_is(
    const Operations::Slice& slice,
    Count index,
    const Expression& expected) -> Bool {
  return slice.get_inputs().get_abstract(index).visit(
      []() { return False; },
      [&](const Abstract& selected) {
        return &selected == &expected ? True : False;
      });
}

static auto selected(const Result<const Expression&, FoldError>& result)
    -> Option<const Expression&> {
  return result.visit(
      [](const Expression& expression) -> Option<const Expression&> {
        return expression;
      },
      [](FoldError) -> Option<const Expression&> { return {}; });
}

PERIMORTEM_UNIT_TEST(LibrarySlice, receiver_type_selection) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  const auto& element = Tetrodotoxin::Library::Dialect::get_unsigned_8();
  Types::Signed_64 integer;
  Types::Fixed fixed("Fixed[Unsigned_8,0]"_view, element, 0);
  Types::View view("View[Unsigned_8]"_view, element);
  Types::Access access("Access[Unsigned_8]"_view, element);
  SliceExpression fixed_receiver("fixed"_view, fixed);
  SliceExpression view_receiver("view"_view, view);
  SliceExpression access_receiver("access"_view, access);
  SliceExpression index("index"_view, integer);
  Operations::Slice fixed_index(
      domain, materializations, fixed_receiver, index);
  Operations::Slice view_index(domain, materializations, view_receiver, index);
  Operations::Slice access_index(
      domain, materializations, access_receiver, index);

  EXPECT(&fixed_index.get_type() == &element);
  EXPECT(&view_index.get_type() == &element);
  EXPECT(&access_index.get_type() == &element);
  EXPECT(
      selects(fixed_index.attempt_fold(domain, materializations), fixed_index));
  EXPECT(
      selects(view_index.attempt_fold(domain, materializations), view_index));
  EXPECT(selects(
      access_index.attempt_fold(domain, materializations), access_index));
  ASSERT_EQ(fixed_index.get_inputs().get_size(), Count(2));
  EXPECT(input_is(fixed_index, 0, fixed_receiver));
  EXPECT(input_is(fixed_index, 1, index));
}

PERIMORTEM_UNIT_TEST(LibrarySlice, range_type_selection) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Unsigned_8 element;
  Types::Signed_64 integer;
  Types::Unsigned_64 unsigned_integer;
  Types::Fixed fixed("Fixed[Unsigned_8,8]"_view, element, 8);
  Types::View view("View[Unsigned_8]"_view, element);
  Types::Access access("Access[Unsigned_8]"_view, element);
  SliceExpression fixed_receiver("fixed"_view, fixed);
  SliceExpression view_receiver("view"_view, view);
  SliceExpression access_receiver("access"_view, access);
  SliceExpression start("start"_view, integer);
  SliceExpression dynamic_size("size"_view, integer);
  Constants::Unsigned fold_input(unsigned_integer, 1);
  Constants::Unsigned fixed_size(unsigned_integer, 4);
  SliceFoldOperation size_operation(
      domain, fold_input, fixed_size, unsigned_integer);
  Operations::Slice fixed_dynamic(
      domain, materializations, fixed_receiver, start, dynamic_size);
  Operations::Slice view_dynamic(
      domain, materializations, view_receiver, start, dynamic_size);
  Operations::Slice access_dynamic(
      domain, materializations, access_receiver, start, dynamic_size);
  Operations::Slice constant_size(
      domain, materializations, fixed_receiver, start, fixed_size);
  Operations::Slice folded_size(
      domain, materializations, fixed_receiver, start, size_operation);

  ASSERT(fixed_dynamic.get_type().is<Types::View>());
  ASSERT(view_dynamic.get_type().is<Types::View>());
  ASSERT(access_dynamic.get_type().is<Types::Access>());
  ASSERT(constant_size.get_type().is<Types::Fixed>());
  ASSERT(folded_size.get_type().is<Types::View>());
  EXPECT(
      selects(folded_size.attempt_fold(domain, materializations), folded_size));
  ASSERT(folded_size.get_type().is<Types::Fixed>());
  const auto& fixed_result =
      static_cast<const Types::Fixed&>(constant_size.get_type());
  EXPECT(&fixed_result.get_element_type() == &element);
  EXPECT(fixed_result.get_extent() == 4);
  EXPECT(input_is(folded_size, 2, fixed_size));
  EXPECT(&fixed_dynamic.get_type() == &view_dynamic.get_type());
  EXPECT(
      &static_cast<const Types::View&>(fixed_dynamic.get_type())
           .get_element_type() == &element);
  EXPECT(
      &static_cast<const Types::Access&>(access_dynamic.get_type())
           .get_element_type() == &element);
  EXPECT(input_is(constant_size, 0, fixed_receiver));
  EXPECT(input_is(constant_size, 1, start));
  EXPECT(input_is(constant_size, 2, fixed_size));
}

PERIMORTEM_UNIT_TEST(LibrarySlice, constant_byte_payloads) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  const auto& element = Tetrodotoxin::Library::Dialect::get_unsigned_8();
  Types::Unsigned_64 integer;
  Types::Fixed bytes_type("Fixed[Unsigned_8,6]"_view, element, 6);
  Constants::Bytes bytes(bytes_type, "abcdef"_view);
  Constants::Unsigned zero(integer, 0);
  Constants::Unsigned one(integer, 1);
  Constants::Unsigned two(integer, 2);
  Constants::Unsigned four(integer, 4);
  Constants::Unsigned six(integer, 6);
  Operations::Slice index(domain, materializations, bytes, one);
  Operations::Slice full(domain, materializations, bytes, zero, six);
  Operations::Slice interior(domain, materializations, bytes, one, four);
  Operations::Slice empty(domain, materializations, bytes, two, zero);
  Operations::Slice terminal_empty(domain, materializations, bytes, six, zero);

  auto indexed = selected(index.attempt_fold(domain, materializations));
  auto full_value = selected(full.attempt_fold(domain, materializations));
  auto interior_value =
      selected(interior.attempt_fold(domain, materializations));
  auto empty_value = selected(empty.attempt_fold(domain, materializations));
  auto terminal_value =
      selected(terminal_empty.attempt_fold(domain, materializations));

  ASSERT(
      indexed && full_value && interior_value && empty_value && terminal_value);
  EXPECT(indexed->is<Constants::Unsigned>());
  EXPECT(
      static_cast<const Constants::Unsigned&>(*indexed).get_value() ==
      Unsigned_64('b'));
  EXPECT_TEXT(
      static_cast<const Constants::Bytes&>(*full_value).get_value(),
      "abcdef"_view);
  EXPECT_TEXT(
      static_cast<const Constants::Bytes&>(*interior_value).get_value(),
      "bcde"_view);
  EXPECT(
      static_cast<const Constants::Bytes&>(*empty_value)
          .get_value()
          .is_empty());
  EXPECT(
      static_cast<const Constants::Bytes&>(*terminal_value)
          .get_value()
          .is_empty());
  EXPECT(&indexed->get_type() == &element);
  EXPECT(full_value->get_type().is<Types::Fixed>());
  EXPECT(interior_value->get_type().is<Types::Fixed>());
  EXPECT(empty_value->get_type().is<Types::Fixed>());
  EXPECT(terminal_value->get_type().is<Types::Fixed>());

  Operations::Slice chained(
      domain, materializations, *interior_value, one, two);
  auto chained_value = selected(chained.attempt_fold(domain, materializations));
  ASSERT(chained_value);
  EXPECT_TEXT(
      static_cast<const Constants::Bytes&>(*chained_value).get_value(),
      "cd"_view);
}

PERIMORTEM_UNIT_TEST(LibrarySlice, partial_folding) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Unsigned_8 element;
  Types::Unsigned_64 integer;
  Types::Fixed fixed("Fixed[Unsigned_8,4]"_view, element, 4);
  SliceExpression dynamic_receiver("receiver"_view, fixed);
  SliceExpression dynamic_index("index"_view, integer);
  SliceExpression dynamic_start("start"_view, integer);
  SliceExpression dynamic_size("size"_view, integer);
  Constants::Bytes bytes(fixed, "abcd"_view);
  Constants::Unsigned zero(integer, 0);
  Constants::Unsigned two(integer, 2);
  Operations::Slice receiver_partial(
      domain, materializations, dynamic_receiver, zero);
  Operations::Slice index_partial(
      domain, materializations, bytes, dynamic_index);
  Operations::Slice start_partial(
      domain, materializations, bytes, dynamic_start, two);
  Operations::Slice size_partial(
      domain, materializations, bytes, zero, dynamic_size);

  EXPECT(selects(
      receiver_partial.attempt_fold(domain, materializations),
      receiver_partial));
  EXPECT(selects(
      index_partial.attempt_fold(domain, materializations), index_partial));
  EXPECT(selects(
      start_partial.attempt_fold(domain, materializations), start_partial));
  EXPECT(selects(
      size_partial.attempt_fold(domain, materializations), size_partial));
  EXPECT(&receiver_partial.get_type() == &element);
  EXPECT(&index_partial.get_type() == &element);
  EXPECT(start_partial.get_type().is<Types::Fixed>());
  EXPECT(size_partial.get_type().is<Types::View>());
}

PERIMORTEM_UNIT_TEST(LibrarySlice, rejected_inputs) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Unsigned_8 element;
  Types::Unsigned_64 integer;
  Types::Signed_64 signed_integer;
  Types::Boolean flag_type;
  Types::Fixed fixed("Fixed[Unsigned_8,3]"_view, element, 3);
  Constants::Bytes bytes(fixed, "abc"_view);
  Constants::Unsigned zero(integer, 0);
  Constants::Unsigned one(integer, 1);
  Constants::Unsigned two(integer, 2);
  Constants::Unsigned three(integer, 3);
  Constants::Unsigned four(integer, 4);
  Constants::Unsigned maximum(integer, Unsigned_64(-1));
  Constants::Signed negative(signed_integer, -1);
  Constants::True flag(flag_type);
  Operations::Slice invalid_receiver(domain, materializations, flag, zero);
  Operations::Slice invalid_operand(domain, materializations, bytes, flag);
  Operations::Slice negative_operand(domain, materializations, bytes, negative);
  Operations::Slice overflow(domain, materializations, bytes, zero, maximum);
  Operations::Slice index_bounds(domain, materializations, bytes, three);
  Operations::Slice start_bounds(domain, materializations, bytes, four, zero);
  Operations::Slice size_bounds(domain, materializations, bytes, two, two);

  EXPECT(reports(
      invalid_receiver.attempt_fold(domain, materializations),
      FoldError::InvalidReceiverType));
  EXPECT(reports(
      invalid_operand.attempt_fold(domain, materializations),
      FoldError::InvalidOperandType));
  EXPECT(reports(
      negative_operand.attempt_fold(domain, materializations),
      FoldError::NegativeOperand));
  EXPECT(reports(
      overflow.attempt_fold(domain, materializations),
      FoldError::CountOverflow));
  EXPECT(reports(
      index_bounds.attempt_fold(domain, materializations),
      FoldError::IndexOutOfBounds));
  EXPECT(reports(
      start_bounds.attempt_fold(domain, materializations),
      FoldError::RangeStartOutOfBounds));
  EXPECT(reports(
      size_bounds.attempt_fold(domain, materializations),
      FoldError::RangeSizeOutOfBounds));
  EXPECT(&invalid_receiver.get_type() == &Invalid::get_invalid());
  EXPECT(&invalid_operand.get_type() == &Invalid::get_invalid());
}
