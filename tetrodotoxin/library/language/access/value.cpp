// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/value.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/types/access.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/model/addressable.hpp"
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

static auto select_required_type(const Abstract& candidate)
    -> Core::Option<const Type&> {
  auto direct = candidate.select<Type>();
  if (direct) {
    return *direct;
  }

  const Abstract& resolved = candidate.resolve();
  auto addressable = candidate.select<Addressable>();
  if (!addressable) {
    addressable = resolved.select<Addressable>();
  }
  const Abstract& selected = addressable ? addressable->get_type() : resolved;
  direct = selected.select<Type>();
  return direct ? direct : selected.resolve().select<Type>();
}

namespace {

// A slice is homogeneous value flow, but the repeated source is still the one
// Value expression that performs selection. Keeping this Layout subordinate to
// Value avoids teaching host-neutral Ranged how a Library Expression fits a
// required element Type, and avoids fabricating one proxy identity per slot.
class SliceLayout final : public Layout {
 public:
  constexpr SliceLayout(
      const Language::Access::Value& source,
      const Type& element,
      Count size)
      : source(source), element(element), size(size) {}

  constexpr auto get_size() const -> Count override { return size; }

  constexpr auto get_abstract(Count index) const
      -> Core::Option<const Abstract&> override {
    BAIL_IF(index >= size);
    return source;
  }

  auto fits_entry(const Layout& target, Count source_index, Count target_index)
      const -> Bool override {
    BAIL_IF(source_index >= size || target_index >= target.get_size());
    return target.get_abstract(target_index)
        .visit(
            []() { return False; },
            [&](const Abstract& required) {
              return select_required_type(required).visit(
                  []() { return False; },
                  [&](const Type& type) {
                    return element.get_layout().fits(type.get_layout());
                  });
            });
  }

  auto fits_at(const Layout& target, Count target_offset) const
      -> Bool override {
    BAIL_IF(!has_target_segment(target, target_offset));
    for (Count index = 0; index < size; index++) {
      BAIL_IF(!fits_entry(target, index, target_offset + index));
    }
    return True;
  }

  auto get_fitted_at(
      const Layout& target,
      Count target_offset,
      Count target_index) const
      -> Utility::Result<const Abstract&, Errors> override {
    if (target_index >= size) {
      return Errors::IndexOutOfBounds;
    }
    if (!has_target_segment(target, target_offset)) {
      return Errors::SizeMismatch;
    }
    if (!fits_at(target, target_offset)) {
      return Errors::IncompatibleFit;
    }

    // Layout index retains which repeated value is consumed. Lowering can use
    // that index without replacing the one semantic producer with shadow nodes.
    return source;
  }

 private:
  const Language::Access::Value& source;
  const Type& element;
  Count size;
};

}  // namespace

static auto parse_operand(
    Memory::Allocator::Arena& domain,
    Language::Monograph& source,
    Cursor& cursor) -> Core::Option<Language::Expression&> {
  Errors operand_errors;
  auto operand_cursor = cursor.branch(operand_errors);

  // The complete operand grammar stays inside this private Cursor. Its
  // provisional diagnostics remain local until Value can attribute failure to
  // the complete postfix.
  auto pack =
      Language::Parser::Expression::parse(domain, source, operand_cursor);
  auto result = pack.visit(
      []() -> Core::Option<Language::Expression&> { return {}; },
      [](Language::Model::Pack& selected) {
        return selected.select<Language::Expression>();
      });
  if (result) {
    cursor.join(operand_cursor);
  }

  return result;
}

static auto select_result_type(
    const Language::Expression& receiver,
    const Language::Expression& first) -> const Abstract& {
  auto element = get_element_type(receiver);
  if (!element || !is_integer(first)) {
    return Invalid::get_invalid();
  }

  return *element;
}

auto Language::Access::Value::parse(
    Memory::Allocator::Arena& domain,
    Monograph& source,
    Cursor& cursor,
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
  auto first = parse_operand(domain, source, cursor);
  if (!first) {
    Span span = complete_postfix_span(cursor, opening);
    reject_operand(cursor, span, Span(first_start, first_end));
    return {};
  }

  // A comma changes element selection into ranged Pack flow. A one-entry range
  // retains that authored range shape even though scalar reflection can expose
  // its one element Type.
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
    second = parse_operand(domain, source, cursor);
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
    return create_authored(domain, receiver, *first, *second, anchor);
  }

  return create_authored(domain, receiver, *first, anchor);
}

auto Language::Access::Value::create_authored(
    Memory::Allocator::Arena& domain,
    Expression& receiver,
    Expression& index,
    Anchor anchor) -> Value& {
  Core::Static::Vector<Ttx::Concept::Reference<Expression>, 2> inputs = {{
    receiver,
    index,
  }};
  return Expression::create_authored<Value>(
      domain, anchor,
      [&](auto source) -> Value { return Value(domain, inputs, source); });
}

auto Language::Access::Value::create_synthetic(
    Memory::Allocator::Arena& domain,
    Expression& receiver,
    Expression& index) -> Value& {
  Core::Static::Vector<Ttx::Concept::Reference<Expression>, 2> inputs = {{
    receiver,
    index,
  }};
  return Expression::create_synthetic<Value>(domain, [&](auto source) -> Value {
    return Value(domain, inputs, source);
  });
}

auto Language::Access::Value::create_authored(
    Memory::Allocator::Arena& domain,
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
      domain, anchor,
      [&](auto source) -> Value { return Value(domain, inputs, source); });
}

auto Language::Access::Value::create_synthetic(
    Memory::Allocator::Arena& domain,
    Expression& receiver,
    Expression& start,
    Expression& count) -> Value& {
  Core::Static::Vector<Ttx::Concept::Reference<Expression>, 3> inputs = {{
    receiver,
    start,
    count,
  }};
  return Expression::create_synthetic<Value>(domain, [&](auto source) -> Value {
    return Value(domain, inputs, source);
  });
}

Language::Access::Value::Value(
    Memory::Allocator::Arena& domain,
    Core::View::Vector<Ttx::Concept::Reference<Expression>> inputs,
    Core::Option<Anchor> anchor)
    : Operation(domain, inputs, anchor),
      domain(domain),
      range(inputs.get_size() == 3) {}

auto Language::Access::Value::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    Core::Option<const Type&> access_scope) -> Bool {
  if (!range) {
    return Operation::link(source, lexical_context, access_scope);
  }

  // Range count determines the complete Pack shape and is therefore a link
  // fact, not a lowering-time payload detail. Start remains ordinary dynamic
  // input because it changes which values flow, never how many slots exist.
  Bool failed = False;
  for (Count index = 0; index < 3; index++) {
    auto input = get_input(index);
    if (!input) {
      failed = True;
      continue;
    }
    failed |= !input->link(source, lexical_context, access_scope);
  }
  BAIL_IF(failed);

  auto receiver = get_input(0);
  auto start = get_input(1);
  auto count = get_input(2);
  auto element =
      receiver ? get_element_type(*receiver) : Core::Option<const Type&>{};
  if (!receiver || !start || !count || !element || !is_integer(*start) ||
      !is_integer(*count)) {
    source.report(
        get_anchor(), "Value range rejects the linked operand Types."_view,
        "Use an indexable receiver and integer start and count Expressions."_view);
    return False;
  }

  Core::Option<Expression&> folded_count;
  Core::Option<Expression::Error> fold_error;
  count->fold().visit(
      [&](const Core::Option<Expression&>& folded) { folded_count = folded; },
      [&](const Expression::Error& error) { fold_error = error; });
  if (fold_error || !folded_count) {
    source.report(
        count->get_anchor(),
        "Value range count did not constant-fold during linking."_view,
        "Supply one nonnegative integer Constant for the range count."_view);
    return False;
  }

  Core::Option<Count> selected_count;
  Core::Option<Expression::Error> count_error;
  get_count(*folded_count, *count)
      .visit(
          [&](const Core::Option<Count>& selected) {
            selected_count = selected;
          },
          [&](const Expression::Error& error) { count_error = error; });
  if (count_error || !selected_count) {
    source.report(
        count->get_anchor(),
        "Value range count is outside the supported nonnegative range."_view,
        "Use a nonnegative integer Constant representable as Count."_view);
    return False;
  }

  if (range_layout) {
    Bool changed = !range_element || &range_element->get() != &*element ||
                   !range_count || *range_count != *selected_count;
    if (changed) {
      source.report(
          get_anchor(),
          "Value range cannot change its linked output Layout."_view,
          "Keep one exact element Type and constant count for this access."_view);
      return False;
    }
    return True;
  }

  range_element = Reference<const Type>(*element);
  range_count = *selected_count;
  const auto& layout =
      domain.construct<SliceLayout>(*this, *element, *selected_count);
  range_layout = layout;
  return True;
}

auto Language::Access::Value::get_type() const -> const Abstract& {
  if (!range) {
    return Operation::get_type();
  }
  if (!range_layout || !range_count || *range_count != 1 || !range_element) {
    return Invalid::get_invalid();
  }

  return range_element->get();
}

auto Language::Access::Value::get_layout() const -> const Layout& {
  if (!range) {
    return Expression::get_layout();
  }

  return *range_layout;
}

auto Language::Access::Value::resolve() const -> const Abstract& {
  if (!range) {
    return Expression::resolve();
  }

  if (!range_layout) {
    return Invalid::get_invalid();
  }

  return static_cast<const Ttx::Model::Pack&>(*this);
}

auto Language::Access::Value::fits(const Type& target) const -> Bool {
  if (!range) {
    return Expression::fits(target);
  }

  // A range Value is not one element with a special carrier Type. Its complete
  // Pack must negotiate with the receiving descriptor so `Fixed[T, count]`,
  // structural Layouts, and the empty Layout all observe the same flow.
  return Model::Pack::fits(target);
}

auto Language::Access::Value::select_type(
    Tetrodotoxin::Language::Monograph&) const -> Core::Option<const Type&> {
  BAIL_IF(range);
  auto receiver = get_input(0);
  auto first = get_input(1);
  if (!receiver || !first) {
    return {};
  }

  return select_result_type(*receiver, *first).select<Type>();
}

auto Language::Access::Value::evaluate_constants(
    Memory::Allocator::Arena& domain)
    -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
  if (range) {
    // A range is a Pack of values rather than one Constant carrier. Folding
    // individual selected values belongs to lowering once it consumes the
    // Pack Layout and its slot indices.
    return Core::Option<Constant&>{};
  }

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
  return first.visit(
      [&](const Core::Option<Count>& index)
          -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
        return receiver->visit<Constants::Bytes>(
            [&](const Constants::Bytes& bytes)
                -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
              Core::View::Bytes value = bytes.get_value();
              if (!index || *index >= value.get_size()) {
                // No payload element exists, so the exact element Type decides
                // whether safe selection has a value or a failure.
                auto fallback =
                    Library::Dialect::create_default(domain, *element);
                if (!fallback) {
                  return Expression::Error(
                      Expression::Error::Type::InvalidConstant, *this);
                }

                return *fallback;
              }

              // Bytes exposes raw elements, but the result still carries the
              // exact eight bit Unsigned Type selected during linking.
              auto byte_type = get_byte_type(*element);
              if (!byte_type) {
                return Expression::Error(
                    Expression::Error::Type::InvalidConstant,
                    *authored_receiver);
              }

              Unsigned_64 selected = Unsigned_64(value.get_data()[*index]);
              return Constants::Unsigned::create_synthetic(
                  domain, *byte_type, selected);
            },
            [&](const Abstract&)
                -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
              // Bytes is the live Constant payload domain. Another legal
              // Constant stays as Value until its payload owner exists.
              return Core::Option<Constant&>{};
            });
      },
      [](const Expression::Error& error)
          -> Utility::Result<Core::Option<Constant&>, Expression::Error> {
        return error;
      });
}
