// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/operations/slice.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/generics/access.hpp"
#include "tetrodotoxin/library/language/generics/fixed.hpp"
#include "tetrodotoxin/library/language/generics/view.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/types/access.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/model/types/signed.hpp"
#include "ttx/model/types/unsigned.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;

static constexpr Unsigned_64 maximum_extent = Unsigned_64(-1) >> 1;

static auto complete_postfix_span(Cursor& cursor, Token opening) -> Span {
  // A malformed tail still belongs to one Slice. Consume only its local
  // closing bracket so the diagnostic covers the authored operation.
  while (!cursor.matches(Code::Type::Terminal) &&
         !cursor.matches(Code::Type::LayoutEnd)) {
    cursor.consume();
  }

  if (cursor.matches(Code::Type::LayoutEnd)) {
    cursor.consume();
  }

  return Span(opening, cursor.peek(-1));
}

static auto reject_syntax(Cursor& cursor, Span span) -> void {
  cursor.create_expression_error(
      span, "Slice has malformed index or range operands."_view,
      "Use `:[index]` or `:[start, size]` with complete delimiters."_view);
}

static auto reject_operand(Cursor& cursor, Span postfix_span, Span operand_span)
    -> void {
  auto report = cursor.create_report(postfix_span);
  report << "Slice operand `"_view
         << operand_span.caculate_text(cursor.get_source_text())
         << "` could not be parsed as a complete Expression."_view;
  report.get_hint() << "Use a complete scalar or byte Expression."_view;
}

static auto get_element_type(const Language::Expression& receiver)
    -> Utility::Option<const Type&> {
  const Abstract& type = receiver.get_type().resolve();
  return type.visit<Language::Types::Fixed>(
      [](const Language::Types::Fixed& fixed) -> Utility::Option<const Type&> {
        return fixed.get_element_type();
      },
      [](const Abstract& type) {
        return type.visit<Language::Types::View>(
            [](const Language::Types::View& view)
                -> Utility::Option<const Type&> {
              return view.get_element_type();
            },
            [](const Abstract& type) {
              return type.visit<Language::Types::Access>(
                  [](const Language::Types::Access& access)
                      -> Utility::Option<const Type&> {
                    return access.get_element_type();
                  },
                  [](const Abstract&) -> Utility::Option<const Type&> {
                    return {};
                  });
            });
      });
}

static auto is_integer(const Language::Expression& expression) -> Bool {
  const Abstract& type = expression.get_type().resolve();
  return type.is<Ttx::Model::Types::Signed>() ||
         type.is<Ttx::Model::Types::Unsigned>();
}

// Signed is checked first because conversion to Count would erase the negative
// distinction. Unsigned then proves the remaining host width before conversion.
static auto get_count(
    const Language::Expression& expression,
    const Language::Expression& authored)
    -> Utility::Result<Count, Language::Expression::Error> {
  return expression.visit<Language::Constants::Signed>(
      [&](const Language::Constants::Signed& value)
          -> Utility::Result<Count, Language::Expression::Error> {
        if (value.get_value() < 0) {
          return Language::Expression::Error(
              Language::Expression::Error::Type::NegativeOperand, authored);
        }

        return Count(value.get_value());
      },
      [&](const Abstract& selected)
          -> Utility::Result<Count, Language::Expression::Error> {
        return selected.visit<Language::Constants::Unsigned>(
            [&](const Language::Constants::Unsigned& value)
                -> Utility::Result<Count, Language::Expression::Error> {
              if (value.get_value() > Unsigned_64(Count(-1))) {
                return Language::Expression::Error(
                    Language::Expression::Error::Type::CountOverflow, authored);
              }

              return Count(value.get_value());
            },
            [&](const Abstract&)
                -> Utility::Result<Count, Language::Expression::Error> {
              return Language::Expression::Error(
                  Language::Expression::Error::Type::InvalidConstant, authored);
            });
      });
}

static auto parse_operand(
    Memory::Allocator::Arena& domain,
    Language::Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context) -> Utility::Option<Language::Expression&> {
  Errors operand_errors;
  auto operand_cursor = cursor.branch(operand_errors);

  // Operand recursion uses the same primary and postfix dispatch exactly
  // once. Its provisional diagnostics stay local until the enclosing Slice
  // can attribute failure to the complete postfix.
  auto result = Language::Parser::Expression::parse(
      domain, materializations, operand_cursor, source_context);
  if (result) {
    cursor.join(operand_cursor);
  }

  return result;
}

// Fixed stores its Generic extent as Signed_64 even though Slice accepts Count.
// Reject the wider host values instead of wrapping the materialized argument.
static auto materialize_fixed(
    Language::Materializations& materializations,
    const Type& element,
    Count size) -> Utility::Option<const Type&> {
  if (Unsigned_64(size) > maximum_extent) {
    return {};
  }

  Core::Static::Vector<Language::Generic::Argument, 2> arguments = {{
    Language::Generic::Argument(element),
    Language::Generic::Argument(Signed_64(size)),
  }};
  return materializations.materialize(
      Language::Generics::Fixed::get_formula(), arguments.get_view());
}

// Only an Access receiver proves write capability. Fixed and View have the same
// ranged element shape but materialize View so Slice never invents mutation.
static auto materialize_dynamic(
    Language::Materializations& materializations,
    const Type& receiver,
    const Type& element) -> Utility::Option<const Type&> {
  Core::Static::Vector<Language::Generic::Argument, 1> arguments = {{
    Language::Generic::Argument(element),
  }};
  if (receiver.resolve().is<Language::Types::Access>()) {
    return materializations.materialize(
        Language::Generics::Access::get_formula(), arguments.get_view());
  }

  return materializations.materialize(
      Language::Generics::View::get_formula(), arguments.get_view());
}

static auto select_result_type(
    Language::Materializations& materializations,
    const Language::Expression& receiver,
    const Language::Expression& first,
    Utility::Option<const Language::Expression&> second) -> const Abstract& {
  auto element = get_element_type(receiver);
  if (!element || !is_integer(first)) {
    return Invalid::get_invalid();
  }

  if (!second) {
    return *element;
  }

  if (!is_integer(*second)) {
    return Invalid::get_invalid();
  }

  // A direct Constant contributes a durable extent during semantic linking.
  // An Operation may later fold to the same value, but changing View into
  // Fixed at that point would make folding double as graph Type resolution.
  if (second->is<Language::Constant>()) {
    auto count = get_count(*second, *second);
    return count.visit(
        [&](Count value) -> const Abstract& {
          return materialize_fixed(materializations, *element, value)
              .visit(
                  []() -> const Abstract& { return Invalid::get_invalid(); },
                  [](const Type& type) -> const Abstract& { return type; });
        },
        [](const Language::Expression::Error&) -> const Abstract& {
          return Invalid::get_invalid();
        });
  }

  const Abstract& receiver_type = receiver.get_type().resolve();
  return receiver_type.visit<Type>(
      [&](const Type& type) -> const Abstract& {
        return materialize_dynamic(materializations, type, *element)
            .visit(
                []() -> const Abstract& { return Invalid::get_invalid(); },
                [](const Type& result) -> const Abstract& { return result; });
      },
      [](const Abstract&) -> const Abstract& {
        return Invalid::get_invalid();
      });
}

auto Language::Operations::Slice::parse(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context,
    Expression& receiver) -> Utility::Option<Expression&> {
  // Expression selects Slice only after seeing SliceOp. Consuming it here
  // commits the transaction to this owner's complete postfix grammar.
  Token opening = cursor.consume();
  Code first_code = cursor.get_code();
  if (first_code.is_one_of({{
        Code::Type::Terminal,
        Code::Type::PackingOp,
        Code::Type::LayoutEnd,
      }})) {
    Span span = complete_postfix_span(cursor, opening);
    reject_syntax(cursor, span);
    return {};
  }

  Token first_start = cursor.current();
  Token first_end =
      cursor.matches(Code::Type::SubOp) ? cursor.peek(1) : first_start;
  auto first = parse_operand(domain, materializations, cursor, source_context);
  if (!first) {
    Span span = complete_postfix_span(cursor, opening);
    reject_operand(cursor, span, Span(first_start, first_end));
    return {};
  }

  // A comma (PackingOp) upgrades to a full span instead of an element level
  // slice. A range of 1 element is still different than a single element access
  // at a semantic level so `:[0, 1]` does not fold into `:[0]`.
  Bool range = cursor.matches(Code::Type::PackingOp);
  Utility::Option<Expression&> second;
  if (range) {
    cursor.consume();
    Code second_code = cursor.get_code();
    if (second_code.is_one_of({{
          Code::Type::Terminal,
          Code::Type::PackingOp,
          Code::Type::LayoutEnd,
        }})) {
      Span span = complete_postfix_span(cursor, opening);
      reject_syntax(cursor, span);
      return {};
    }

    Token second_start = cursor.current();
    Token second_end =
        cursor.matches(Code::Type::SubOp) ? cursor.peek(1) : second_start;
    second = parse_operand(domain, materializations, cursor, source_context);
    if (!second) {
      Span span = complete_postfix_span(cursor, opening);
      reject_operand(cursor, span, Span(second_start, second_end));
      return {};
    }
  }

  if (!cursor.matches(Code::Type::LayoutEnd)) {
    // No semantic operation exists until the closing token proves the complete
    // authored Slice. Recovery can therefore reject the tail without leaving a
    // partial graph owner.
    Span span = complete_postfix_span(cursor, opening);
    reject_syntax(cursor, span);
    return {};
  }

  cursor.consume();
  Token closing = cursor.peek(-1);
  const auto& receiver_anchor = receiver.get_anchor();
  const auto& first_anchor = first->get_anchor();
  if (!receiver_anchor || !first_anchor ||
      (range && (!second || !second->get_anchor()))) {
    cursor.create_expression_error(
        Span(opening, closing),
        "Slice requires authored operand Anchors."_view);
    return {};
  }

  auto anchor =
      Anchor::create(opening, receiver_anchor->get_span(), Span(closing));
  if (range) {
    return create_authored(
        domain, materializations, receiver, *first, *second, anchor);
  }

  return create_authored(domain, materializations, receiver, *first, anchor);
}

auto Language::Operations::Slice::create_authored(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& receiver,
    Expression& index,
    Anchor anchor) -> Slice& {
  Core::Static::Vector<Ttx::Concept::Reference<Expression>, 2> inputs = {{
    receiver,
    index,
  }};
  return Expression::create_authored<Slice>(
      domain, anchor, [&](auto source) -> Slice {
        return Slice(domain, materializations, inputs, source);
      });
}

auto Language::Operations::Slice::create_synthetic(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& receiver,
    Expression& index) -> Slice& {
  Core::Static::Vector<Ttx::Concept::Reference<Expression>, 2> inputs = {{
    receiver,
    index,
  }};
  return Expression::create_synthetic<Slice>(domain, [&](auto source) -> Slice {
    return Slice(domain, materializations, inputs, source);
  });
}

auto Language::Operations::Slice::create_authored(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& receiver,
    Expression& start,
    Expression& size,
    Anchor anchor) -> Slice& {
  Core::Static::Vector<Ttx::Concept::Reference<Expression>, 3> inputs = {{
    receiver,
    start,
    size,
  }};
  return Expression::create_authored<Slice>(
      domain, anchor, [&](auto source) -> Slice {
        return Slice(domain, materializations, inputs, source);
      });
}

auto Language::Operations::Slice::create_synthetic(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& receiver,
    Expression& start,
    Expression& size) -> Slice& {
  Core::Static::Vector<Ttx::Concept::Reference<Expression>, 3> inputs = {{
    receiver,
    start,
    size,
  }};
  return Expression::create_synthetic<Slice>(domain, [&](auto source) -> Slice {
    return Slice(domain, materializations, inputs, source);
  });
}

Language::Operations::Slice::Slice(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Core::View::Vector<Ttx::Concept::Reference<Expression>> inputs,
    Utility::Option<Anchor> anchor)
    : Operation(domain, materializations, inputs, anchor),
      range(inputs.get_size() == 3) {}

auto Language::Operations::Slice::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Operations::Slice::select_type(
    Materializations& materializations) const -> Utility::Option<const Type&> {
  auto receiver = get_input(0);
  auto first = get_input(1);
  if (!receiver || !first) {
    return {};
  }

  Utility::Option<const Expression&> second;
  if (is_range()) {
    second = get_input(2);
    if (!second) {
      return {};
    }
  }

  return select_result_type(materializations, *receiver, *first, second)
      .visit<Type>(
          [](const Type& type) -> Utility::Option<const Type&> { return type; },
          [](const Abstract&) -> Utility::Option<const Type&> { return {}; });
}

auto Language::Operations::Slice::evaluate_constants(
    Memory::Allocator::Arena& domain,
    Materializations&)
    -> Utility::Result<Utility::Option<Constant&>, Expression::Error> {
  auto authored_receiver = get_input(0);
  auto authored_first = get_input(1);
  auto receiver = get_folded_input(0);
  auto first_expression = get_folded_input(1);
  if (!authored_receiver || !authored_first || !receiver || !first_expression) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  auto element = get_element_type(*receiver);
  if (!element) {
    return Expression::Error(
        Expression::Error::Type::InvalidOperationType, *this);
  }

  auto first = get_count(*first_expression, *authored_first);
  if (!is_range()) {
    return first.visit(
        [&](Count index)
            -> Utility::Result<Utility::Option<Constant&>, Expression::Error> {
          return receiver->visit<Constants::Bytes>(
              [&](const Constants::Bytes& bytes)
                  -> Utility::Result<
                      Utility::Option<Constant&>, Expression::Error> {
                Core::View::Bytes value = bytes.get_value();
                if (index >= value.get_size()) {
                  return Expression::Error(
                      Expression::Error::Type::IndexOutOfBounds,
                      *authored_first);
                }

                if (&*element != &Dialect::get_unsigned_8()) {
                  return Expression::Error(
                      Expression::Error::Type::InvalidConstant,
                      *authored_receiver);
                }

                return Constants::Unsigned::create_synthetic(
                    domain, Dialect::get_unsigned_8(),
                    Unsigned_64(value[index]));
              },
              [&](const Abstract&)
                  -> Utility::Result<
                      Utility::Option<Constant&>, Expression::Error> {
                // Bytes is the live Constant payload domain. Another legal
                // ranged Constant stays as Slice until its payload owner
                // exists.
                return Utility::Option<Constant&>{};
              });
        },
        [](const Expression::Error& error)
            -> Utility::Result<Utility::Option<Constant&>, Expression::Error> {
          return error;
        });
  }

  auto authored_size = get_input(2);
  auto size_expression = get_folded_input(2);
  if (!authored_size || !size_expression) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  auto size = get_count(*size_expression, *authored_size);
  return first.visit(
      [&](Count start_value)
          -> Utility::Result<Utility::Option<Constant&>, Expression::Error> {
        return size.visit(
            [&](Count size_value)
                -> Utility::Result<
                    Utility::Option<Constant&>, Expression::Error> {
              if (Unsigned_64(size_value) > maximum_extent) {
                return Expression::Error(
                    Expression::Error::Type::CountOverflow, *authored_size);
              }

              return receiver->visit<Constants::Bytes>(
                  [&](const Constants::Bytes& bytes)
                      -> Utility::Result<
                          Utility::Option<Constant&>, Expression::Error> {
                    Core::View::Bytes value = bytes.get_value();
                    if (start_value > value.get_size()) {
                      return Expression::Error(
                          Expression::Error::Type::RangeStartOutOfBounds,
                          *authored_first);
                    }

                    // Start is established before subtraction, so the accepted
                    // remainder never depends on wrapped addition.
                    Count available = value.get_size() - start_value;
                    if (size_value > available) {
                      return Expression::Error(
                          Expression::Error::Type::RangeSizeOutOfBounds,
                          *authored_size);
                    }

                    const Abstract& result_type = get_type().resolve();
                    return result_type.visit<Type>(
                        [&](const Type& type)
                            -> Utility::Result<
                                Utility::Option<Constant&>, Expression::Error> {
                          return Constants::Bytes::create_synthetic(
                              domain, type,
                              value.slice(start_value, size_value));
                        },
                        [&](const Abstract&)
                            -> Utility::Result<
                                Utility::Option<Constant&>, Expression::Error> {
                          return Expression::Error(
                              Expression::Error::Type::InvalidOperationType,
                              *this);
                        });
                  },
                  [&](const Abstract&)
                      -> Utility::Result<
                          Utility::Option<Constant&>, Expression::Error> {
                    // Type legality does not imply a universal Constant payload
                    // interface, so unsupported payload owners remain Slice.
                    return Utility::Option<Constant&>{};
                  });
            },
            [](const Expression::Error& error)
                -> Utility::Result<
                    Utility::Option<Constant&>, Expression::Error> {
              return error;
            });
      },
      [](const Expression::Error& error)
          -> Utility::Result<Utility::Option<Constant&>, Expression::Error> {
        return error;
      });
}
