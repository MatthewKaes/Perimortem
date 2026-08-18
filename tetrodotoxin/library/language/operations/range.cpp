// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/range.hpp"

#include "tetrodotoxin/library/language/generics/range.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;

auto Language::Operations::Range::parse(
    const Abstract& context,
    Cursor& cursor,
    Model::Pack& left,
    Span left_span) -> Core::Option<Expression&> {
  Token opening = cursor.consume();
  Token right_start = cursor.current();
  auto right = Language::Parser::Expression::parse_operand(
      context, cursor, Code::Type::RangeOp);
  Span span(opening, cursor.peek(-1));
  if (!right) {
    cursor.create_expression_error(
        span, "Range has a malformed right endpoint."_view,
        "Use one complete integer Expression after `...`."_view);
    return {};
  }
  if (cursor.matches(Code::Type::RangeOp)) {
    cursor.create_expression_error(
        Span(opening, cursor.current()),
        "Range accepts exactly two endpoints."_view,
        "Finish one Range before starting another Expression."_view);
    return {};
  }

  auto left_expression = left.select<Expression>();
  auto right_expression = right->select<Expression>();
  Span right_span(right_start, cursor.peek(-1));
  if (!left_expression || !right_expression) {
    cursor.create_expression_error(
        Anchor::create(opening, left_span, right_span),
        "Library Range requires one Expression from each endpoint Pack."_view,
        "Use one unlabelled integer value for each Range endpoint."_view);
    return {};
  }

  Anchor anchor = Anchor::create(opening, left_span, right_span);
  Range& range = create_authored(
      cursor.get_arena(), *left_expression, *right_expression, anchor);
  return range;
}

TTX_BINARY_OP(Range);

auto Language::Operations::Range::lower(Llvm::Builder& body) const -> Bool {
  auto folded = lower_folded(body);
  if (folded) {
    return *folded;
  }

  auto inputs = get_inputs();
  const Expression& left = inputs.get_data()[0].get();
  const Expression& right = inputs.get_data()[1].get();
  auto carrier = get_type().resolve().select<Language::Model::Type>();

  if (!carrier) {
    return False;
  }

  Bool carrier_ready = carrier->reserve(body.get_program()) &&
                       carrier->complete(body.get_program());
  if (!carrier_ready) {
    return False;
  }

  Bool lowered = lower_inputs(body);
  if (!lowered) {
    return False;
  }

  return body.range(*carrier, *this, left, right);
}

auto Language::Operations::Range::select_type(
    const Ttx::Concept::Abstract& context) const
    -> Core::Option<const Language::Model::Type&> {
  auto inputs = get_inputs();
  const Expression& left = inputs.get_data()[0].get();
  const Expression& right = inputs.get_data()[1].get();
  const Abstract& left_type = left.get_type().resolve();
  const Abstract& right_type = right.get_type().resolve();
  auto element = left_type.select<Language::Model::Type>();
  if (!element || &left_type != &right_type ||
      (!left_type.is<Tetrodotoxin::Library::Language::Model::Types::Signed>() &&
       !left_type
            .is<Tetrodotoxin::Library::Language::Model::Types::Unsigned>())) {
    return {};
  }

  Core::Static::Vector<Generic::Argument, 1> arguments = {{
    Generic::Argument(*element),
  }};
  const Abstract& selected = context.resolve_context(Generics::Range::name);
  auto generic = selected.select<Generic>();
  BAIL_IF(!generic);
  return generic->materialize(arguments.get_view())
      .visit(
          [](const Language::Model::Type& type)
              -> Core::Option<const Language::Model::Type&> { return type; },
          [](const Generic::Failure&)
              -> Core::Option<const Language::Model::Type&> { return {}; });
}

auto Language::Operations::Range::evaluate_constants(Memory::Allocator::Arena&)
    -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
  return Core::Option<Constant&>{};
}
