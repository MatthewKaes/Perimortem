// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/value.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
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

static auto complete_postfix_span(Cursor& cursor, Token opening) -> Span {
  // A malformed tail still belongs to one Value access. Consume only its local
  // closing bracket so the diagnostic covers the authored operation.
  while (!cursor.matches(Code::Type::Terminal) &&
         !cursor.matches(Code::Type::BracketEnd)) {
    cursor.consume();
  }

  if (cursor.matches(Code::Type::BracketEnd)) {
    cursor.consume();
  }

  return Span(opening, cursor.peek(-1));
}

static auto reject_syntax(Cursor& cursor, Span span) -> void {
  cursor.create_expression_error(
      span, "Value access has malformed index or range operands."_view,
      "Use `:[index]` or `:[start, count]` with complete delimiters."_view);
}

static auto reject_operand(Cursor& cursor, Span postfix_span, Span operand_span)
    -> void {
  auto report = cursor.create_report(postfix_span);
  report << "Value access operand `"_view
         << operand_span.caculate_text(cursor.get_source_text())
         << "` could not be parsed as a complete Expression."_view;
  report.get_hint() << "Use a complete scalar or byte Expression."_view;
}

static auto get_element_type(const Language::Expression& receiver)
    -> Core::Option<const Type&> {
  const Abstract& type = receiver.get_type().resolve();
  return type.visit<Language::Types::Fixed>(
      [](const Language::Types::Fixed& fixed) -> Core::Option<const Type&> {
        return fixed.get_element_type();
      },
      [](const Abstract& type) {
        return type.visit<Language::Types::View>(
            [](const Language::Types::View& view) -> Core::Option<const Type&> {
              return view.get_element_type();
            },
            [](const Abstract& type) {
              return type.visit<Language::Types::Access>(
                  [](const Language::Types::Access& access)
                      -> Core::Option<const Type&> {
                    return access.get_element_type();
                  },
                  [](const Abstract&) -> Core::Option<const Type&> {
                    return {};
                  });
            });
      });
}

static auto get_byte_type(const Type& type)
    -> Core::Option<const Ttx::Model::Types::Unsigned&> {
  return type.visit<Ttx::Model::Types::Unsigned>(
      [](const Ttx::Model::Types::Unsigned& selected)
          -> Core::Option<const Ttx::Model::Types::Unsigned&> {
        if (selected.get_width() != 8 || selected.get_size() != 1) {
          return {};
        }

        return selected;
      },
      [](const Abstract&) -> Core::Option<const Ttx::Model::Types::Unsigned&> {
        return {};
      });
}

static auto is_integer(const Language::Expression& expression) -> Bool {
  const Abstract& type = expression.get_type().resolve();
  return type.is<Ttx::Model::Types::Signed>() ||
         type.is<Ttx::Model::Types::Unsigned>();
}

// Option is the safe miss produced by an integer outside Count. Error remains
// reserved for a Constant that does not expose its promised integer domain.
static auto get_count(
    const Language::Expression& expression,
    const Language::Expression& authored)
    -> Utility::Result<Core::Option<Count>, Language::Expression::Error> {
  return expression.visit<Language::Constants::Signed>(
      [&](const Language::Constants::Signed& value)
          -> Utility::Result<Core::Option<Count>, Language::Expression::Error> {
        if (value.get_value() < 0) {
          return Core::Option<Count>{};
        }

        Unsigned_64 selected = Unsigned_64(value.get_value());
        if (selected > Unsigned_64(Count(-1))) {
          return Core::Option<Count>{};
        }

        return Core::Option<Count>(Count(selected));
      },
      [&](const Abstract& selected)
          -> Utility::Result<Core::Option<Count>, Language::Expression::Error> {
        return selected.visit<Language::Constants::Unsigned>(
            [&](const Language::Constants::Unsigned& value)
                -> Utility::Result<
                    Core::Option<Count>, Language::Expression::Error> {
              if (value.get_value() > Unsigned_64(Count(-1))) {
                return Core::Option<Count>{};
              }

              return Core::Option<Count>(Count(value.get_value()));
            },
            [&](const Abstract&)
                -> Utility::Result<
                    Core::Option<Count>, Language::Expression::Error> {
              return Language::Expression::Error(
                  Language::Expression::Error::Type::InvalidConstant, authored);
            });
      });
}

static auto parse_operand(
    Memory::Allocator::Arena& domain,
    Language::Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context) -> Core::Option<Language::Expression&> {
  Errors operand_errors;
  auto operand_cursor = cursor.branch(operand_errors);

  // The complete operand grammar stays inside this private Cursor. Its
  // provisional diagnostics remain local until Value can attribute failure to
  // the complete postfix.
  auto result = Language::Parser::Expression::parse(
      domain, materializations, operand_cursor, source_context);
  if (result) {
    cursor.join(operand_cursor);
  }

  return result;
}

// Value returns a view even when its receiver supplies writable access.
// The separate bracket access form owns optional references, so retaining
// Access here would let safe value selection manufacture write capability.
static auto materialize_view(
    Language::Materializations& materializations,
    const Type& element) -> Core::Option<const Type&> {
  Core::Static::Vector<Language::Generic::Argument, 1> arguments = {{
    Language::Generic::Argument(element),
  }};
  return materializations.materialize(
      Language::Generics::View::get_formula(), arguments.get_view());
}

static auto select_result_type(
    Language::Materializations& materializations,
    const Language::Expression& receiver,
    const Language::Expression& first,
    Core::Option<const Language::Expression&> second) -> const Abstract& {
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

  return materialize_view(materializations, *element)
      .visit(
          []() -> const Abstract& { return Invalid::get_invalid(); },
          [](const Type& result) -> const Abstract& { return result; });
}

auto Language::Access::Value::parse(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context,
    Expression& receiver) -> Core::Option<Expression&> {
  // Expression selects Value only after seeing ValueAccessOp. Consuming it here
  // commits the transaction to this owner's complete postfix grammar.
  Token opening = cursor.consume();
  Code first_code = cursor.get_code();
  if (first_code.is_one_of({{
        Code::Type::Terminal,
        Code::Type::PackingOp,
        Code::Type::BracketEnd,
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

  // A comma changes element selection into a ranged value. A range containing
  // one element still has View Type, so `:[0, 1]` does not become `:[0]`.
  Bool range = cursor.matches(Code::Type::PackingOp);
  Core::Option<Expression&> second;
  if (range) {
    cursor.consume();
    Code second_code = cursor.get_code();
    if (second_code.is_one_of({{
          Code::Type::Terminal,
          Code::Type::PackingOp,
          Code::Type::BracketEnd,
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

  if (!cursor.matches(Code::Type::BracketEnd)) {
    // No semantic operation exists until the closing token proves the complete
    // authored Value. Recovery can therefore reject the tail without leaving a
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
        "Value access requires authored operand Anchors."_view);
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

auto Language::Access::Value::create_authored(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& receiver,
    Expression& index,
    Anchor anchor) -> Value& {
  Core::Static::Vector<Ttx::Concept::Reference<Expression>, 2> inputs = {{
    receiver,
    index,
  }};
  return Expression::create_authored<Value>(
      domain, anchor, [&](auto source) -> Value {
        return Value(domain, materializations, inputs, source);
      });
}

auto Language::Access::Value::create_synthetic(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& receiver,
    Expression& index) -> Value& {
  Core::Static::Vector<Ttx::Concept::Reference<Expression>, 2> inputs = {{
    receiver,
    index,
  }};
  return Expression::create_synthetic<Value>(domain, [&](auto source) -> Value {
    return Value(domain, materializations, inputs, source);
  });
}

auto Language::Access::Value::create_authored(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& receiver,
    Expression& start,
    Expression& count,
    Anchor anchor) -> Value& {
  Core::Static::Vector<Ttx::Concept::Reference<Expression>, 3> inputs = {{
    receiver,
    start,
    count,
  }};
  return Expression::create_authored<Value>(
      domain, anchor, [&](auto source) -> Value {
        return Value(domain, materializations, inputs, source);
      });
}

auto Language::Access::Value::create_synthetic(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Expression& receiver,
    Expression& start,
    Expression& count) -> Value& {
  Core::Static::Vector<Ttx::Concept::Reference<Expression>, 3> inputs = {{
    receiver,
    start,
    count,
  }};
  return Expression::create_synthetic<Value>(domain, [&](auto source) -> Value {
    return Value(domain, materializations, inputs, source);
  });
}

Language::Access::Value::Value(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Core::View::Vector<Ttx::Concept::Reference<Expression>> inputs,
    Core::Option<Anchor> anchor)
    : Operation(domain, materializations, inputs, anchor) {}

auto Language::Access::Value::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Access::Value::select_type(
    Materializations& materializations) const -> Core::Option<const Type&> {
  auto receiver = get_input(0);
  auto first = get_input(1);
  if (!receiver || !first) {
    return {};
  }

  Core::Option<const Expression&> count;
  auto authored_count = get_input(2);
  if (authored_count) {
    count = *authored_count;
  }

  return select_result_type(materializations, *receiver, *first, count)
      .select<Type>();
}

auto Language::Access::Value::evaluate_constants(
    Memory::Allocator::Arena& domain,
    Materializations&)
    -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
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
  auto authored_count = get_input(2);
  if (!authored_count) {
    return first.visit(
        [&](const Core::Option<Count>& index)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          return receiver->visit<Constants::Bytes>(
              [&](const Constants::Bytes& bytes)
                  -> Utility::Result<
                      Core::Option<Constant&>, Expression::Error> {
                Core::View::Bytes value = bytes.get_value();
                // Bytes exposes raw elements, but the result still carries the
                // exact eight bit Unsigned Type selected during linking.
                auto byte_type = get_byte_type(*element);
                if (!byte_type) {
                  return Expression::Error(
                      Expression::Error::Type::InvalidConstant,
                      *authored_receiver);
                }

                // Bounds misses need the element Type's real default owner.
                // Until that owner exists the semantic access remains valid
                // but folding stays dynamic instead of inventing zero here.
                if (!index || *index >= value.get_size()) {
                  return Core::Option<Constant&>{};
                }

                Unsigned_64 selected = Unsigned_64(value.get_data()[*index]);
                return Constants::Unsigned::create_synthetic(
                    domain, *byte_type, selected);
              },
              [&](const Abstract&)
                  -> Utility::Result<
                      Core::Option<Constant&>, Expression::Error> {
                // Bytes is the live Constant payload domain. Another legal
                // ranged Constant stays as Value until its payload owner
                // exists.
                return Core::Option<Constant&>{};
              });
        },
        [](const Expression::Error& error)
            -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
          return error;
        });
  }

  auto count_expression = get_folded_input(2);
  if (!count_expression) {
    return Expression::Error(Expression::Error::Type::InvalidInput, *this);
  }

  auto count = get_count(*count_expression, *authored_count);
  return first.visit(
      [&](const Core::Option<Count>& start)
          -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
        return count.visit(
            [&](const Core::Option<Count>& count)
                -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
              return receiver->visit<Constants::Bytes>(
                  [&](const Constants::Bytes& bytes)
                      -> Utility::Result<
                          Core::Option<Constant&>, Expression::Error> {
                    // A value that cannot become an extent needs the real
                    // default owner. Valid extents delegate empty and clipped
                    // results to the same View contract used elsewhere.
                    if (!start || !count) {
                      return Core::Option<Constant&>{};
                    }

                    Core::View::Bytes selected =
                        bytes.get_value().slice(*start, *count);
                    const Abstract& result_type = get_type().resolve();
                    return result_type.visit<Type>(
                        [&](const Type& type)
                            -> Utility::Result<
                                Core::Option<Constant&>, Expression::Error> {
                          return Constants::Bytes::create_synthetic(
                              domain, type, selected);
                        },
                        [&](const Abstract&)
                            -> Utility::Result<
                                Core::Option<Constant&>, Expression::Error> {
                          return Expression::Error(
                              Expression::Error::Type::InvalidOperationType,
                              *this);
                        });
                  },
                  [&](const Abstract&)
                      -> Utility::Result<
                          Core::Option<Constant&>, Expression::Error> {
                    // Type legality does not imply a universal Constant payload
                    // interface, so unsupported payload owners remain Value.
                    return Core::Option<Constant&>{};
                  });
            },
            [](const Expression::Error& error)
                -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
              return error;
            });
      },
      [](const Expression::Error& error)
          -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
        return error;
      });
}
