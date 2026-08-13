// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/modulo.hpp"

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
#include "tetrodotoxin/library/language/types/signed_8.hpp"
#include "tetrodotoxin/library/language/types/unsigned_16.hpp"
#include "tetrodotoxin/library/language/types/unsigned_8.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

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

class ModuloMonograph : public Tetrodotoxin::Language::Monograph {
 public:
  ModuloMonograph(Allocator::Arena& domain)
      : Tetrodotoxin::Language::Monograph(domain, Documentation::get_empty()) {}

  constexpr auto get_name() const -> View::Bytes override {
    return "ModuloMonograph"_view;
  }

  constexpr auto resolve_context(View::Bytes) const
      -> const Abstract& override {
    return Invalid::get_invalid();
  }
};

static auto link_operation(
    Operation& operation,
    Tetrodotoxin::Language::Monograph& source) -> Bool {
  return operation.link(source, Invalid::get_invalid());
}

class ModuloExpression : public Expression {
 public:
  ModuloExpression(View::Bytes name, const Abstract& type)
      : Expression({}), name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Abstract& override { return type; }

 private:
  View::Bytes name;
  const Abstract& type;
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

  auto get_name() const -> View::Bytes override { return "Fold input"_view; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_evaluations() const -> Count { return evaluations; }

 protected:
  auto evaluate_constants(Allocator::Arena&)
      -> Result<Option<Constant&>, Expression::Error> override {
    evaluations++;
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
  Count evaluations = 0;
};

static auto selected(
    const Result<Option<Model::Pack&>, Expression::Error>& result)
    -> Option<Expression&> {
  return result.visit(
      [](const Option<Model::Pack&>& folded) -> Option<Expression&> {
        return folded.visit(
            []() -> Option<Expression&> { return {}; },
            [](Model::Pack& selected) -> Option<Expression&> {
              return selected.select<Expression>();
            });
      },
      [](const Expression::Error&) -> Option<Expression&> { return {}; });
}

static auto reports(
    const Result<Option<Model::Pack&>, Expression::Error>& result,
    Expression::Error::Type expected,
    const Expression& origin) -> Bool {
  return result.visit(
      [](const Option<Model::Pack&>&) { return False; },
      [&](const Expression::Error& error) {
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

PERIMORTEM_UNIT_TEST(LibraryModulo, type_selection_and_partial) {
  Allocator::Arena domain;
  ModuloMonograph source(domain);
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
  auto& real = Constants::Real::create_synthetic(domain, real_32, 1.0);
  auto& truth = Constants::True::create_synthetic(domain, boolean);
  auto& bytes =
      Constants::Bytes::create_synthetic(domain, bytes_type, "x"_view);
  auto& signed_exact =
      Operations::Modulo::create_synthetic(domain, signed_left, signed_right);
  auto& unsigned_exact = Operations::Modulo::create_synthetic(
      domain, unsigned_left, unsigned_right);
  auto& mismatch =
      Operations::Modulo::create_synthetic(domain, unsigned_left, other);
  auto& unresolved_pair =
      Operations::Modulo::create_synthetic(domain, unresolved, unresolved);
  auto& invalid_pair =
      Operations::Modulo::create_synthetic(domain, invalid, invalid);
  auto& real_values = Operations::Modulo::create_synthetic(domain, real, real);
  auto& flags = Operations::Modulo::create_synthetic(domain, truth, truth);
  auto& byte_values =
      Operations::Modulo::create_synthetic(domain, bytes, bytes);

  EXPECT(signed_exact.get_type().resolve().is<Invalid>());
  EXPECT_NOT(signed_exact.get_anchor());
  EXPECT(link_operation(signed_exact, source));
  EXPECT(link_operation(unsigned_exact, source));
  EXPECT(!link_operation(mismatch, source));
  EXPECT(!link_operation(unresolved_pair, source));
  EXPECT(!link_operation(invalid_pair, source));
  EXPECT(!link_operation(real_values, source));
  EXPECT(!link_operation(flags, source));
  EXPECT(!link_operation(byte_values, source));

  auto retained = selected(unsigned_exact.fold());

  EXPECT(&signed_exact.get_type() == &signed_8);
  EXPECT(&unsigned_exact.get_type() == &unsigned_8);
  EXPECT_NOT(retained);
  EXPECT(mismatch.get_type().resolve().is<Invalid>());
  EXPECT(unresolved_pair.get_type().resolve().is<Invalid>());
  EXPECT(invalid_pair.get_type().resolve().is<Invalid>());
  EXPECT(real_values.get_type().resolve().is<Invalid>());
  EXPECT(flags.get_type().resolve().is<Invalid>());
  EXPECT(byte_values.get_type().resolve().is<Invalid>());
}

PERIMORTEM_UNIT_TEST(LibraryModulo, integer_remainders) {
  Allocator::Arena domain;
  ModuloMonograph source(domain);
  Types::Signed_8 signed_type;
  Types::Unsigned_8 unsigned_type;
  auto& positive = Constants::Signed::create_synthetic(domain, signed_type, 7);
  auto& negative = Constants::Signed::create_synthetic(domain, signed_type, -7);
  auto& three = Constants::Signed::create_synthetic(domain, signed_type, 3);
  auto& negative_three =
      Constants::Signed::create_synthetic(domain, signed_type, -3);
  auto& zero = Constants::Signed::create_synthetic(domain, signed_type, 0);
  auto& one = Constants::Signed::create_synthetic(domain, signed_type, 1);
  auto& negative_one =
      Constants::Signed::create_synthetic(domain, signed_type, -1);
  auto& minimum =
      Constants::Signed::create_synthetic(domain, signed_type, -128);
  auto& invalid_left =
      Constants::Signed::create_synthetic(domain, signed_type, 255);
  auto& invalid_right =
      Constants::Signed::create_synthetic(domain, signed_type, 256);
  auto& seven = Constants::Unsigned::create_synthetic(domain, unsigned_type, 7);
  auto& unsigned_three =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 3);
  auto& unsigned_zero =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 0);
  auto& unsigned_one =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 1);
  auto& maximum =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 255);
  auto& invalid_unsigned_left =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 256);
  auto& invalid_unsigned_right =
      Constants::Unsigned::create_synthetic(domain, unsigned_type, 257);
  auto& positive_result =
      Operations::Modulo::create_synthetic(domain, positive, three);
  auto& negative_dividend =
      Operations::Modulo::create_synthetic(domain, negative, three);
  auto& negative_divisor =
      Operations::Modulo::create_synthetic(domain, positive, negative_three);
  auto& both_negative =
      Operations::Modulo::create_synthetic(domain, negative, negative_three);
  auto& zero_result = Operations::Modulo::create_synthetic(domain, zero, one);
  auto& one_result =
      Operations::Modulo::create_synthetic(domain, positive, one);
  auto& minimum_result =
      Operations::Modulo::create_synthetic(domain, minimum, three);
  auto& zero_divisor =
      Operations::Modulo::create_synthetic(domain, positive, zero);
  auto& endpoint_overflow =
      Operations::Modulo::create_synthetic(domain, minimum, negative_one);
  auto& signed_width =
      Operations::Modulo::create_synthetic(domain, invalid_left, invalid_right);
  auto& unsigned_result =
      Operations::Modulo::create_synthetic(domain, seven, unsigned_three);
  auto& unsigned_zero_result =
      Operations::Modulo::create_synthetic(domain, unsigned_zero, unsigned_one);
  auto& unsigned_endpoint =
      Operations::Modulo::create_synthetic(domain, maximum, unsigned_three);
  auto& unsigned_zero_divisor =
      Operations::Modulo::create_synthetic(domain, maximum, unsigned_zero);
  auto& unsigned_width = Operations::Modulo::create_synthetic(
      domain, invalid_unsigned_left, invalid_unsigned_right);

  EXPECT(positive_result.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(positive_result, source));
  EXPECT(link_operation(negative_dividend, source));
  EXPECT(link_operation(negative_divisor, source));
  EXPECT(link_operation(both_negative, source));
  EXPECT(link_operation(zero_result, source));
  EXPECT(link_operation(one_result, source));
  EXPECT(link_operation(minimum_result, source));
  EXPECT(link_operation(zero_divisor, source));
  EXPECT(link_operation(endpoint_overflow, source));
  EXPECT(link_operation(signed_width, source));
  EXPECT(link_operation(unsigned_result, source));
  EXPECT(link_operation(unsigned_zero_result, source));
  EXPECT(link_operation(unsigned_endpoint, source));
  EXPECT(link_operation(unsigned_zero_divisor, source));
  EXPECT(link_operation(unsigned_width, source));

  auto positive_fold = selected(positive_result.fold());
  auto negative_fold = selected(negative_dividend.fold());
  auto negative_divisor_fold = selected(negative_divisor.fold());
  auto both_negative_fold = selected(both_negative.fold());
  auto zero_fold = selected(zero_result.fold());
  auto one_fold = selected(one_result.fold());
  auto minimum_fold = selected(minimum_result.fold());
  auto unsigned_fold = selected(unsigned_result.fold());
  auto unsigned_zero_fold = selected(unsigned_zero_result.fold());
  auto unsigned_endpoint_fold = selected(unsigned_endpoint.fold());

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
      zero_divisor.fold(), Expression::Error::Type::DivisionByZero,
      zero_divisor));
  EXPECT(reports(
      unsigned_zero_divisor.fold(), Expression::Error::Type::DivisionByZero,
      unsigned_zero_divisor));
  EXPECT(reports(
      endpoint_overflow.fold(), Expression::Error::Type::ArithmeticOverflow,
      endpoint_overflow));
  EXPECT(reports(
      signed_width.fold(), Expression::Error::Type::ArithmeticOverflow,
      signed_width));
  EXPECT(reports(
      unsigned_width.fold(), Expression::Error::Type::ArithmeticOverflow,
      unsigned_width));
}

PERIMORTEM_UNIT_TEST(LibraryModulo, recursive_provenance_and_atomicity) {
  Allocator::Arena domain;
  ModuloMonograph context(domain);
  Tetrodotoxin::Library::Dialect dialect;
  Errors host_errors;
  Tokenizer host_tokens(domain, {}, "modulo-source.ttx"_view);
  Cursor host_cursor(host_tokens, host_errors);
  auto retained_source = dialect.interpret(
      domain, host_cursor, Documentation::get_empty(), Anchor::create(Span()),
      context);
  ASSERT(retained_source && retained_source->is<Monograph>());
  auto& source = static_cast<Monograph&>(*retained_source);
  Types::Unsigned_8 selected_type;
  auto& input = Constants::Unsigned::create_synthetic(domain, selected_type, 1);
  auto& folded =
      Constants::Unsigned::create_synthetic(domain, selected_type, 13);
  auto& divisor =
      Constants::Unsigned::create_synthetic(domain, selected_type, 5);
  ModuloFoldInput child(domain, input, folded, selected_type);
  ModuloFoldInput failing(domain, input, folded, selected_type, True);
  auto& modulo = Operations::Modulo::create_synthetic(domain, child, divisor);
  auto& failure =
      Operations::Modulo::create_synthetic(domain, failing, divisor);

  EXPECT(modulo.get_type().resolve().is<Invalid>());
  EXPECT(link_operation(modulo, source));
  EXPECT(link_operation(modulo, source));
  EXPECT(link_operation(failure, source));

  auto first = selected(modulo.fold());
  auto second = selected(modulo.fold());

  ASSERT(first && second);
  EXPECT(&*first == &*second);
  EXPECT(value_is<Constants::Unsigned>(*first, Unsigned_64(3)));
  EXPECT(child.get_evaluations() == 1);
  EXPECT(reports(
      failure.fold(), Expression::Error::Type::InvalidConstant, failing));

  const auto& parser_type = Tetrodotoxin::Library::Dialect::get_signed_64();
  Errors success_errors;
  Tokenizer success_tokens(domain, "-7 % -3"_view, "modulo.ttx"_view);
  Cursor success_cursor(success_tokens, success_errors);
  Token success_left_trigger = success_cursor.consume();
  Token success_left_end = success_cursor.consume();
  auto success_left_anchor = Anchor::create(
      success_left_trigger, Span(success_left_trigger, success_left_end));
  auto& success_left = Constants::Signed::create_authored(
      domain, parser_type, -7, success_left_anchor);
  auto parsed = Operations::Modulo::parse(
      domain, source, success_cursor, success_left,
      Span(success_left_trigger, success_left_end));
  Errors failure_errors;
  Tokenizer failure_tokens(domain, "-7 % true"_view, "modulo.ttx"_view);
  Cursor failure_cursor(failure_tokens, failure_errors);
  Token failure_left_trigger = failure_cursor.consume();
  Token failure_left_end = failure_cursor.consume();
  auto failure_left_anchor = Anchor::create(
      failure_left_trigger, Span(failure_left_trigger, failure_left_end));
  auto& failure_left = Constants::Signed::create_authored(
      domain, parser_type, -7, failure_left_anchor);
  auto rejected = Operations::Modulo::parse(
      domain, source, failure_cursor, failure_left,
      Span(failure_left_trigger, failure_left_end));

  ASSERT(parsed);
  EXPECT(parsed->is<Operations::Modulo>());
  EXPECT(parsed->get_type().resolve().is<Invalid>());
  EXPECT(success_cursor.matches(Code::Type::Terminal));
  EXPECT(success_errors.is_empty());
  EXPECT(parsed->link(source, Invalid::get_invalid()));

  auto parsed_fold = parsed->visit<Operation>(
      [&](Operation& operation) { return selected(operation.fold()); },
      [](Abstract&) -> Option<Expression&> { return {}; });
  auto parsed_value =
      parsed_fold ? get_value<Constants::Signed, Signed_64>(*parsed_fold)
                  : Option<Signed_64>();

  ASSERT(parsed_value);
  EXPECT(*parsed_value == -1);
  EXPECT(&parsed->get_type() == &parser_type);

  ASSERT(rejected);
  EXPECT(rejected->is<Operations::Modulo>());
  EXPECT(rejected->get_type().resolve().is<Invalid>());
  EXPECT(failure_cursor.matches(Code::Type::Terminal));
  EXPECT(failure_errors.is_empty());
  EXPECT_NOT(rejected->link(source, Invalid::get_invalid()));
  EXPECT(rejected->get_type().resolve().is<Invalid>());

  auto diagnostics = source.get_diagnostics();
  ASSERT(diagnostics.get_size() == 1);
  ASSERT(diagnostics.get_data()[0].get_anchor());
  EXPECT(
      diagnostics.get_data()[0].get_anchor()->get_span().get_size() ==
      Count(9));
}
