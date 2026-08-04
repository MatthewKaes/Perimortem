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
#include "ttx/lexical/tokenizer.hpp"
#include "ttx/model/types/signed.hpp"
#include "ttx/model/types/unsigned.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;

static constexpr Unsigned_64 maximum_extent = Unsigned_64(-1) >> 1;

static auto complete_postfix_span(Cursor& cursor, Token opening, Token ending)
    -> Span {
  // A malformed tail still belongs to one Slice. Consume only its local
  // closing bracket so the diagnostic covers the authored operation.
  while (!cursor.matches(Code::Type::Terminal) &&
         !cursor.matches(Code::Type::LayoutEnd)) {
    ending = cursor.consume();
  }

  if (cursor.matches(Code::Type::LayoutEnd)) {
    ending = cursor.consume();
  }

  if (cursor.matches(Code::Type::Terminal)) {
    // Tokenizer keeps its sentinel one byte beyond the authored source. Slice
    // closes the diagnostic at the source boundary so a missing bracket does
    // not extend the rendered marker.
    Token terminal = cursor.current();
    ending = Token(
        Unsigned_16(cursor.get_source_text().get_size()), terminal.get_line(),
        terminal.get_column(), 0, Code::Type::Terminal);
  }

  return Span(opening, ending);
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
          return Language::FoldError::NegativeOperand;
        }

        return Count(value.get_value());
      },
      [](const Abstract& expression)
          -> Utility::Result<Count, Language::FoldError> {
        return expression.visit<Language::Constants::Unsigned>(
            [](const Language::Constants::Unsigned& value)
                -> Utility::Result<Count, Language::FoldError> {
              if (value.get_value() > Unsigned_64(Count(-1))) {
                return Language::FoldError::CountOverflow;
              }

              return Count(value.get_value());
            },
            [](const Abstract&) -> Utility::Result<Count, Language::FoldError> {
              return Language::FoldError::InvalidOperandType;
            });
      });
}

static auto get_input(const Language::Operations::Slice& slice, Count index)
    -> const Language::Expression& {
  auto selected = slice.get_inputs().get_abstract(index);
  return static_cast<const Language::Expression&>(*selected);
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
    Language::FoldError error) -> void {
  switch (error) {
  case Language::FoldError::InvalidReceiverType: {
    auto report = cursor.create_report(span);
    report << "Slice receiver Type `"_view
           << get_input(slice, 0).get_type().get_name()
           << "` is not a homogeneous Fixed, View, or Access Type."_view;
    report.get_hint()
        << "Use an expression with retained contiguous element identity."_view;
    return;
  }
  case Language::FoldError::InvalidOperandType: {
    Count rejected = 1;
    for (Count i = 1; i < slice.get_inputs().get_size(); i++) {
      if (!is_integer(get_input(slice, i))) {
        rejected = i;
        break;
      }
    }

    const auto& operand = get_input(slice, rejected);
    auto report = cursor.create_report(span);
    report << "Slice "_view << get_operand_name(slice, rejected)
           << " Type `"_view << operand.get_type().get_name()
           << "` is not a signed or unsigned integer Type."_view;
    report.get_hint()
        << "Use an integer expression for every slice operand."_view;
    return;
  }
  case Language::FoldError::NegativeOperand: {
    Count rejected = 1;
    Signed_64 value = 0;
    for (Count i = 1; i < slice.get_inputs().get_size(); i++) {
      const auto& operand = get_input(slice, i);
      Bool negative = operand.visit<Language::Constants::Signed>(
          [&](const Language::Constants::Signed& selected) {
            value = selected.get_value();
            return value < 0 ? True : False;
          },
          [](const Abstract&) { return False; });
      if (negative) {
        rejected = i;
        break;
      }
    }

    auto report = cursor.create_report(span);
    report << "Slice "_view << get_operand_name(slice, rejected)
           << " value "_view << value << " is negative."_view;
    report.get_hint() << "Use zero or a positive integer value."_view;
    return;
  }
  case Language::FoldError::CountOverflow: {
    Count rejected = slice.is_range() ? 2 : 1;
    auto report = cursor.create_report(span);
    report << "Slice "_view << get_operand_name(slice, rejected)
           << " value "_view << get_integer_value(get_input(slice, rejected))
           << " exceeds the supported Count or Fixed extent range."_view;
    report.get_hint()
        << "Use a value no greater than 9223372036854775807."_view;
    return;
  }
  case Language::FoldError::IndexOutOfBounds: {
    const auto& receiver =
        static_cast<const Language::Constants::Bytes&>(get_input(slice, 0));
    Unsigned_64 index = get_integer_value(get_input(slice, 1));
    auto report = cursor.create_report(span);
    report << "Slice index "_view << index
           << " is outside a value containing "_view
           << Unsigned_64(receiver.get_value().get_size()) << " bytes."_view;
    report.get_hint() << "Use an index below the byte count."_view;
    return;
  }
  case Language::FoldError::RangeStartOutOfBounds: {
    const auto& receiver =
        static_cast<const Language::Constants::Bytes&>(get_input(slice, 0));
    Unsigned_64 start = get_integer_value(get_input(slice, 1));
    auto report = cursor.create_report(span);
    report << "Slice start "_view << start
           << " is beyond a value containing "_view
           << Unsigned_64(receiver.get_value().get_size()) << " bytes."_view;
    report.get_hint() << "Use a start no greater than the byte count."_view;
    return;
  }
  case Language::FoldError::RangeSizeOutOfBounds: {
    const auto& receiver =
        static_cast<const Language::Constants::Bytes&>(get_input(slice, 0));
    Unsigned_64 start = get_integer_value(get_input(slice, 1));
    Unsigned_64 size = get_integer_value(get_input(slice, 2));
    Count available = receiver.get_value().get_size() - Count(start);
    auto report = cursor.create_report(span);
    report << "Slice size "_view << size << " exceeds the remaining "_view
           << Unsigned_64(available) << " bytes after start "_view << start
           << "."_view;
    report.get_hint() << "Reduce the size to the remaining byte count."_view;
    return;
  }
  case Language::FoldError::TypeMaterializationFailure: {
    cursor.create_expression_error(
        span, "Library could not materialize the Slice result Type."_view,
        "Check the canonical Fixed, View, and Access formulas."_view);
    return;
  }
  case Language::FoldError::Unknown: {
    cursor.create_expression_error(
        span, "Library could not fold this Slice operation."_view,
        "Check the receiver and every authored operand."_view);
    return;
  }
  }
}

static auto parse_operand(
    Memory::Allocator::Arena& domain,
    Language::Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context)
    -> Utility::Option<const Language::Expression&> {
  Memory::Allocator::Arena token_domain;
  Errors operand_errors;
  Tokenizer operand_tokens(
      token_domain, cursor.get_source_text(), cursor.get_source_path());
  Cursor operand_cursor(operand_tokens, operand_errors);
  operand_cursor.sync(cursor);

  // Operand recursion uses the same primary and postfix dispatch exactly
  // once. Its provisional diagnostics stay local until the enclosing Slice
  // can attribute failure to the complete postfix.
  auto result = Language::Parser::Expression::parse(
      domain, materializations, operand_cursor, source_context);
  if (result) {
    cursor.sync(operand_cursor);
  }

  return result;
}

static auto fold_slice(
    Memory::Allocator::Arena& domain,
    Language::Materializations& materializations,
    Cursor& cursor,
    Span span,
    const Language::Operations::Slice& slice)
    -> Utility::Option<const Language::Expression&> {
  auto folded = slice.attempt_fold(domain, materializations);
  return folded.visit(
      [](const Language::Expression& expression)
          -> Utility::Option<const Language::Expression&> {
        return expression;
      },
      [&](Language::FoldError error)
          -> Utility::Option<const Language::Expression&> {
        report_fold_error(cursor, span, slice, error);
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
    Span span = complete_postfix_span(cursor, opening, opening);
    reject_syntax(cursor, span);
    return {};
  }

  Token first_start = cursor.current();
  Token first_end =
      cursor.matches(Code::Type::SubOp) ? cursor.peek(1) : first_start;
  Code first_operand_code = first_end.get_code();
  auto first = parse_operand(domain, materializations, cursor, source_context);
  if (!first) {
    Span span = complete_postfix_span(cursor, opening, opening);
    reject_operand(
        cursor, span, Span(first_start, first_end), first_operand_code);
    return {};
  }

  // A comma (PackingOp) upgrades to a full span instead of an element level
  // slice. A range of 1 element is still different than a single element access
  // at a semantic level so `:[0, 1]` does not fold into `:[0]`.
  Bool range = cursor.matches(Code::Type::PackingOp);
  Token separator;
  Utility::Option<const Expression&> second;
  if (range) {
    separator = cursor.consume();
    Code second_code = cursor.get_code();
    if (second_code.is_one_of({{
          Code::Type::Terminal,
          Code::Type::PackingOp,
          Code::Type::LayoutEnd,
        }})) {
      Span span = complete_postfix_span(cursor, opening, separator);
      reject_syntax(cursor, span);
      return {};
    }

    Token second_start = cursor.current();
    Token second_end =
        cursor.matches(Code::Type::SubOp) ? cursor.peek(1) : second_start;
    Code second_operand_code = second_end.get_code();
    second = parse_operand(domain, materializations, cursor, source_context);
    if (!second) {
      Span span = complete_postfix_span(cursor, opening, separator);
      reject_operand(
          cursor, span, Span(second_start, second_end), second_operand_code);
      return {};
    }
  }

  if (!cursor.matches(Code::Type::LayoutEnd)) {
    // No semantic operation exists until the closing token proves the complete
    // authored Slice. Recovery can therefore reject the tail without leaving a
    // partial graph owner.
    Span span = complete_postfix_span(cursor, opening, cursor.current());
    reject_syntax(cursor, span);
    return {};
  }

  Token closing = cursor.consume();
  Span span(opening, closing);
  // Construction and folding share the graph transaction. A completed
  // Constant can replace the operation before its edge reaches the caller.
  if (range) {
    const auto& slice = domain.construct<Slice>(
        domain, materializations, receiver, *first, *second);
    return fold_slice(domain, materializations, cursor, span, slice);
  }

  const auto& slice =
      domain.construct<Slice>(domain, materializations, receiver, *first);
  return fold_slice(domain, materializations, cursor, span, slice);
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
      materializations(materializations),
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
      materializations(materializations),
      range(True) {}

auto Language::Operations::Slice::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Operations::Slice::get_expression(Count index) const
    -> const Expression& {
  auto selected = get_inputs().get_abstract(index);
  return static_cast<const Expression&>(*selected);
}

auto Language::Operations::Slice::get_type() const -> const Abstract& {
  const Expression& receiver = get_expression(0);
  const Expression& first = get_expression(1);

  // The concrete range Type retains its element even when Fixed has no entry
  // at index zero. Query that owner instead of treating Layout size as Type.
  auto element = get_element_type(receiver);
  if (!element || !is_integer(first)) {
    return Invalid::get_invalid();
  }

  // Indexing preserves the exact retained element Type without materialization.
  if (!is_range()) {
    return *element;
  }

  const Expression& size = get_expression(2);
  if (!is_integer(size)) {
    return Invalid::get_invalid();
  }

  if (size.is<Language::Constant>()) {
    // A folded size chooses Fixed even while receiver or start stays dynamic.
    // That stable count is shape rather than a read only storage capability.
    auto count = get_count(size);
    return count.visit(
        [&](Count value) -> const Abstract& {
          return materialize_fixed(materializations, *element, value)
              .visit(
                  []() -> const Abstract& { return Invalid::get_invalid(); },
                  [](const Type& type) -> const Abstract& { return type; });
        },
        [](Language::FoldError) -> const Abstract& {
          return Invalid::get_invalid();
        });
  }

  // Dynamic size keeps Access only when the receiver already owns that write
  // capability. Fixed, View, Bytes, and Constant identity cannot invent it.
  return materialize_dynamic(
             materializations,
             static_cast<const Type&>(receiver.get_type().resolve()), *element)
      .visit(
          []() -> const Abstract& { return Invalid::get_invalid(); },
          [](const Type& type) -> const Abstract& { return type; });
}

auto Language::Operations::Slice::evaluate_constants(
    Memory::Allocator::Arena& domain,
    Materializations&) const -> Utility::Result<const Expression&, FoldError> {
  const Expression& receiver = get_expression(0);
  auto element = get_element_type(receiver);
  if (!element) {
    return FoldError::InvalidReceiverType;
  }

  auto first = get_count(get_expression(1));
  if (!is_range()) {
    return first.visit(
        [&](Count index) -> Utility::Result<const Expression&, FoldError> {
          if (!receiver.is<Constants::Bytes>()) {
            // Bytes is the live Constant payload domain. Other legal ranged
            // Constants stay as Slice until their own payload owner exists.
            return static_cast<const Expression&>(*this);
          }

          const auto& bytes = static_cast<const Constants::Bytes&>(receiver);
          Core::View::Bytes value = bytes.get_value();
          if (index >= value.get_size()) {
            return FoldError::IndexOutOfBounds;
          }

          if (&*element != &Dialect::get_unsigned_8()) {
            return FoldError::InvalidReceiverType;
          }

          return domain.construct<Constants::Unsigned>(
              Dialect::get_unsigned_8(), Unsigned_64(value[index]));
        },
        [](FoldError error) -> Utility::Result<const Expression&, FoldError> {
          return error;
        });
  }

  auto size = get_count(get_expression(2));
  FoldError error = FoldError::Unknown;
  Count start_value = first.visit(
      [](Count value) { return value; },
      [&](FoldError selected) {
        error = selected;
        return Count(0);
      });
  Count size_value = size.visit(
      [](Count value) { return value; },
      [&](FoldError selected) {
        error = selected;
        return Count(0);
      });
  if (error != FoldError::Unknown) {
    return error;
  }

  if (Unsigned_64(size_value) > maximum_extent) {
    return FoldError::CountOverflow;
  }

  if (!receiver.is<Constants::Bytes>()) {
    // Type legality does not imply a universal Constant payload interface.
    return static_cast<const Expression&>(*this);
  }

  const auto& bytes = static_cast<const Constants::Bytes&>(receiver);
  Core::View::Bytes value = bytes.get_value();
  if (start_value > value.get_size()) {
    return FoldError::RangeStartOutOfBounds;
  }

  // Start is established before subtraction, so the accepted remainder never
  // depends on wrapped addition.
  Count available = value.get_size() - start_value;
  if (size_value > available) {
    return FoldError::RangeSizeOutOfBounds;
  }

  const Abstract& result_type = get_type().resolve();
  if (!result_type.is<Type>()) {
    return FoldError::TypeMaterializationFailure;
  }

  return domain.construct<Constants::Bytes>(
      static_cast<const Type&>(result_type),
      value.slice(start_value, size_value));
}
