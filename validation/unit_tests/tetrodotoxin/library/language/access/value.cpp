// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/value.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
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
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Validation;
using Tetrodotoxin::Library::Language::Access::Value;

static Harness LibraryValue = {
  .name = "Tetrodotoxin::Library::Language::Access::Value"_view,
};

class ValueMonograph : public Tetrodotoxin::Language::Monograph {
 public:
  ValueMonograph(Allocator::Arena& domain)
      : Tetrodotoxin::Language::Monograph(domain, Documentation::get_empty()) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "ValueMonograph"_view;
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

static auto create_source(
    Allocator::Arena& domain,
    Tetrodotoxin::Library::Dialect& dialect,
    Abstract& context) -> Option<Monograph&> {
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(domain, {}, "value-source.ttx"_view);
  Ttx::Lexical::Cursor cursor(tokenizer, errors);
  auto retained = dialect.interpret(
      domain, cursor, Documentation::get_empty(),
      Ttx::Lexical::Anchor::create({}), context);
  BAIL_IF(!retained || !errors.is_empty() || !retained->is<Monograph>());
  return static_cast<Monograph&>(*retained);
}

static auto link_operation(Operation& operation, Monograph& source) -> Bool {
  return operation.link(source, Invalid::get_invalid());
}

class ValueExpression : public Expression {
 public:
  ValueExpression(View::Bytes name, const Ttx::Model::Type& type)
      : Expression({}), name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Ttx::Model::Type& override { return type; }

 private:
  View::Bytes name;
  const Ttx::Model::Type& type;
};

class ValueConstant : public Constant {
 public:
  explicit ValueConstant(const Ttx::Model::Type& type)
      : Constant({}), type(type) {}

  constexpr auto get_type() const -> const Ttx::Model::Type& override {
    return type;
  }
  constexpr auto equals(const Constant& rhs) const -> Bool override {
    return has_same_type(rhs);
  }

 private:
  const Ttx::Model::Type& type;
};

class ValueFoldOperation : public Operation {
 public:
  ValueFoldOperation(
      Allocator::Arena& domain,
      Expression& input,
      Constant& result,
      const Ttx::Model::Type& type,
      Bool fails = False)
      : Operation(
            domain,
            Static::Vector<Reference<Expression>, 1>{{input}},
            {}),
        result(result),
        type(type),
        fails(fails) {}

  auto get_name() const -> View::Bytes override { return "Fold size"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }

 protected:
  auto evaluate_constants(Allocator::Arena&)
      -> Result<Option<Constant&>, Expression::Error> override {
    if (fails) {
      return Expression::Error(Expression::Error::Type::InvalidConstant, *this);
    }

    return result;
  }

  auto select_type(Tetrodotoxin::Language::Monograph&) const
      -> Option<const Ttx::Model::Type&> override {
    return type;
  }

 private:
  Constant& result;
  const Ttx::Model::Type& type;
  Bool fails;
};

static auto is_dynamic(
    const Result<Option<Expression&>, Expression::Error>& result) -> Bool {
  return result.visit(
      [](const Option<Expression&>& selected) {
        return !selected ? True : False;
      },
      [](const Expression::Error&) { return False; });
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

static auto supplies_self(const Value& value, Count size) -> Bool {
  const Layout& layout = value.get_layout();
  BAIL_IF(layout.get_size() != size);
  for (Count index = 0; index < size; index++) {
    auto source = layout.get_abstract(index);
    BAIL_IF(!source || &*source != &value);
  }
  return True;
}

PERIMORTEM_UNIT_TEST(LibraryValue, receiver_type_selection) {
  Allocator::Arena domain;
  ValueMonograph context(domain);
  Tetrodotoxin::Library::Dialect dialect;
  auto retained_source = create_source(domain, dialect, context);
  ASSERT(retained_source);
  auto& source = *retained_source;
  const auto& element = Tetrodotoxin::Library::Dialect::get_unsigned_8();
  Types::Signed_64 integer;
  Types::Fixed fixed("Fixed[Unsigned_8,0]"_view, element, 0);
  Types::View view("View[Unsigned_8]"_view, element);
  Types::Access access("Access[Unsigned_8]"_view, element);
  ValueExpression fixed_receiver("fixed"_view, fixed);
  ValueExpression view_receiver("view"_view, view);
  ValueExpression access_receiver("access"_view, access);
  ValueExpression index("index"_view, integer);
  auto& fixed_index = Value::create_synthetic(domain, fixed_receiver, index);
  auto& view_index = Value::create_synthetic(domain, view_receiver, index);
  auto& access_index = Value::create_synthetic(domain, access_receiver, index);

  EXPECT(fixed_index.get_type().resolve().is<Invalid>());
  EXPECT_NOT(fixed_index.get_anchor());
  EXPECT(link_operation(fixed_index, source));
  EXPECT(link_operation(view_index, source));
  EXPECT(link_operation(access_index, source));

  EXPECT(&fixed_index.get_type() == &element);
  EXPECT(&view_index.get_type() == &element);
  EXPECT(&access_index.get_type() == &element);
  EXPECT(is_dynamic(fixed_index.fold()));
  EXPECT(is_dynamic(view_index.fold()));
  EXPECT(is_dynamic(access_index.fold()));
}

PERIMORTEM_UNIT_TEST(LibraryValue, range_pack_shape) {
  Allocator::Arena domain;
  ValueMonograph context(domain);
  Tetrodotoxin::Library::Dialect dialect;
  auto retained_source = create_source(domain, dialect, context);
  ASSERT(retained_source);
  auto& source = *retained_source;
  Types::Unsigned_8 element;
  Types::Signed_64 integer;
  Types::Unsigned_64 unsigned_integer;
  Types::Fixed fixed("Fixed[Unsigned_8,8]"_view, element, 8);
  Types::View view("View[Unsigned_8]"_view, element);
  Types::Access access("Access[Unsigned_8]"_view, element);
  ValueExpression fixed_receiver("fixed"_view, fixed);
  ValueExpression view_receiver("view"_view, view);
  ValueExpression access_receiver("access"_view, access);
  ValueExpression start("start"_view, integer);
  ValueExpression dynamic_size("size"_view, integer);
  auto& fold_input =
      Constants::Unsigned::create_synthetic(domain, unsigned_integer, 1);
  auto& fixed_size =
      Constants::Unsigned::create_synthetic(domain, unsigned_integer, 4);
  auto& empty_size =
      Constants::Unsigned::create_synthetic(domain, unsigned_integer, 0);
  ValueFoldOperation size_operation(
      domain, fold_input, fixed_size, unsigned_integer);
  auto& fixed_dynamic =
      Value::create_synthetic(domain, fixed_receiver, start, dynamic_size);
  auto& view_dynamic =
      Value::create_synthetic(domain, view_receiver, start, dynamic_size);
  auto& access_dynamic =
      Value::create_synthetic(domain, access_receiver, start, dynamic_size);
  auto& constant_size =
      Value::create_synthetic(domain, fixed_receiver, start, fixed_size);
  auto& folded_size =
      Value::create_synthetic(domain, fixed_receiver, start, size_operation);
  auto& view_size =
      Value::create_synthetic(domain, view_receiver, start, fixed_size);
  auto& access_size =
      Value::create_synthetic(domain, access_receiver, start, fixed_size);
  auto& single_size =
      Value::create_synthetic(domain, fixed_receiver, start, fold_input);
  auto& empty =
      Value::create_synthetic(domain, fixed_receiver, start, empty_size);

  EXPECT(fixed_dynamic.get_type().resolve().is<Invalid>());
  EXPECT(!link_operation(fixed_dynamic, source));
  EXPECT(!link_operation(view_dynamic, source));
  EXPECT(!link_operation(access_dynamic, source));
  EXPECT(link_operation(constant_size, source));
  EXPECT(link_operation(folded_size, source));
  EXPECT(link_operation(view_size, source));
  EXPECT(link_operation(access_size, source));
  EXPECT(link_operation(single_size, source));
  EXPECT(link_operation(empty, source));

  auto folded_result = folded_size.fold();

  EXPECT(constant_size.get_type().is<Invalid>());
  EXPECT(folded_size.get_type().is<Invalid>());
  EXPECT(view_size.get_type().is<Invalid>());
  EXPECT(access_size.get_type().is<Invalid>());
  EXPECT(&single_size.get_type() == &element);
  EXPECT(empty.get_type().is<Invalid>());
  EXPECT(is_dynamic(folded_result));
  EXPECT(supplies_self(constant_size, 4));
  EXPECT(supplies_self(folded_size, 4));
  EXPECT(supplies_self(view_size, 4));
  EXPECT(supplies_self(access_size, 4));
  EXPECT(supplies_self(single_size, 1));
  EXPECT(supplies_self(empty, 0));
  Types::Fixed four_values("Fixed[Unsigned_8,4]"_view, element, 4);
  Types::Fixed no_values("Fixed[Unsigned_8,0]"_view, element, 0);
  EXPECT(constant_size.get_layout().fits(four_values.get_layout()));
  EXPECT(empty.get_layout().fits(no_values.get_layout()));
}

PERIMORTEM_UNIT_TEST(LibraryValue, scalar_fold_and_range_provenance) {
  Allocator::Arena domain;
  ValueMonograph context(domain);
  Tetrodotoxin::Library::Dialect dialect;
  auto retained_source = create_source(domain, dialect, context);
  ASSERT(retained_source);
  auto& source = *retained_source;
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
  auto& index = Value::create_synthetic(domain, bytes, one);
  auto& full = Value::create_synthetic(domain, bytes, zero, six);
  auto& interior = Value::create_synthetic(domain, bytes, one, four);
  auto& empty = Value::create_synthetic(domain, bytes, two, zero);
  auto& terminal_empty = Value::create_synthetic(domain, bytes, six, zero);

  EXPECT(index.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(index, source));
  EXPECT(link_operation(full, source));
  EXPECT(link_operation(interior, source));
  EXPECT(link_operation(empty, source));
  EXPECT(link_operation(terminal_empty, source));

  auto indexed = selected(index.fold());
  auto full_value = full.fold();
  auto interior_value = interior.fold();
  auto empty_value = empty.fold();
  auto terminal_value = terminal_empty.fold();
  auto indexed_byte = indexed ? get_unsigned(*indexed) : Option<Unsigned_64>();

  ASSERT(indexed);
  EXPECT(indexed->is<Constants::Unsigned>());
  EXPECT(indexed_byte && *indexed_byte == Unsigned_64('b'));
  EXPECT(&indexed->get_type() == &element);
  EXPECT(is_dynamic(full_value));
  EXPECT(is_dynamic(interior_value));
  EXPECT(is_dynamic(empty_value));
  EXPECT(is_dynamic(terminal_value));
  EXPECT(supplies_self(full, 6));
  EXPECT(supplies_self(interior, 4));
  EXPECT(supplies_self(empty, 0));
  EXPECT(supplies_self(terminal_empty, 0));
}

PERIMORTEM_UNIT_TEST(LibraryValue, partial_folding) {
  Allocator::Arena domain;
  ValueMonograph context(domain);
  Tetrodotoxin::Library::Dialect dialect;
  auto retained_source = create_source(domain, dialect, context);
  ASSERT(retained_source);
  auto& source = *retained_source;
  Types::Unsigned_8 element;
  Types::Unsigned_64 integer;
  Types::Fixed fixed("Fixed[Unsigned_8,4]"_view, element, 4);
  ValueExpression dynamic_receiver("receiver"_view, fixed);
  ValueExpression dynamic_index("index"_view, integer);
  ValueExpression dynamic_start("start"_view, integer);
  ValueExpression dynamic_size("size"_view, integer);
  auto& bytes = Constants::Bytes::create_synthetic(domain, fixed, "abcd"_view);
  auto& zero = Constants::Unsigned::create_synthetic(domain, integer, 0);
  auto& two = Constants::Unsigned::create_synthetic(domain, integer, 2);
  auto& receiver_partial =
      Value::create_synthetic(domain, dynamic_receiver, zero);
  auto& index_partial = Value::create_synthetic(domain, bytes, dynamic_index);
  auto& start_partial =
      Value::create_synthetic(domain, bytes, dynamic_start, two);
  auto& size_partial =
      Value::create_synthetic(domain, bytes, zero, dynamic_size);

  EXPECT(receiver_partial.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(receiver_partial, source));
  EXPECT(link_operation(index_partial, source));
  EXPECT(link_operation(start_partial, source));
  EXPECT(!link_operation(size_partial, source));

  EXPECT(is_dynamic(receiver_partial.fold()));
  EXPECT(is_dynamic(index_partial.fold()));
  EXPECT(is_dynamic(start_partial.fold()));
  EXPECT(&receiver_partial.get_type() == &element);
  EXPECT(&index_partial.get_type() == &element);
  EXPECT(start_partial.get_type().is<Invalid>());
  EXPECT(supplies_self(start_partial, 2));
  EXPECT(size_partial.get_type().is<Invalid>());
}

PERIMORTEM_UNIT_TEST(LibraryValue, operand_rejection_and_safe_bounds) {
  Allocator::Arena domain;
  ValueMonograph context(domain);
  Tetrodotoxin::Library::Dialect dialect;
  auto retained_source = create_source(domain, dialect, context);
  ASSERT(retained_source);
  auto& source = *retained_source;
  const auto& element = Tetrodotoxin::Library::Dialect::get_unsigned_8();
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
  auto& invalid_receiver = Value::create_synthetic(domain, flag, zero);
  auto& invalid_operand = Value::create_synthetic(domain, bytes, flag);
  auto& invalid_count = Value::create_synthetic(domain, bytes, zero, flag);
  auto& negative_index = Value::create_synthetic(domain, bytes, negative);
  auto& maximum_index = Value::create_synthetic(domain, bytes, maximum);
  auto& maximum_range = Value::create_synthetic(domain, bytes, zero, maximum);
  auto& negative_start = Value::create_synthetic(domain, bytes, negative, two);
  auto& negative_size = Value::create_synthetic(domain, bytes, zero, negative);
  auto& index_bounds = Value::create_synthetic(domain, bytes, three);
  ValueFoldOperation nested_index(domain, zero, three, integer);
  auto& nested_index_bounds =
      Value::create_synthetic(domain, bytes, nested_index);
  auto& start_bounds = Value::create_synthetic(domain, bytes, four, zero);
  auto& size_bounds = Value::create_synthetic(domain, bytes, two, two);

  EXPECT(invalid_receiver.get_type().resolve().is<Invalid>());
  EXPECT(!link_operation(invalid_receiver, source));
  EXPECT(!link_operation(invalid_operand, source));
  EXPECT(!link_operation(invalid_count, source));
  EXPECT(link_operation(negative_index, source));
  EXPECT(link_operation(maximum_index, source));
  EXPECT(link_operation(maximum_range, source));
  EXPECT(link_operation(negative_start, source));
  EXPECT(!link_operation(negative_size, source));
  EXPECT(link_operation(index_bounds, source));
  EXPECT(link_operation(nested_index_bounds, source));
  EXPECT(link_operation(start_bounds, source));
  EXPECT(link_operation(size_bounds, source));

  EXPECT(is_dynamic(invalid_receiver.fold()));
  EXPECT(is_dynamic(invalid_operand.fold()));
  EXPECT(is_dynamic(invalid_count.fold()));
  auto negative_index_value = selected(negative_index.fold());
  auto maximum_index_value = selected(maximum_index.fold());
  auto index_bounds_value = selected(index_bounds.fold());
  auto nested_index_value = selected(nested_index_bounds.fold());
  auto negative_index_default = negative_index_value
                                    ? get_unsigned(*negative_index_value)
                                    : Option<Unsigned_64>();
  auto maximum_index_default = maximum_index_value
                                   ? get_unsigned(*maximum_index_value)
                                   : Option<Unsigned_64>();
  auto index_bounds_default = index_bounds_value
                                  ? get_unsigned(*index_bounds_value)
                                  : Option<Unsigned_64>();
  auto nested_index_default = nested_index_value
                                  ? get_unsigned(*nested_index_value)
                                  : Option<Unsigned_64>();
  ASSERT(
      negative_index_default && maximum_index_default && index_bounds_default &&
      nested_index_default);
  EXPECT(*negative_index_default == 0);
  EXPECT(*maximum_index_default == 0);
  EXPECT(*index_bounds_default == 0);
  EXPECT(*nested_index_default == 0);
  EXPECT(&negative_index_value->get_type() == &element);
  EXPECT(&maximum_index_value->get_type() == &element);
  EXPECT(&index_bounds_value->get_type() == &element);
  EXPECT(&nested_index_value->get_type() == &element);
  EXPECT(is_dynamic(negative_start.fold()));
  EXPECT(is_dynamic(maximum_range.fold()));
  EXPECT(is_dynamic(start_bounds.fold()));
  EXPECT(is_dynamic(size_bounds.fold()));
  EXPECT(supplies_self(negative_start, 2));
  EXPECT_EQ(maximum_range.get_layout().get_size(), Count(-1));
  EXPECT(supplies_self(start_bounds, 0));
  EXPECT(supplies_self(size_bounds, 2));
  EXPECT(&invalid_receiver.get_type() == &Invalid::get_invalid());
  EXPECT(&invalid_operand.get_type() == &Invalid::get_invalid());
  EXPECT(&invalid_count.get_type() == &Invalid::get_invalid());
}

PERIMORTEM_UNIT_TEST(LibraryValue, scalar_defaults) {
  Allocator::Arena domain;
  ValueMonograph context(domain);
  Tetrodotoxin::Library::Dialect dialect;
  auto retained_source = create_source(domain, dialect, context);
  ASSERT(retained_source);
  auto& source = *retained_source;
  const auto& boolean = Tetrodotoxin::Library::Dialect::get_bool();
  const auto& signed_integer = Tetrodotoxin::Library::Dialect::get_signed_64();
  const auto& real = Tetrodotoxin::Library::Dialect::get_real_64();
  const auto& index_type = Tetrodotoxin::Library::Dialect::get_unsigned_64();
  Types::Fixed booleans("Fixed[Bool,0]"_view, boolean, 0);
  Types::Fixed signed_values("Fixed[Signed_64,0]"_view, signed_integer, 0);
  Types::Fixed real_values("Fixed[Real_64,0]"_view, real, 0);
  auto& boolean_bytes =
      Constants::Bytes::create_synthetic(domain, booleans, {});
  auto& signed_bytes =
      Constants::Bytes::create_synthetic(domain, signed_values, {});
  auto& real_bytes =
      Constants::Bytes::create_synthetic(domain, real_values, {});
  auto& zero = Constants::Unsigned::create_synthetic(domain, index_type, 0);
  auto& boolean_default = Value::create_synthetic(domain, boolean_bytes, zero);
  auto& signed_default = Value::create_synthetic(domain, signed_bytes, zero);
  auto& real_default = Value::create_synthetic(domain, real_bytes, zero);

  EXPECT(link_operation(boolean_default, source));
  EXPECT(link_operation(signed_default, source));
  EXPECT(link_operation(real_default, source));

  auto boolean_value = selected(boolean_default.fold());
  auto signed_value = selected(signed_default.fold());
  auto real_value = selected(real_default.fold());
  auto signed_payload =
      signed_value ? get_signed(*signed_value) : Option<Signed_64>();
  auto real_payload = real_value ? get_real(*real_value) : Option<Real_64>();

  ASSERT(boolean_value && signed_payload && real_payload);
  EXPECT(boolean_value->is<Constants::False>());
  EXPECT(*signed_payload == 0);
  EXPECT(*real_payload == 0.0);
  EXPECT(&boolean_value->get_type() == &boolean);
  EXPECT(&signed_value->get_type() == &signed_integer);
  EXPECT(&real_value->get_type() == &real);
}

PERIMORTEM_UNIT_TEST(LibraryValue, unsupported_default_and_payload) {
  Allocator::Arena domain;
  ValueMonograph context(domain);
  Tetrodotoxin::Library::Dialect dialect;
  auto retained_source = create_source(domain, dialect, context);
  ASSERT(retained_source);
  auto& source = *retained_source;
  Types::Unsigned_8 unsupported_element;
  const auto& integer = Tetrodotoxin::Library::Dialect::get_unsigned_64();
  const auto& signed_integer = Tetrodotoxin::Library::Dialect::get_signed_64();
  Types::Fixed fixed("Fixed[Unsigned_8,1]"_view, unsupported_element, 1);
  auto& bytes = Constants::Bytes::create_synthetic(domain, fixed, "a"_view);
  ValueConstant opaque(fixed);
  auto& zero = Constants::Unsigned::create_synthetic(domain, integer, 0);
  auto& one = Constants::Unsigned::create_synthetic(domain, integer, 1);
  auto& negative =
      Constants::Signed::create_synthetic(domain, signed_integer, -1);
  auto& missing = Value::create_synthetic(domain, bytes, one);
  auto& unsupported = Value::create_synthetic(domain, opaque, zero);
  auto& safe_range = Value::create_synthetic(domain, opaque, negative, one);

  EXPECT(link_operation(missing, source));
  EXPECT(link_operation(unsupported, source));
  EXPECT(link_operation(safe_range, source));

  EXPECT(reports(
      missing.fold(), Expression::Error::Type::InvalidConstant, missing));
  EXPECT(is_dynamic(unsupported.fold()));
  EXPECT(is_dynamic(safe_range.fold()));
  EXPECT(supplies_self(safe_range, 1));
  EXPECT(&safe_range.get_type() == &unsupported_element);
  EXPECT(&missing.get_type() == &unsupported_element);
  EXPECT(&unsupported.get_type() == &unsupported_element);
}

PERIMORTEM_UNIT_TEST(LibraryValue, child_failure_propagates) {
  Allocator::Arena domain;
  ValueMonograph context(domain);
  Tetrodotoxin::Library::Dialect dialect;
  auto retained_source = create_source(domain, dialect, context);
  ASSERT(retained_source);
  auto& source = *retained_source;
  const auto& element = Tetrodotoxin::Library::Dialect::get_unsigned_8();
  const auto& integer = Tetrodotoxin::Library::Dialect::get_unsigned_64();
  Types::Fixed fixed("Fixed[Unsigned_8,1]"_view, element, 1);
  auto& bytes = Constants::Bytes::create_synthetic(domain, fixed, "a"_view);
  auto& zero = Constants::Unsigned::create_synthetic(domain, integer, 0);
  auto& one = Constants::Unsigned::create_synthetic(domain, integer, 1);
  ValueFoldOperation failing(domain, zero, one, integer, True);
  auto& access = Value::create_synthetic(domain, bytes, failing);

  EXPECT(link_operation(access, source));

  auto direct = failing.fold();
  auto propagated = access.fold();
  auto repeated = access.fold();

  EXPECT(reports(direct, Expression::Error::Type::InvalidConstant, failing));
  EXPECT(
      reports(propagated, Expression::Error::Type::InvalidConstant, failing));
  EXPECT(reports(repeated, Expression::Error::Type::InvalidConstant, failing));
}
