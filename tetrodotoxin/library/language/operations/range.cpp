// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/range.hpp"

#include "tetrodotoxin/library/language/generics/range.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/types/signed.hpp"
#include "ttx/model/types/unsigned.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;

auto Language::Operations::Range::parse(
    Memory::Allocator::Arena& domain,
    Monograph& source,
    Cursor& cursor,
    Model::Pack& left,
    Span left_span) -> Core::Option<Expression&> {
  auto transaction = cursor.branch();
  Token opening = transaction.consume();
  Token right_start = transaction.current();
  auto right = Language::Parser::Expression::parse_operand(
      domain, source, transaction, Code::Type::RangeOp);
  Span span(opening, transaction.peek(-1));
  if (!right) {
    transaction.create_expression_error(
        span, "Range has a malformed right endpoint."_view,
        "Use one complete integer Expression after `...`."_view);
    return {};
  }
  if (transaction.matches(Code::Type::RangeOp)) {
    transaction.create_expression_error(
        Span(opening, transaction.current()),
        "Range accepts exactly two endpoints."_view,
        "Finish one Range before starting another Expression."_view);
    return {};
  }

  auto left_expression = left.select<Expression>();
  auto right_expression = right->select<Expression>();
  Span right_span(right_start, transaction.peek(-1));
  if (!left_expression || !right_expression) {
    transaction.create_expression_error(
        Anchor::create(opening, left_span, right_span),
        "Library Range requires one Expression from each endpoint Pack."_view,
        "Use one unlabelled integer value for each Range endpoint."_view);
    return {};
  }

  Anchor anchor = Anchor::create(opening, left_span, right_span);
  Range& range =
      create_authored(domain, *left_expression, *right_expression, anchor);
  cursor.join(transaction);
  return range;
}

TTX_BINARY_OP(Range);

auto Language::Operations::Range::select_type(
    Tetrodotoxin::Language::Monograph& source) const
    -> Core::Option<const Type&> {
  auto left = get_input(0);
  auto right = get_input(1);
  if (!left || !right) {
    return {};
  }

  const Abstract& left_type = left->get_type().resolve();
  const Abstract& right_type = right->get_type().resolve();
  auto element = left_type.select<Type>();
  if (!element || &left_type != &right_type ||
      (!left_type.is<Ttx::Model::Types::Signed>() &&
       !left_type.is<Ttx::Model::Types::Unsigned>())) {
    return {};
  }

  Core::Static::Vector<Generic::Argument, 1> arguments = {{
    Generic::Argument(*element),
  }};
  // Range construction consumes the graph transaction hosted by the concrete
  // Library source. Parser and Operation retain no second capability edge.
  auto& library_source = static_cast<Language::Monograph&>(source);
  return library_source.get_materializations().materialize(
      Generics::Range::get_formula(), arguments.get_view());
}

auto Language::Operations::Range::evaluate_constants(Memory::Allocator::Arena&)
    -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
  return Core::Option<Constant&>{};
}
