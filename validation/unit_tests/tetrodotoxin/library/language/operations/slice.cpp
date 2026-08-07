// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/slice.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/option.hpp"
#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/language/monograph.hpp"
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

class SliceMonograph : public Tetrodotoxin::Language::Monograph {
 public:
  SliceMonograph(Allocator::Arena& domain)
      : Tetrodotoxin::Language::Monograph(domain, Documentation::get_empty()) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "SliceMonograph"_view;
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

static auto link_operation(
    Operation& operation,
    SliceMonograph& source,
    Materializations& materializations) -> Bool {
  return operation.link(source, Invalid::get_invalid(), materializations);
}

class SliceExpression : public Expression {
 public:
  SliceExpression(View::Bytes name, const Ttx::Model::Type& type)
      : Expression({}), name(name), type(type) {}

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

  auto get_name() const -> View::Bytes override { return "Fold size"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

 protected:
  auto evaluate_constants(Allocator::Arena&, Materializations&)
      -> Result<Option<Constant&>, Expression::Error> override {
    return result;
  }

  auto select_type(Materializations&) const
      -> Option<const Ttx::Model::Type&> override {
    return type;
  }

 private:
  Constant& result;
  const Ttx::Model::Type& type;
};

static auto reports(
    const Result<Option<Expression&>, Expression::Error>& result,
    Expression::Error::Type expected,
    const Expression& origin) -> Bool {
  return result.visit(
      [](const Option<Expression&>&) { return False; },
      [&](const Expression::Error& selected) {
        return selected.get_type() == expected &&
                       &selected.get_expression() == &origin
                   ? True
                   : False;
      });
}

static auto is_dynamic(
    const Result<Option<Expression&>, Expression::Error>& result) -> Bool {
  return result.visit(
      [](const Option<Expression&>& selected) {
        return !selected ? True : False;
      },
      [](const Expression::Error&) { return False; });
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

static auto get_fixed(const Abstract& type) -> Option<const Types::Fixed&> {
  return type.visit<Types::Fixed>(
      [](const Types::Fixed& selected) -> Option<const Types::Fixed&> {
        return selected;
      },
      [](const Abstract&) -> Option<const Types::Fixed&> { return {}; });
}

static auto get_view(const Abstract& type) -> Option<const Types::View&> {
  return type.visit<Types::View>(
      [](const Types::View& selected) -> Option<const Types::View&> {
        return selected;
      },
      [](const Abstract&) -> Option<const Types::View&> { return {}; });
}

static auto get_access(const Abstract& type) -> Option<const Types::Access&> {
  return type.visit<Types::Access>(
      [](const Types::Access& selected) -> Option<const Types::Access&> {
        return selected;
      },
      [](const Abstract&) -> Option<const Types::Access&> { return {}; });
}

static auto get_unsigned(const Expression& expression) -> Option<Unsigned_64> {
  return expression.visit<Constants::Unsigned>(
      [](const Constants::Unsigned& selected) -> Option<Unsigned_64> {
        return selected.get_value();
      },
      [](const Abstract&) -> Option<Unsigned_64> { return {}; });
}

static auto get_bytes(const Expression& expression) -> Option<View::Bytes> {
  return expression.visit<Constants::Bytes>(
      [](const Constants::Bytes& selected) -> Option<View::Bytes> {
        return selected.get_value();
      },
      [](const Abstract&) -> Option<View::Bytes> { return {}; });
}

PERIMORTEM_UNIT_TEST(LibrarySlice, receiver_type_selection) {
  Allocator::Arena domain;
  SliceMonograph source(domain);
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
  auto& fixed_index = Operations::Slice::create_synthetic(
      domain, materializations, fixed_receiver, index);
  auto& view_index = Operations::Slice::create_synthetic(
      domain, materializations, view_receiver, index);
  auto& access_index = Operations::Slice::create_synthetic(
      domain, materializations, access_receiver, index);

  EXPECT(fixed_index.get_type().resolve().is<Invalid>());
  EXPECT_NOT(fixed_index.get_anchor());
  EXPECT(link_operation(fixed_index, source, materializations));
  EXPECT(link_operation(view_index, source, materializations));
  EXPECT(link_operation(access_index, source, materializations));

  EXPECT(&fixed_index.get_type() == &element);
  EXPECT(&view_index.get_type() == &element);
  EXPECT(&access_index.get_type() == &element);
  EXPECT(is_dynamic(fixed_index.fold()));
  EXPECT(is_dynamic(view_index.fold()));
  EXPECT(is_dynamic(access_index.fold()));
  ASSERT_EQ(fixed_index.get_inputs().get_size(), Count(2));
  EXPECT(input_is(fixed_index, 0, fixed_receiver));
  EXPECT(input_is(fixed_index, 1, index));
}

PERIMORTEM_UNIT_TEST(LibrarySlice, range_type_selection) {
  Allocator::Arena domain;
  SliceMonograph source(domain);
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
  auto& fold_input =
      Constants::Unsigned::create_synthetic(domain, unsigned_integer, 1);
  auto& fixed_size =
      Constants::Unsigned::create_synthetic(domain, unsigned_integer, 4);
  SliceFoldOperation size_operation(
      domain, materializations, fold_input, fixed_size, unsigned_integer);
  auto& fixed_dynamic = Operations::Slice::create_synthetic(
      domain, materializations, fixed_receiver, start, dynamic_size);
  auto& view_dynamic = Operations::Slice::create_synthetic(
      domain, materializations, view_receiver, start, dynamic_size);
  auto& access_dynamic = Operations::Slice::create_synthetic(
      domain, materializations, access_receiver, start, dynamic_size);
  auto& constant_size = Operations::Slice::create_synthetic(
      domain, materializations, fixed_receiver, start, fixed_size);
  auto& folded_size = Operations::Slice::create_synthetic(
      domain, materializations, fixed_receiver, start, size_operation);

  EXPECT(fixed_dynamic.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(fixed_dynamic, source, materializations));
  EXPECT(link_operation(view_dynamic, source, materializations));
  EXPECT(link_operation(access_dynamic, source, materializations));
  EXPECT(link_operation(constant_size, source, materializations));
  EXPECT(link_operation(folded_size, source, materializations));

  auto fixed_result = get_fixed(constant_size.get_type());
  auto fixed_view = get_view(fixed_dynamic.get_type());
  auto access_view = get_access(access_dynamic.get_type());
  const Abstract& folded_type = folded_size.get_type();
  auto folded_result = folded_size.fold();

  ASSERT(fixed_dynamic.get_type().is<Types::View>());
  ASSERT(view_dynamic.get_type().is<Types::View>());
  ASSERT(access_dynamic.get_type().is<Types::Access>());
  ASSERT(constant_size.get_type().is<Types::Fixed>());
  ASSERT(folded_type.is<Types::View>());
  EXPECT(is_dynamic(folded_result));
  EXPECT(&folded_size.get_type() == &folded_type);
  ASSERT(fixed_result && fixed_view && access_view);
  EXPECT(&fixed_result->get_element_type() == &element);
  EXPECT(fixed_result->get_extent() == 4);
  EXPECT(input_is(folded_size, 2, size_operation));
  EXPECT(&fixed_dynamic.get_type() == &view_dynamic.get_type());
  EXPECT(&fixed_view->get_element_type() == &element);
  EXPECT(&access_view->get_element_type() == &element);
  EXPECT(input_is(constant_size, 0, fixed_receiver));
  EXPECT(input_is(constant_size, 1, start));
  EXPECT(input_is(constant_size, 2, fixed_size));
}

PERIMORTEM_UNIT_TEST(LibrarySlice, constant_byte_payloads) {
  Allocator::Arena domain;
  SliceMonograph source(domain);
  Materializations materializations(domain);
  const auto& element = Tetrodotoxin::Library::Dialect::get_unsigned_8();
  Types::Unsigned_64 integer;
  Types::Fixed bytes_type("Fixed[Unsigned_8,6]"_view, element, 6);
  auto& bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "abcdef"_view);
  auto& zero = Constants::Unsigned::create_synthetic(domain, integer, 0);
  auto& one = Constants::Unsigned::create_synthetic(domain, integer, 1);
  auto& two = Constants::Unsigned::create_synthetic(domain, integer, 2);
  auto& four = Constants::Unsigned::create_synthetic(domain, integer, 4);
  auto& six = Constants::Unsigned::create_synthetic(domain, integer, 6);
  auto& index =
      Operations::Slice::create_synthetic(domain, materializations, bytes, one);
  auto& full = Operations::Slice::create_synthetic(
      domain, materializations, bytes, zero, six);
  auto& interior = Operations::Slice::create_synthetic(
      domain, materializations, bytes, one, four);
  auto& empty = Operations::Slice::create_synthetic(
      domain, materializations, bytes, two, zero);
  auto& terminal_empty = Operations::Slice::create_synthetic(
      domain, materializations, bytes, six, zero);

  EXPECT(index.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(index, source, materializations));
  EXPECT(link_operation(full, source, materializations));
  EXPECT(link_operation(interior, source, materializations));
  EXPECT(link_operation(empty, source, materializations));
  EXPECT(link_operation(terminal_empty, source, materializations));

  auto indexed = selected(index.fold());
  auto full_value = selected(full.fold());
  auto interior_value = selected(interior.fold());
  auto empty_value = selected(empty.fold());
  auto terminal_value = selected(terminal_empty.fold());
  auto indexed_byte = indexed ? get_unsigned(*indexed) : Option<Unsigned_64>();
  auto full_bytes = full_value ? get_bytes(*full_value) : Option<View::Bytes>();
  auto interior_bytes =
      interior_value ? get_bytes(*interior_value) : Option<View::Bytes>();
  auto empty_bytes =
      empty_value ? get_bytes(*empty_value) : Option<View::Bytes>();
  auto terminal_bytes =
      terminal_value ? get_bytes(*terminal_value) : Option<View::Bytes>();

  ASSERT(
      indexed && full_value && interior_value && empty_value && terminal_value);
  EXPECT(indexed->is<Constants::Unsigned>());
  EXPECT(indexed_byte && *indexed_byte == Unsigned_64('b'));
  ASSERT(full_bytes && interior_bytes && empty_bytes && terminal_bytes);
  EXPECT_TEXT(*full_bytes, "abcdef"_view);
  EXPECT_TEXT(*interior_bytes, "bcde"_view);
  EXPECT(empty_bytes->is_empty());
  EXPECT(terminal_bytes->is_empty());
  EXPECT(&indexed->get_type() == &element);
  EXPECT(full_value->get_type().is<Types::Fixed>());
  EXPECT(interior_value->get_type().is<Types::Fixed>());
  EXPECT(empty_value->get_type().is<Types::Fixed>());
  EXPECT(terminal_value->get_type().is<Types::Fixed>());

  auto& chained = Operations::Slice::create_synthetic(
      domain, materializations, *interior_value, one, two);
  EXPECT(chained.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(chained, source, materializations));

  auto chained_value = selected(chained.fold());
  auto chained_bytes =
      chained_value ? get_bytes(*chained_value) : Option<View::Bytes>();
  ASSERT(chained_value);
  ASSERT(chained_bytes);
  EXPECT_TEXT(*chained_bytes, "cd"_view);
}

PERIMORTEM_UNIT_TEST(LibrarySlice, partial_folding) {
  Allocator::Arena domain;
  SliceMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 element;
  Types::Unsigned_64 integer;
  Types::Fixed fixed("Fixed[Unsigned_8,4]"_view, element, 4);
  SliceExpression dynamic_receiver("receiver"_view, fixed);
  SliceExpression dynamic_index("index"_view, integer);
  SliceExpression dynamic_start("start"_view, integer);
  SliceExpression dynamic_size("size"_view, integer);
  auto& bytes = Constants::Bytes::create_synthetic(domain, fixed, "abcd"_view);
  auto& zero = Constants::Unsigned::create_synthetic(domain, integer, 0);
  auto& two = Constants::Unsigned::create_synthetic(domain, integer, 2);
  auto& receiver_partial = Operations::Slice::create_synthetic(
      domain, materializations, dynamic_receiver, zero);
  auto& index_partial = Operations::Slice::create_synthetic(
      domain, materializations, bytes, dynamic_index);
  auto& start_partial = Operations::Slice::create_synthetic(
      domain, materializations, bytes, dynamic_start, two);
  auto& size_partial = Operations::Slice::create_synthetic(
      domain, materializations, bytes, zero, dynamic_size);

  EXPECT(receiver_partial.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(receiver_partial, source, materializations));
  EXPECT(link_operation(index_partial, source, materializations));
  EXPECT(link_operation(start_partial, source, materializations));
  EXPECT(link_operation(size_partial, source, materializations));

  EXPECT(is_dynamic(receiver_partial.fold()));
  EXPECT(is_dynamic(index_partial.fold()));
  EXPECT(is_dynamic(start_partial.fold()));
  EXPECT(is_dynamic(size_partial.fold()));
  EXPECT(&receiver_partial.get_type() == &element);
  EXPECT(&index_partial.get_type() == &element);
  EXPECT(start_partial.get_type().is<Types::Fixed>());
  EXPECT(size_partial.get_type().is<Types::View>());
}

PERIMORTEM_UNIT_TEST(LibrarySlice, rejected_inputs) {
  Allocator::Arena domain;
  SliceMonograph source(domain);
  Materializations materializations(domain);
  Types::Unsigned_8 element;
  Types::Unsigned_64 integer;
  Types::Signed_64 signed_integer;
  Types::Boolean flag_type;
  Types::Fixed fixed("Fixed[Unsigned_8,3]"_view, element, 3);
  auto& bytes = Constants::Bytes::create_synthetic(domain, fixed, "abc"_view);
  auto& zero = Constants::Unsigned::create_synthetic(domain, integer, 0);
  auto& two = Constants::Unsigned::create_synthetic(domain, integer, 2);
  auto& three = Constants::Unsigned::create_synthetic(domain, integer, 3);
  auto& four = Constants::Unsigned::create_synthetic(domain, integer, 4);
  auto& maximum =
      Constants::Unsigned::create_synthetic(domain, integer, Unsigned_64(-1));
  auto& negative =
      Constants::Signed::create_synthetic(domain, signed_integer, -1);
  auto& flag = Constants::True::create_synthetic(domain, flag_type);
  auto& invalid_receiver =
      Operations::Slice::create_synthetic(domain, materializations, flag, zero);
  auto& invalid_operand = Operations::Slice::create_synthetic(
      domain, materializations, bytes, flag);
  auto& negative_operand = Operations::Slice::create_synthetic(
      domain, materializations, bytes, negative);
  auto& overflow = Operations::Slice::create_synthetic(
      domain, materializations, bytes, zero, maximum);
  auto& index_bounds = Operations::Slice::create_synthetic(
      domain, materializations, bytes, three);
  SliceFoldOperation nested_index(
      domain, materializations, zero, three, integer);
  auto& nested_index_bounds = Operations::Slice::create_synthetic(
      domain, materializations, bytes, nested_index);
  auto& start_bounds = Operations::Slice::create_synthetic(
      domain, materializations, bytes, four, zero);
  auto& size_bounds = Operations::Slice::create_synthetic(
      domain, materializations, bytes, two, two);

  EXPECT(invalid_receiver.get_type().resolve().is<Invalid>());
  EXPECT(!link_operation(invalid_receiver, source, materializations));
  EXPECT(!link_operation(invalid_operand, source, materializations));
  EXPECT(link_operation(negative_operand, source, materializations));
  EXPECT(!link_operation(overflow, source, materializations));
  EXPECT(link_operation(index_bounds, source, materializations));
  EXPECT(link_operation(nested_index_bounds, source, materializations));
  EXPECT(link_operation(start_bounds, source, materializations));
  EXPECT(link_operation(size_bounds, source, materializations));

  EXPECT(is_dynamic(invalid_receiver.fold()));
  EXPECT(is_dynamic(invalid_operand.fold()));
  EXPECT(reports(
      negative_operand.fold(), Expression::Error::Type::NegativeOperand,
      negative));
  EXPECT(is_dynamic(overflow.fold()));
  EXPECT(reports(
      index_bounds.fold(), Expression::Error::Type::IndexOutOfBounds, three));
  EXPECT(reports(
      nested_index_bounds.fold(), Expression::Error::Type::IndexOutOfBounds,
      nested_index));
  EXPECT(reports(
      start_bounds.fold(), Expression::Error::Type::RangeStartOutOfBounds,
      four));
  EXPECT(reports(
      size_bounds.fold(), Expression::Error::Type::RangeSizeOutOfBounds, two));
  EXPECT(&invalid_receiver.get_type() == &Invalid::get_invalid());
  EXPECT(&invalid_operand.get_type() == &Invalid::get_invalid());
}
