// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/greater.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/false.hpp"
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
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness LibraryGreater = {
  .name = "Tetrodotoxin::Library::Language::Operations::Greater"_view,
};

class GreaterExpression : public Expression {
 public:
  GreaterExpression(View::Bytes name, const Abstract& type)
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

class GreaterUnresolvedType : public Ttx::Model::Type {
 public:
  auto get_name() const -> View::Bytes override { return "Unresolved"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve() const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

class GreaterFoldInput : public Operation {
 public:
  GreaterFoldInput(
      Allocator::Arena& domain,
      const Expression& input,
      const Expression& result,
      const Ttx::Model::Type& type,
      Bool fails = False)
      : Operation(domain, Static::Vector<Reference<Expression>, 1>{{input}}),
        result(result),
        type(type),
        fails(fails) {}

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
    if (fails) {
      return FoldError(FoldError::Type::InvalidConstant, *this);
    }

    return result;
  }

 private:
  const Expression& result;
  const Ttx::Model::Type& type;
  Bool fails;
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

static auto input_is(
    const Operations::Greater& greater,
    Count index,
    const Expression& expected) -> Bool {
  return greater.get_inputs().get_abstract(index).visit(
      []() { return False; },
      [&](const Abstract& expression) {
        return &expression == &expected ? True : False;
      });
}

static auto matches_token(const Cursor& cursor, Token expected) -> Bool {
  Token current = cursor.current();
  return current.get_offset() == expected.get_offset() &&
         current.get_code() == expected.get_code();
}

static auto span_width(View::Bytes rendered) -> Count {
  Count caret = Algorithm::search(rendered, "^"_view);
  if (caret == Count(-1)) {
    return 0;
  }

  Count width = 1;
  while (caret + width < rendered.get_size() &&
         rendered[caret + width] == '-') {
    width++;
  }

  return width;
}

PERIMORTEM_UNIT_TEST(LibraryGreater, type_selection_and_partial) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Signed_8 signed_8;
  Types::Unsigned_8 unsigned_8;
  Types::Unsigned_16 unsigned_16;
  Types::Real_32 real_32;
  Types::Boolean boolean;
  Types::Fixed bytes_type(
      "Fixed[Unsigned_8,1]"_view,
      Tetrodotoxin::Library::Dialect::get_unsigned_8(), 1);
  GreaterUnresolvedType unresolved_type;
  GreaterExpression signed_left("signed left"_view, signed_8);
  GreaterExpression signed_right("signed right"_view, signed_8);
  GreaterExpression unsigned_left("unsigned left"_view, unsigned_8);
  GreaterExpression unsigned_right("unsigned right"_view, unsigned_8);
  GreaterExpression real_left("real left"_view, real_32);
  GreaterExpression real_right("real right"_view, real_32);
  GreaterExpression other("other"_view, unsigned_16);
  GreaterExpression unresolved("unresolved"_view, unresolved_type);
  GreaterExpression invalid("invalid"_view, Invalid::get_invalid());
  Constants::True truth(boolean);
  Constants::Bytes bytes(bytes_type, "x"_view);
  Operations::Greater signed_exact(domain, signed_left, signed_right);
  Operations::Greater unsigned_exact(domain, unsigned_left, unsigned_right);
  Operations::Greater real_exact(domain, real_left, real_right);
  Operations::Greater mismatch(domain, unsigned_left, other);
  Operations::Greater unresolved_pair(domain, unresolved, unresolved);
  Operations::Greater invalid_pair(domain, invalid, invalid);
  Operations::Greater flags(domain, truth, truth);
  Operations::Greater byte_values(domain, bytes, bytes);
  auto retained =
      selected(unsigned_exact.attempt_fold(domain, materializations));

  EXPECT(
      &signed_exact.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(
      &unsigned_exact.get_type() ==
      &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(&real_exact.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(retained && &*retained == &unsigned_exact);
  EXPECT(input_is(unsigned_exact, 0, unsigned_left));
  EXPECT(input_is(unsigned_exact, 1, unsigned_right));
  EXPECT(mismatch.get_type().resolve().is<Invalid>());
  EXPECT(unresolved_pair.get_type().resolve().is<Invalid>());
  EXPECT(invalid_pair.get_type().resolve().is<Invalid>());
  EXPECT(flags.get_type().resolve().is<Invalid>());
  EXPECT(byte_values.get_type().resolve().is<Invalid>());
}

PERIMORTEM_UNIT_TEST(LibraryGreater, integer_endpoints) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Signed_8 signed_type;
  Types::Unsigned_8 unsigned_type;
  Constants::Signed minimum(signed_type, -128);
  Constants::Signed maximum(signed_type, 127);
  Constants::Signed equal(signed_type, 127);
  Constants::Unsigned zero(unsigned_type, 0);
  Constants::Unsigned top(unsigned_type, 255);
  Constants::Unsigned same_top(unsigned_type, 255);
  Operations::Greater signed_true(domain, maximum, minimum);
  Operations::Greater signed_false(domain, minimum, maximum);
  Operations::Greater signed_equal(domain, maximum, equal);
  Operations::Greater unsigned_true(domain, top, zero);
  Operations::Greater unsigned_false(domain, zero, top);
  Operations::Greater unsigned_equal(domain, top, same_top);
  auto signed_yes =
      selected(signed_true.attempt_fold(domain, materializations));
  auto signed_no =
      selected(signed_false.attempt_fold(domain, materializations));
  auto signed_same =
      selected(signed_equal.attempt_fold(domain, materializations));
  auto unsigned_yes =
      selected(unsigned_true.attempt_fold(domain, materializations));
  auto unsigned_no =
      selected(unsigned_false.attempt_fold(domain, materializations));
  auto unsigned_same =
      selected(unsigned_equal.attempt_fold(domain, materializations));

  ASSERT(
      signed_yes && signed_no && signed_same && unsigned_yes && unsigned_no &&
      unsigned_same);
  EXPECT(signed_yes->is<Constants::True>());
  EXPECT(signed_no->is<Constants::False>());
  EXPECT(signed_same->is<Constants::False>());
  EXPECT(unsigned_yes->is<Constants::True>());
  EXPECT(unsigned_no->is<Constants::False>());
  EXPECT(unsigned_same->is<Constants::False>());
}

PERIMORTEM_UNIT_TEST(LibraryGreater, ieee_domains) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Real_32 real_32;
  Types::Real_64 real_64;
  Constants::Real narrow_left(real_32, 1.00000001);
  Constants::Real narrow_right(real_32, 1.0);
  Constants::Real wide_left(real_64, 4.0);
  Constants::Real wide_right(real_64, 3.0);
  Constants::Real positive_infinity(real_64, __builtin_inf());
  Constants::Real negative_infinity(real_64, -__builtin_inf());
  Constants::Real nan(real_64, __builtin_nan(""));
  Constants::Real positive_zero(real_64, 0.0);
  Constants::Real negative_zero(real_64, -0.0);
  Operations::Greater narrow(domain, narrow_left, narrow_right);
  Operations::Greater wide(domain, wide_left, wide_right);
  Operations::Greater positive_infinite(domain, positive_infinity, wide_left);
  Operations::Greater negative_infinite(domain, negative_infinity, wide_left);
  Operations::Greater left_unordered(domain, nan, wide_left);
  Operations::Greater right_unordered(domain, wide_left, nan);
  Operations::Greater zero_forward(domain, positive_zero, negative_zero);
  Operations::Greater zero_reverse(domain, negative_zero, positive_zero);
  auto narrow_result = selected(narrow.attempt_fold(domain, materializations));
  auto wide_result = selected(wide.attempt_fold(domain, materializations));
  auto positive_result =
      selected(positive_infinite.attempt_fold(domain, materializations));
  auto negative_result =
      selected(negative_infinite.attempt_fold(domain, materializations));
  auto left_nan =
      selected(left_unordered.attempt_fold(domain, materializations));
  auto right_nan =
      selected(right_unordered.attempt_fold(domain, materializations));
  auto forward = selected(zero_forward.attempt_fold(domain, materializations));
  auto reverse = selected(zero_reverse.attempt_fold(domain, materializations));

  ASSERT(
      narrow_result && wide_result && positive_result && negative_result &&
      left_nan && right_nan && forward && reverse);
  EXPECT(narrow_result->is<Constants::False>());
  EXPECT(wide_result->is<Constants::True>());
  EXPECT(positive_result->is<Constants::True>());
  EXPECT(negative_result->is<Constants::False>());
  EXPECT(left_nan->is<Constants::False>());
  EXPECT(right_nan->is<Constants::False>());
  EXPECT(forward->is<Constants::False>());
  EXPECT(reverse->is<Constants::False>());
  EXPECT(
      &narrow_result->get_type() ==
      &Tetrodotoxin::Library::Dialect::get_bool());
}

PERIMORTEM_UNIT_TEST(LibraryGreater, recursive_provenance_and_atomicity) {
  Allocator::Arena domain;
  Allocator::Arena rendering;
  Materializations materializations(domain);
  Types::Unsigned_8 selected_type;
  Constants::Unsigned input(selected_type, 1);
  Constants::Unsigned folded(selected_type, 8);
  Constants::Unsigned right(selected_type, 5);
  GreaterFoldInput child(domain, input, folded, selected_type);
  GreaterFoldInput failing(domain, input, folded, selected_type, True);
  Operations::Greater greater(domain, child, right);
  Operations::Greater failure(domain, failing, right);
  auto first = selected(greater.attempt_fold(domain, materializations));
  auto second = selected(greater.attempt_fold(domain, materializations));

  ASSERT(first && second);
  EXPECT(&*first == &*second);
  EXPECT(first->is<Constants::True>());
  EXPECT(input_is(greater, 0, folded));
  EXPECT(child.get_evaluations() == 1);
  EXPECT(reports(
      failure.attempt_fold(domain, materializations),
      FoldError::Type::InvalidConstant, failing));

  const auto& parser_type = Tetrodotoxin::Library::Dialect::get_unsigned_64();
  Constants::Unsigned left(parser_type, 2);
  Errors success_errors;
  Tokenizer success_tokens(domain, "> 1"_view, "greater.ttx"_view);
  Cursor success_cursor(success_tokens, success_errors);
  auto parsed = Operations::Greater::parse(
      domain, materializations, success_cursor, Invalid::get_invalid(), left);
  Errors failure_errors;
  Tokenizer failure_tokens(domain, "> true"_view, "greater.ttx"_view);
  Cursor failure_cursor(failure_tokens, failure_errors);
  Token opening = failure_cursor.current();
  auto rejected = Operations::Greater::parse(
      domain, materializations, failure_cursor, Invalid::get_invalid(), left);

  ASSERT(parsed);
  EXPECT(parsed->is<Constants::True>());
  EXPECT(success_cursor.matches(Code::Type::Terminal));
  EXPECT(success_errors.is_empty());
  EXPECT_NOT(rejected);
  EXPECT(matches_token(failure_cursor, opening));
  ASSERT(failure_errors.get_size() == 1);
  View::Bytes rendered = failure_errors.render_message(rendering, 0);

  EXPECT(span_width(rendered) == Count(6));
}
