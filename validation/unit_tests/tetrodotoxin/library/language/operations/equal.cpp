// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/equal.hpp"

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

static Harness LibraryEqual = {
  .name = "Tetrodotoxin::Library::Language::Operations::Equal"_view,
};

class EqualExpression : public Expression {
 public:
  EqualExpression(View::Bytes name, const Abstract& type)
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

class EqualUnresolvedType : public Ttx::Model::Type {
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

class EqualFoldInput : public Operation {
 public:
  EqualFoldInput(
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
    const Operations::Equal& equal,
    Count index,
    const Expression& expected) -> Bool {
  return equal.get_inputs().get_abstract(index).visit(
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

PERIMORTEM_UNIT_TEST(LibraryEqual, type_selection_and_partial) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Signed_8 signed_8;
  Types::Unsigned_8 unsigned_8;
  Types::Unsigned_16 unsigned_16;
  Types::Real_64 real_64;
  Types::Boolean boolean;
  Types::Fixed bytes_type(
      "Fixed[Unsigned_8,1]"_view,
      Tetrodotoxin::Library::Dialect::get_unsigned_8(), 1);
  Types::Fixed other_bytes_type(
      "Fixed[Unsigned_8,2]"_view,
      Tetrodotoxin::Library::Dialect::get_unsigned_8(), 2);
  EqualUnresolvedType unresolved_type;
  EqualExpression signed_left("signed left"_view, signed_8);
  EqualExpression signed_right("signed right"_view, signed_8);
  EqualExpression unsigned_left("unsigned left"_view, unsigned_8);
  EqualExpression unsigned_right("unsigned right"_view, unsigned_8);
  EqualExpression real_left("real left"_view, real_64);
  EqualExpression real_right("real right"_view, real_64);
  EqualExpression flag_left("flag left"_view, boolean);
  EqualExpression flag_right("flag right"_view, boolean);
  EqualExpression other("other"_view, unsigned_16);
  EqualExpression unresolved("unresolved"_view, unresolved_type);
  EqualExpression invalid("invalid"_view, Invalid::get_invalid());
  EqualExpression dynamic_bytes("dynamic bytes"_view, bytes_type);
  Constants::Bytes bytes(bytes_type, "x"_view);
  Constants::Bytes same_bytes(bytes_type, "x"_view);
  Constants::Bytes other_bytes(other_bytes_type, "xx"_view);
  Operations::Equal signed_exact(domain, signed_left, signed_right);
  Operations::Equal unsigned_exact(domain, unsigned_left, unsigned_right);
  Operations::Equal real_exact(domain, real_left, real_right);
  Operations::Equal flag_exact(domain, flag_left, flag_right);
  Operations::Equal byte_values(domain, bytes, same_bytes);
  Operations::Equal mismatch(domain, unsigned_left, other);
  Operations::Equal unresolved_pair(domain, unresolved, unresolved);
  Operations::Equal invalid_pair(domain, invalid, invalid);
  Operations::Equal incomplete_bytes(domain, dynamic_bytes, bytes);
  Operations::Equal byte_mismatch(domain, bytes, other_bytes);
  auto retained =
      selected(unsigned_exact.attempt_fold(domain, materializations));

  EXPECT(
      &signed_exact.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(
      &unsigned_exact.get_type() ==
      &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(&real_exact.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(&flag_exact.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(
      &byte_values.get_type() == &Tetrodotoxin::Library::Dialect::get_bool());
  EXPECT(retained && &*retained == &unsigned_exact);
  EXPECT(input_is(unsigned_exact, 0, unsigned_left));
  EXPECT(input_is(unsigned_exact, 1, unsigned_right));
  EXPECT(mismatch.get_type().resolve().is<Invalid>());
  EXPECT(unresolved_pair.get_type().resolve().is<Invalid>());
  EXPECT(invalid_pair.get_type().resolve().is<Invalid>());
  EXPECT(incomplete_bytes.get_type().resolve().is<Invalid>());
  EXPECT(byte_mismatch.get_type().resolve().is<Invalid>());
}

PERIMORTEM_UNIT_TEST(LibraryEqual, complete_constant_domains) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Signed_8 signed_type;
  Types::Unsigned_8 unsigned_type;
  Types::Real_64 real_type;
  Types::Boolean boolean;
  Types::Fixed bytes_type(
      "Fixed[Unsigned_8,2]"_view,
      Tetrodotoxin::Library::Dialect::get_unsigned_8(), 2);
  Constants::Signed signed_value(signed_type, -8);
  Constants::Signed same_signed(signed_type, -8);
  Constants::Signed other_signed(signed_type, 8);
  Constants::Unsigned unsigned_value(unsigned_type, 8);
  Constants::Unsigned same_unsigned(unsigned_type, 8);
  Constants::Unsigned other_unsigned(unsigned_type, 9);
  Constants::Real real_value(real_type, 0.5);
  Constants::Real same_real(real_type, 0.5);
  Constants::Real other_real(real_type, 1.0);
  Constants::True truth(boolean);
  Constants::True same_truth(boolean);
  Constants::False falsity(boolean);
  Constants::Bytes bytes(bytes_type, "ab"_view);
  Constants::Bytes same_bytes(bytes_type, "ab"_view);
  Constants::Bytes other_bytes(bytes_type, "ac"_view);
  Operations::Equal signed_equal(domain, signed_value, same_signed);
  Operations::Equal signed_different(domain, signed_value, other_signed);
  Operations::Equal unsigned_equal(domain, unsigned_value, same_unsigned);
  Operations::Equal unsigned_different(domain, unsigned_value, other_unsigned);
  Operations::Equal real_equal(domain, real_value, same_real);
  Operations::Equal real_different(domain, real_value, other_real);
  Operations::Equal flag_equal(domain, truth, same_truth);
  Operations::Equal flag_different(domain, truth, falsity);
  Operations::Equal bytes_equal(domain, bytes, same_bytes);
  Operations::Equal bytes_different(domain, bytes, other_bytes);
  auto signed_yes =
      selected(signed_equal.attempt_fold(domain, materializations));
  auto signed_no =
      selected(signed_different.attempt_fold(domain, materializations));
  auto unsigned_yes =
      selected(unsigned_equal.attempt_fold(domain, materializations));
  auto unsigned_no =
      selected(unsigned_different.attempt_fold(domain, materializations));
  auto real_yes = selected(real_equal.attempt_fold(domain, materializations));
  auto real_no =
      selected(real_different.attempt_fold(domain, materializations));
  auto flag_yes = selected(flag_equal.attempt_fold(domain, materializations));
  auto flag_no =
      selected(flag_different.attempt_fold(domain, materializations));
  auto bytes_yes = selected(bytes_equal.attempt_fold(domain, materializations));
  auto bytes_no =
      selected(bytes_different.attempt_fold(domain, materializations));

  ASSERT(
      signed_yes && signed_no && unsigned_yes && unsigned_no && real_yes &&
      real_no && flag_yes && flag_no && bytes_yes && bytes_no);
  EXPECT(signed_yes->is<Constants::True>());
  EXPECT(signed_no->is<Constants::False>());
  EXPECT(unsigned_yes->is<Constants::True>());
  EXPECT(unsigned_no->is<Constants::False>());
  EXPECT(real_yes->is<Constants::True>());
  EXPECT(real_no->is<Constants::False>());
  EXPECT(flag_yes->is<Constants::True>());
  EXPECT(flag_no->is<Constants::False>());
  EXPECT(bytes_yes->is<Constants::True>());
  EXPECT(bytes_no->is<Constants::False>());
}

PERIMORTEM_UNIT_TEST(LibraryEqual, real_equivalence) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Real_64 real_type;
  Constants::Real left_nan(real_type, __builtin_nan("left"));
  Constants::Real right_nan(real_type, __builtin_nan("right"));
  Constants::Real finite(real_type, 1.0);
  Constants::Real positive_zero(real_type, 0.0);
  Constants::Real negative_zero(real_type, -0.0);
  Operations::Equal nan_pair(domain, left_nan, right_nan);
  Operations::Equal nan_finite(domain, left_nan, finite);
  Operations::Equal signed_zero(domain, positive_zero, negative_zero);
  auto nan_equal = selected(nan_pair.attempt_fold(domain, materializations));
  auto nan_other = selected(nan_finite.attempt_fold(domain, materializations));
  auto zeros = selected(signed_zero.attempt_fold(domain, materializations));

  ASSERT(nan_equal && nan_other && zeros);
  EXPECT(nan_equal->is<Constants::True>());
  EXPECT(nan_other->is<Constants::False>());
  EXPECT(zeros->is<Constants::True>());
}

PERIMORTEM_UNIT_TEST(LibraryEqual, recursive_provenance_and_atomicity) {
  Allocator::Arena domain;
  Allocator::Arena rendering;
  Materializations materializations(domain);
  Types::Unsigned_8 selected_type;
  Constants::Unsigned input(selected_type, 1);
  Constants::Unsigned folded(selected_type, 8);
  Constants::Unsigned right(selected_type, 8);
  Constants::Bytes wrong_domain(selected_type, "x"_view);
  EqualFoldInput child(domain, input, folded, selected_type);
  EqualFoldInput failing(domain, input, folded, selected_type, True);
  EqualFoldInput invalid_child(domain, input, wrong_domain, selected_type);
  Operations::Equal equal(domain, child, right);
  Operations::Equal failure(domain, failing, right);
  Operations::Equal invalid_constant(domain, invalid_child, right);
  auto first = selected(equal.attempt_fold(domain, materializations));
  auto second = selected(equal.attempt_fold(domain, materializations));

  ASSERT(first && second);
  EXPECT(&*first == &*second);
  EXPECT(first->is<Constants::True>());
  EXPECT(input_is(equal, 0, folded));
  EXPECT(child.get_evaluations() == 1);
  EXPECT(reports(
      failure.attempt_fold(domain, materializations),
      FoldError::Type::InvalidConstant, failing));
  EXPECT(reports(
      invalid_constant.attempt_fold(domain, materializations),
      FoldError::Type::InvalidConstant, wrong_domain));

  const auto& parser_type = Tetrodotoxin::Library::Dialect::get_unsigned_64();
  Constants::Unsigned left(parser_type, 2);
  Errors success_errors;
  Tokenizer success_tokens(domain, "== 2"_view, "equal.ttx"_view);
  Cursor success_cursor(success_tokens, success_errors);
  auto parsed = Operations::Equal::parse(
      domain, materializations, success_cursor, Invalid::get_invalid(), left);
  Errors failure_errors;
  Tokenizer failure_tokens(domain, "== true"_view, "equal.ttx"_view);
  Cursor failure_cursor(failure_tokens, failure_errors);
  Token opening = failure_cursor.current();
  auto rejected = Operations::Equal::parse(
      domain, materializations, failure_cursor, Invalid::get_invalid(), left);

  ASSERT(parsed);
  EXPECT(parsed->is<Constants::True>());
  EXPECT(success_cursor.matches(Code::Type::Terminal));
  EXPECT(success_errors.is_empty());
  EXPECT_NOT(rejected);
  EXPECT(matches_token(failure_cursor, opening));
  ASSERT(failure_errors.get_size() == 1);
  View::Bytes rendered = failure_errors.render_message(rendering, 0);

  EXPECT(span_width(rendered) == Count(7));
}
