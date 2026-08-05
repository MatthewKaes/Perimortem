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

static auto reject_operand(
    Cursor& cursor,
    Span postfix_span,
    Span operand_span,
    Code code) -> void {
  auto report = cursor.create_report(postfix_span);
  if (code == Code::Type::Float) {
    report << "Slice operand `"_view
           << operand_span.caculate_text(cursor.get_source_text())
           << "` has Real_64 Type instead of an integer Type."_view;
    report.get_hint() << "Use a signed or unsigned integer expression."_view;
    return;
  }

  if (code == Code::Type::Numeric || code == Code::Type::Hex) {
    report << "Slice operand `"_view
           << operand_span.caculate_text(cursor.get_source_text())
           << "` exceeds the supported integer or Count range."_view;
    report.get_hint()
        << "Use a value no greater than 9223372036854775807."_view;
    return;
  }

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
static auto get_count(const Language::Expression& expression)
    -> Utility::Result<Count, Language::FoldError> {
  return expression.visit<Language::Constants::Signed>(
      [](const Language::Constants::Signed& value)
          -> Utility::Result<Count, Language::FoldError> {
        if (value.get_value() < 0) {
          return Language::FoldError(
              Language::FoldError::Type::NegativeOperand, value);
        }

        return Count(value.get_value());
      },
      [&](const Abstract& selected)
          -> Utility::Result<Count, Language::FoldError> {
        return selected.visit<Language::Constants::Unsigned>(
            [](const Language::Constants::Unsigned& value)
                -> Utility::Result<Count, Language::FoldError> {
              if (value.get_value() > Unsigned_64(Count(-1))) {
                return Language::FoldError(
                    Language::FoldError::Type::CountOverflow, value);
              }

              return Count(value.get_value());
            },
            [&](const Abstract&)
                -> Utility::Result<Count, Language::FoldError> {
              return Language::FoldError(
                  Language::FoldError::Type::InvalidConstant, expression);
            });
      });
}

static auto get_integer_value(const Language::Expression& expression)
    -> Unsigned_64 {
  return expression.visit<Language::Constants::Signed>(
      [](const Language::Constants::Signed& value) {
        return Unsigned_64(value.get_value());
      },
      [](const Abstract& expression) {
        return expression.visit<Language::Constants::Unsigned>(
            [](const Language::Constants::Unsigned& value) {
              return value.get_value();
            },
            [](const Abstract&) { return Unsigned_64(0); });
      });
}

static auto get_operand_name(
    const Language::Operations::Slice& slice,
    Count index) -> Core::View::Bytes {
  if (!slice.is_range()) {
    return "index"_view;
  }

  return index == 1 ? "start"_view : "size"_view;
}

static auto report_fold_error(
    Cursor& cursor,
    Span span,
    const Language::Operations::Slice& slice,
    const Language::Expression& receiver,
    const Language::Expression& first,
    Utility::Option<const Language::Expression&> second,
    const Language::FoldError& error) -> void {
  Count rejected = &error.get_expression() == &first ? 1 : 2;
  switch (error.get_type()) {
  case Language::FoldError::Type::NegativeOperand: {
    Signed_64 value = 0;
    error.get_expression().visit<Language::Constants::Signed>(
        [&](const Language::Constants::Signed& selected) {
          value = selected.get_value();
        },
        [](const Abstract&) {});

    auto report = cursor.create_report(span);
    report << "Slice "_view << get_operand_name(slice, rejected)
           << " value "_view << value << " is negative."_view;
    report.get_hint() << "Use zero or a positive integer value."_view;
    return;
  }
  case Language::FoldError::Type::CountOverflow: {
    auto report = cursor.create_report(span);
    report << "Slice "_view << get_operand_name(slice, rejected)
           << " value "_view << get_integer_value(error.get_expression())
           << " exceeds the supported Count or Fixed extent range."_view;
    report.get_hint()
        << "Use a value no greater than 9223372036854775807."_view;
    return;
  }
  case Language::FoldError::Type::IndexOutOfBounds: {
    receiver.visit<Language::Constants::Bytes>(
        [&](const Language::Constants::Bytes& bytes) {
          auto report = cursor.create_report(span);
          report << "Slice index "_view << get_integer_value(first)
                 << " is outside a value containing "_view
                 << Unsigned_64(bytes.get_value().get_size()) << " bytes."_view;
          report.get_hint() << "Use an index below the byte count."_view;
        },
        [](const Abstract&) {});
    return;
  }
  case Language::FoldError::Type::RangeStartOutOfBounds: {
    receiver.visit<Language::Constants::Bytes>(
        [&](const Language::Constants::Bytes& bytes) {
          auto report = cursor.create_report(span);
          report << "Slice start "_view << get_integer_value(first)
                 << " is beyond a value containing "_view
                 << Unsigned_64(bytes.get_value().get_size()) << " bytes."_view;
          report.get_hint()
              << "Use a start no greater than the byte count."_view;
        },
        [](const Abstract&) {});
    return;
  }
  case Language::FoldError::Type::RangeSizeOutOfBounds: {
    receiver.visit<Language::Constants::Bytes>(
        [&](const Language::Constants::Bytes& bytes) {
          Unsigned_64 start = get_integer_value(first);
          Unsigned_64 size = second ? get_integer_value(*second) : 0;
          Count available = bytes.get_value().get_size() - Count(start);
          auto report = cursor.create_report(span);
          report << "Slice size "_view << size << " exceeds the remaining "_view
                 << Unsigned_64(available) << " bytes after start "_view
                 << start << "."_view;
          report.get_hint()
              << "Reduce the size to the remaining byte count."_view;
        },
        [](const Abstract&) {});
    return;
  }
  default:
    break;
  }

  auto report = cursor.create_report(span);
  report << "Slice input `"_view << error.get_expression().get_name()
         << "` failed folding with "_view << error.get_name() << "."_view;
  report.get_hint()
      << "Check that input operation and its explicit result Type."_view;
}

static auto report_invalid_type(
    Cursor& cursor,
    Span span,
    const Language::Operations::Slice& slice,
    const Language::Expression& receiver,
    const Language::Expression& first,
    Utility::Option<const Language::Expression&> second) -> void {
  auto element = get_element_type(receiver);
  if (!element) {
    auto report = cursor.create_report(span);
    report << "Slice receiver Type `"_view << receiver.get_type().get_name()
           << "` is not a homogeneous Fixed, View, or Access Type."_view;
    report.get_hint()
        << "Use an expression with retained contiguous element identity."_view;
    return;
  }

  Utility::Option<const Language::Expression&> rejected;
  Count rejected_index = 1;
  if (!is_integer(first)) {
    rejected = first;
  } else if (second && !is_integer(*second)) {
    rejected = *second;
    rejected_index = 2;
  }

  if (rejected) {
    auto report = cursor.create_report(span);
    report << "Slice "_view << get_operand_name(slice, rejected_index)
           << " Type `"_view << rejected->get_type().get_name()
           << "` is not a signed or unsigned integer Type."_view;
    report.get_hint()
        << "Use an integer expression for every slice operand."_view;
    return;
  }

  if (second && second->is<Language::Constant>()) {
    auto count = get_count(*second);
    Bool failed = count.visit(
        [&](Count value) {
          if (Unsigned_64(value) <= maximum_extent) {
            return False;
          }

          report_fold_error(
              cursor, span, slice, receiver, first, second,
              Language::FoldError(
                  Language::FoldError::Type::CountOverflow, *second));
          return True;
        },
        [&](const Language::FoldError& error) {
          report_fold_error(
              cursor, span, slice, receiver, first, second, error);
          return True;
        });
    if (failed) {
      return;
    }
  }

  cursor.create_expression_error(
      span, "Library could not materialize the Slice result Type."_view,
      "Check the canonical Fixed, View, and Access formulas."_view);
}

static auto parse_operand(
    Memory::Allocator::Arena& domain,
    Language::Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context)
    -> Utility::Option<const Language::Expression&> {
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

static auto fold_slice(
    Memory::Allocator::Arena& domain,
    Language::Materializations& materializations,
    Cursor& cursor,
    Span span,
    const Language::Operations::Slice& slice,
    const Language::Expression& receiver,
    const Language::Expression& first,
    Utility::Option<const Language::Expression&> second)
    -> Utility::Option<const Language::Expression&> {
  if (!slice.get_type().resolve().is<Type>()) {
    report_invalid_type(cursor, span, slice, receiver, first, second);
    return {};
  }

  auto folded = slice.attempt_fold(domain, materializations);
  return folded.visit(
      [](const Language::Expression& expression)
          -> Utility::Option<const Language::Expression&> {
        return expression;
      },
      [&](const Language::FoldError& error)
          -> Utility::Option<const Language::Expression&> {
        report_fold_error(cursor, span, slice, receiver, first, second, error);
        return {};
      });
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

  // A direct Constant contributes a durable extent during construction. An
  // Operation may later fold to the same value, but changing View into Fixed
  // at that point would make folding double as graph Type resolution.
  if (second->is<Language::Constant>()) {
    auto count = get_count(*second);
    return count.visit(
        [&](Count value) -> const Abstract& {
          return materialize_fixed(materializations, *element, value)
              .visit(
                  []() -> const Abstract& { return Invalid::get_invalid(); },
                  [](const Type& type) -> const Abstract& { return type; });
        },
        [](const Language::FoldError&) -> const Abstract& {
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
    const Expression& receiver) -> Utility::Option<const Expression&> {
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
  Code first_operand_code = first_end.get_code();
  auto first = parse_operand(domain, materializations, cursor, source_context);
  if (!first) {
    Span span = complete_postfix_span(cursor, opening);
    reject_operand(
        cursor, span, Span(first_start, first_end), first_operand_code);
    return {};
  }

  // A comma (PackingOp) upgrades to a full span instead of an element level
  // slice. A range of 1 element is still different than a single element access
  // at a semantic level so `:[0, 1]` does not fold into `:[0]`.
  Bool range = cursor.matches(Code::Type::PackingOp);
  Utility::Option<const Expression&> second;
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
    Code second_operand_code = second_end.get_code();
    second = parse_operand(domain, materializations, cursor, source_context);
    if (!second) {
      Span span = complete_postfix_span(cursor, opening);
      reject_operand(
          cursor, span, Span(second_start, second_end), second_operand_code);
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
  Span span(opening, cursor.peek(-1));
  // Construction and folding share the graph transaction. A completed
  // Constant can replace the operation before its edge reaches the caller.
  if (range) {
    const auto& slice = domain.construct<Slice>(
        domain, materializations, receiver, *first, *second);
    return fold_slice(
        domain, materializations, cursor, span, slice, receiver, *first,
        second);
  }

  const auto& slice =
      domain.construct<Slice>(domain, materializations, receiver, *first);
  return fold_slice(
      domain, materializations, cursor, span, slice, receiver, *first, {});
}

Language::Operations::Slice::Slice(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    const Expression& receiver,
    const Expression& index)
    : Operation(
          domain,
          Core::Static::Vector<Ttx::Concept::Reference<Expression>, 2>{{
            receiver,
            index,
          }}),
      result_type(select_result_type(materializations, receiver, index, {})),
      range(False) {}

Language::Operations::Slice::Slice(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    const Expression& receiver,
    const Expression& start,
    const Expression& size)
    : Operation(
          domain,
          Core::Static::Vector<Ttx::Concept::Reference<Expression>, 3>{{
            receiver,
            start,
            size,
          }}),
      result_type(select_result_type(materializations, receiver, start, size)),
      range(True) {}

auto Language::Operations::Slice::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Operations::Slice::get_type() const -> const Abstract& {
  return result_type;
}

auto Language::Operations::Slice::evaluate_constants(
    Memory::Allocator::Arena& domain,
    Materializations&) const -> Utility::Result<const Expression&, FoldError> {
  auto receiver = get_input(0);
  auto first_expression = get_input(1);
  if (!receiver || !first_expression) {
    return FoldError(FoldError::Type::InvalidInput, *this);
  }

  auto element = get_element_type(*receiver);
  if (!element) {
    return FoldError(FoldError::Type::InvalidOperationType, *this);
  }

  auto first = get_count(*first_expression);
  if (!is_range()) {
    return first.visit(
        [&](Count index) -> Utility::Result<const Expression&, FoldError> {
          return receiver->visit<Constants::Bytes>(
              [&](const Constants::Bytes& bytes)
                  -> Utility::Result<const Expression&, FoldError> {
                Core::View::Bytes value = bytes.get_value();
                if (index >= value.get_size()) {
                  return FoldError(
                      FoldError::Type::IndexOutOfBounds, *first_expression);
                }

                if (&*element != &Dialect::get_unsigned_8()) {
                  return FoldError(FoldError::Type::InvalidConstant, *receiver);
                }

                return domain.construct<Constants::Unsigned>(
                    Dialect::get_unsigned_8(), Unsigned_64(value[index]));
              },
              [&](const Abstract&)
                  -> Utility::Result<const Expression&, FoldError> {
                // Bytes is the live Constant payload domain. Another legal
                // ranged Constant stays as Slice until its payload owner
                // exists.
                return *this;
              });
        },
        [](const FoldError& error)
            -> Utility::Result<const Expression&, FoldError> { return error; });
  }

  auto size_expression = get_input(2);
  if (!size_expression) {
    return FoldError(FoldError::Type::InvalidInput, *this);
  }

  auto size = get_count(*size_expression);
  return first.visit(
      [&](Count start_value) -> Utility::Result<const Expression&, FoldError> {
        return size.visit(
            [&](Count size_value)
                -> Utility::Result<const Expression&, FoldError> {
              if (Unsigned_64(size_value) > maximum_extent) {
                return FoldError(
                    FoldError::Type::CountOverflow, *size_expression);
              }

              return receiver->visit<Constants::Bytes>(
                  [&](const Constants::Bytes& bytes)
                      -> Utility::Result<const Expression&, FoldError> {
                    Core::View::Bytes value = bytes.get_value();
                    if (start_value > value.get_size()) {
                      return FoldError(
                          FoldError::Type::RangeStartOutOfBounds,
                          *first_expression);
                    }

                    // Start is established before subtraction, so the accepted
                    // remainder never depends on wrapped addition.
                    Count available = value.get_size() - start_value;
                    if (size_value > available) {
                      return FoldError(
                          FoldError::Type::RangeSizeOutOfBounds,
                          *size_expression);
                    }

                    const Abstract& result_type = get_type().resolve();
                    return result_type.visit<Type>(
                        [&](const Type& type)
                            -> Utility::Result<const Expression&, FoldError> {
                          return domain.construct<Constants::Bytes>(
                              type, value.slice(start_value, size_value));
                        },
                        [&](const Abstract&)
                            -> Utility::Result<const Expression&, FoldError> {
                          return FoldError(
                              FoldError::Type::InvalidOperationType, *this);
                        });
                  },
                  [&](const Abstract&)
                      -> Utility::Result<const Expression&, FoldError> {
                    // Type legality does not imply a universal Constant payload
                    // interface, so unsupported payload owners remain Slice.
                    return *this;
                  });
            },
            [](const FoldError& error)
                -> Utility::Result<const Expression&, FoldError> {
              return error;
            });
      },
      [](const FoldError& error)
          -> Utility::Result<const Expression&, FoldError> { return error; });
}
