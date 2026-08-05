// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/modulo.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/true.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/types/bool.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/real_32.hpp"
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

static Harness LibraryModulo = {
  .name = "Tetrodotoxin::Library::Language::Operations::Modulo"_view,
};

class ModuloExpression : public Expression {
 public:
  ModuloExpression(View::Bytes name, const Abstract& type)
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

class ModuloUnresolvedType : public Ttx::Model::Type {
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

class ModuloFoldInput : public Operation {
 public:
  ModuloFoldInput(
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

template <typename constant_type, typename value_type>
static auto get_value(const Expression& expression) -> Option<value_type> {
  return expression.visit<constant_type>(
      [](const constant_type& constant) -> Option<value_type> {
        return constant.get_value();
      },
      [](const Abstract&) -> Option<value_type> { return {}; });
}

template <typename constant_type, typename value_type>
static auto value_is(const Expression& expression, value_type expected)
    -> Bool {
  auto value = get_value<constant_type, value_type>(expression);
  return value && *value == expected ? True : False;
}

static auto input_is(
    const Operations::Modulo& modulo,
    Count index,
    const Expression& expected) -> Bool {
  return modulo.get_inputs().get_abstract(index).visit(
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

PERIMORTEM_UNIT_TEST(LibraryModulo, type_selection_and_partial) {
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
  ModuloUnresolvedType unresolved_type;
  ModuloExpression signed_left("signed left"_view, signed_8);
  ModuloExpression signed_right("signed right"_view, signed_8);
  ModuloExpression unsigned_left("unsigned left"_view, unsigned_8);
  ModuloExpression unsigned_right("unsigned right"_view, unsigned_8);
  ModuloExpression other("other"_view, unsigned_16);
  ModuloExpression unresolved("unresolved"_view, unresolved_type);
  ModuloExpression invalid("invalid"_view, Invalid::get_invalid());
  Constants::Real real(real_32, 1.0);
  Constants::True truth(boolean);
  Constants::Bytes bytes(bytes_type, "x"_view);
  Operations::Modulo signed_exact(domain, signed_left, signed_right);
  Operations::Modulo unsigned_exact(domain, unsigned_left, unsigned_right);
  Operations::Modulo mismatch(domain, unsigned_left, other);
  Operations::Modulo unresolved_pair(domain, unresolved, unresolved);
  Operations::Modulo invalid_pair(domain, invalid, invalid);
  Operations::Modulo real_values(domain, real, real);
  Operations::Modulo flags(domain, truth, truth);
  Operations::Modulo byte_values(domain, bytes, bytes);
  auto retained =
      selected(unsigned_exact.attempt_fold(domain, materializations));

  EXPECT(&signed_exact.get_type() == &signed_8);
  EXPECT(&unsigned_exact.get_type() == &unsigned_8);
  EXPECT(retained && &*retained == &unsigned_exact);
  EXPECT(input_is(unsigned_exact, 0, unsigned_left));
  EXPECT(input_is(unsigned_exact, 1, unsigned_right));
  EXPECT(mismatch.get_type().resolve().is<Invalid>());
  EXPECT(unresolved_pair.get_type().resolve().is<Invalid>());
  EXPECT(invalid_pair.get_type().resolve().is<Invalid>());
  EXPECT(real_values.get_type().resolve().is<Invalid>());
  EXPECT(flags.get_type().resolve().is<Invalid>());
  EXPECT(byte_values.get_type().resolve().is<Invalid>());
}

PERIMORTEM_UNIT_TEST(LibraryModulo, integer_remainders) {
  Allocator::Arena domain;
  Materializations materializations(domain);
  Types::Signed_8 signed_type;
  Types::Unsigned_8 unsigned_type;
  Constants::Signed positive(signed_type, 7);
  Constants::Signed negative(signed_type, -7);
  Constants::Signed three(signed_type, 3);
  Constants::Signed negative_three(signed_type, -3);
  Constants::Signed zero(signed_type, 0);
  Constants::Signed one(signed_type, 1);
  Constants::Signed negative_one(signed_type, -1);
  Constants::Signed minimum(signed_type, -128);
  Constants::Signed invalid_left(signed_type, 255);
  Constants::Signed invalid_right(signed_type, 256);
  Constants::Unsigned seven(unsigned_type, 7);
  Constants::Unsigned unsigned_three(unsigned_type, 3);
  Constants::Unsigned unsigned_zero(unsigned_type, 0);
  Constants::Unsigned unsigned_one(unsigned_type, 1);
  Constants::Unsigned maximum(unsigned_type, 255);
  Constants::Unsigned invalid_unsigned_left(unsigned_type, 256);
  Constants::Unsigned invalid_unsigned_right(unsigned_type, 257);
  Operations::Modulo positive_result(domain, positive, three);
  Operations::Modulo negative_dividend(domain, negative, three);
  Operations::Modulo negative_divisor(domain, positive, negative_three);
  Operations::Modulo both_negative(domain, negative, negative_three);
  Operations::Modulo zero_result(domain, zero, one);
  Operations::Modulo one_result(domain, positive, one);
  Operations::Modulo minimum_result(domain, minimum, three);
  Operations::Modulo zero_divisor(domain, positive, zero);
  Operations::Modulo endpoint_overflow(domain, minimum, negative_one);
  Operations::Modulo signed_width(domain, invalid_left, invalid_right);
  Operations::Modulo unsigned_result(domain, seven, unsigned_three);
  Operations::Modulo unsigned_zero_result(domain, unsigned_zero, unsigned_one);
  Operations::Modulo unsigned_endpoint(domain, maximum, unsigned_three);
  Operations::Modulo unsigned_zero_divisor(domain, maximum, unsigned_zero);
  Operations::Modulo unsigned_width(
      domain, invalid_unsigned_left, invalid_unsigned_right);
  auto positive_fold =
      selected(positive_result.attempt_fold(domain, materializations));
  auto negative_fold =
      selected(negative_dividend.attempt_fold(domain, materializations));
  auto negative_divisor_fold =
      selected(negative_divisor.attempt_fold(domain, materializations));
  auto both_negative_fold =
      selected(both_negative.attempt_fold(domain, materializations));
  auto zero_fold = selected(zero_result.attempt_fold(domain, materializations));
  auto one_fold = selected(one_result.attempt_fold(domain, materializations));
  auto minimum_fold =
      selected(minimum_result.attempt_fold(domain, materializations));
  auto unsigned_fold =
      selected(unsigned_result.attempt_fold(domain, materializations));
  auto unsigned_zero_fold =
      selected(unsigned_zero_result.attempt_fold(domain, materializations));
  auto unsigned_endpoint_fold =
      selected(unsigned_endpoint.attempt_fold(domain, materializations));

  ASSERT(
      positive_fold && negative_fold && negative_divisor_fold &&
      both_negative_fold && zero_fold && one_fold && minimum_fold &&
      unsigned_fold && unsigned_zero_fold && unsigned_endpoint_fold);
  EXPECT(value_is<Constants::Signed>(*positive_fold, Signed_64(1)));
  EXPECT(value_is<Constants::Signed>(*negative_fold, Signed_64(-1)));
  EXPECT(value_is<Constants::Signed>(*negative_divisor_fold, Signed_64(1)));
  EXPECT(value_is<Constants::Signed>(*both_negative_fold, Signed_64(-1)));
  EXPECT(value_is<Constants::Signed>(*zero_fold, Signed_64(0)));
  EXPECT(value_is<Constants::Signed>(*one_fold, Signed_64(0)));
  EXPECT(value_is<Constants::Signed>(*minimum_fold, Signed_64(-2)));
  EXPECT(value_is<Constants::Unsigned>(*unsigned_fold, Unsigned_64(1)));
  EXPECT(value_is<Constants::Unsigned>(*unsigned_zero_fold, Unsigned_64(0)));
  EXPECT(
      value_is<Constants::Unsigned>(*unsigned_endpoint_fold, Unsigned_64(0)));
  EXPECT(&positive_fold->get_type() == &signed_type);
  EXPECT(&unsigned_fold->get_type() == &unsigned_type);
  EXPECT(reports(
      zero_divisor.attempt_fold(domain, materializations),
      FoldError::Type::DivisionByZero, zero_divisor));
  EXPECT(reports(
      unsigned_zero_divisor.attempt_fold(domain, materializations),
      FoldError::Type::DivisionByZero, unsigned_zero_divisor));
  EXPECT(reports(
      endpoint_overflow.attempt_fold(domain, materializations),
      FoldError::Type::ArithmeticOverflow, endpoint_overflow));
  EXPECT(reports(
      signed_width.attempt_fold(domain, materializations),
      FoldError::Type::ArithmeticOverflow, signed_width));
  EXPECT(reports(
      unsigned_width.attempt_fold(domain, materializations),
      FoldError::Type::ArithmeticOverflow, unsigned_width));
}

PERIMORTEM_UNIT_TEST(LibraryModulo, recursive_provenance_and_atomicity) {
  Allocator::Arena domain;
  Allocator::Arena rendering;
  Materializations materializations(domain);
  Types::Unsigned_8 selected_type;
  Constants::Unsigned input(selected_type, 1);
  Constants::Unsigned folded(selected_type, 13);
  Constants::Unsigned divisor(selected_type, 5);
  ModuloFoldInput child(domain, input, folded, selected_type);
  ModuloFoldInput failing(domain, input, folded, selected_type, True);
  Operations::Modulo modulo(domain, child, divisor);
  Operations::Modulo failure(domain, failing, divisor);
  auto first = selected(modulo.attempt_fold(domain, materializations));
  auto second = selected(modulo.attempt_fold(domain, materializations));

  ASSERT(first && second);
  EXPECT(&*first == &*second);
  EXPECT(value_is<Constants::Unsigned>(*first, Unsigned_64(3)));
  EXPECT(input_is(modulo, 0, folded));
  EXPECT(child.get_evaluations() == 1);
  EXPECT(reports(
      failure.attempt_fold(domain, materializations),
      FoldError::Type::InvalidConstant, failing));

  const auto& parser_type = Tetrodotoxin::Library::Dialect::get_signed_64();
  Constants::Signed left(parser_type, -7);
  Errors success_errors;
  Tokenizer success_tokens(domain, "% -3"_view, "modulo.ttx"_view);
  Cursor success_cursor(success_tokens, success_errors);
  auto parsed = Operations::Modulo::parse(
      domain, materializations, success_cursor, Invalid::get_invalid(), left);
  auto parsed_value = parsed ? get_value<Constants::Signed, Signed_64>(*parsed)
                             : Option<Signed_64>();
  Errors failure_errors;
  Tokenizer failure_tokens(domain, "% true"_view, "modulo.ttx"_view);
  Cursor failure_cursor(failure_tokens, failure_errors);
  Token opening = failure_cursor.current();
  auto rejected = Operations::Modulo::parse(
      domain, materializations, failure_cursor, Invalid::get_invalid(), left);

  ASSERT(parsed && parsed_value);
  EXPECT(*parsed_value == -1);
  EXPECT(success_cursor.matches(Code::Type::Terminal));
  EXPECT(success_errors.is_empty());
  EXPECT_NOT(rejected);
  EXPECT(matches_token(failure_cursor, opening));
  ASSERT(failure_errors.get_size() == 1);
  View::Bytes rendered = failure_errors.render_message(rendering, 0);

  EXPECT(span_width(rendered) == Count(6));
}
