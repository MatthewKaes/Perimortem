// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/slice.hpp"

#include "perimortem/memory/managed/vector.hpp"

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
  // A malformed tail still belongs to one Slice access. Consume only its local
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
      span, "Slice access has malformed index or range operands."_view,
      "Use `:[index]` or `:[start, count]` with complete delimiters."_view);
}

static auto reject_operand(Cursor& cursor, Span postfix_span, Span operand_span)
    -> void {
  auto report = cursor.create_report(postfix_span);
  report << "Slice access operand `"_view
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

static auto select_scalar(Language::Model::Pack& pack)
    -> Core::Option<Language::Expression&> {
  return pack.select<Language::Expression>();
}

static auto select_required_type(const Abstract& candidate)
    -> Core::Option<const Type&> {
  auto direct = candidate.select<Type>();
  if (direct) {
    return *direct;
  }

  auto pack = candidate.select<Language::Model::Pack>();
  if (pack) {
    direct = pack->get_type().select<Type>();
    if (direct) {
      return *direct;
    }
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

// A slice is homogeneous value flow, but the repeated source is still the one
// Slice expression that performs selection. Keeping this Layout subordinate to
// Slice avoids teaching host-neutral Ranged how a Library Expression fits a
// required element Type, and avoids fabricating one proxy identity per slot.
static auto create_layout(
    Memory::Allocator::Arena& domain,
    const Language::Access::Slice& source,
    const Type& element,
    Count size) -> const Ttx::Concept::Layout& {
  class Layout final : public Ttx::Concept::Layout {
   public:
    constexpr Layout(
        const Language::Access::Slice& source,
        const Type& element,
        Count size)
        : source(source), element(element), size(size) {}

    constexpr auto get_size() const -> Count override { return size; }

    constexpr auto get_abstract(Count index) const
        -> Core::Option<const Abstract&> override {
      BAIL_IF(index >= size);
      return source;
    }

    auto fits_entry(
        const Ttx::Concept::Layout& target,
        Count source_index,
        Count target_index) const -> Bool override {
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

    auto fits_at(const Ttx::Concept::Layout& target, Count target_offset) const
        -> Bool override {
      BAIL_IF(!has_target_segment(target, target_offset));
      for (Count index = 0; index < size; index++) {
        BAIL_IF(!fits_entry(target, index, target_offset + index));
      }
      return True;
    }

    auto get_fitted_at(
        const Ttx::Concept::Layout& target,
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
      // that index without replacing the one semantic producer with shadow
      // nodes.
      return source;
    }

   private:
    const Language::Access::Slice& source;
    const Type& element;
    Count size;
  };

  return domain.construct<Layout>(source, element, size);
}

static auto parse_operand(
    Memory::Allocator::Arena& domain,
    Language::Monograph& source,
    Cursor& cursor) -> Core::Option<Language::Expression&> {
  Errors operand_errors;
  auto operand_cursor = cursor.branch(operand_errors);

  // The complete operand grammar stays inside this private Cursor. Its
  // provisional diagnostics remain local until Slice can attribute failure to
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

auto Language::Access::Slice::parse(
    Memory::Allocator::Arena& domain,
    Monograph& source,
    Cursor& cursor,
    Expression& receiver) -> Core::Option<Expression&> {
  // Expression selects Slice only after seeing ValueAccessOp. Consuming it here
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
        "Slice access requires authored operand Anchors."_view);
    return {};
  }

  auto anchor =
      Anchor::create(opening, receiver_anchor->get_span(), Span(closing));
  if (range) {
    return create_authored(domain, receiver, *first, *second, anchor);
  }

  return create_authored(domain, receiver, *first, anchor);
}

auto Language::Access::Slice::create_authored(
    Memory::Allocator::Arena& domain,
    Expression& receiver,
    Expression& index,
    Anchor anchor) -> Slice& {
  return Expression::create_authored<Slice>(
      domain, anchor, [&](auto source) -> Slice {
        return Slice(domain, receiver, index, source);
      });
}

auto Language::Access::Slice::create_authored(
    Memory::Allocator::Arena& domain,
    Expression& receiver,
    Expression& start,
    Expression& count,
    Anchor anchor) -> Slice& {
  return Expression::create_authored<Slice>(
      domain, anchor, [&](auto source) -> Slice {
        return Slice(domain, receiver, start, count, source);
      });
}

Language::Access::Slice::Slice(
    Memory::Allocator::Arena& domain,
    Expression& receiver,
    Expression& index,
    Core::Option<Anchor> anchor)
    : Expression(anchor), domain(domain), receiver(receiver), first(index) {}

Language::Access::Slice::Slice(
    Memory::Allocator::Arena& domain,
    Expression& receiver,
    Expression& start,
    Expression& count,
    Core::Option<Anchor> anchor)
    : Expression(anchor),
      domain(domain),
      receiver(receiver),
      first(start),
      count(Ttx::Concept::Reference<Expression>(count)) {}

auto Language::Access::Slice::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    Core::Option<const Type&> access_scope) -> Bool {
  Bool failed = !receiver.link(source, lexical_context, access_scope);
  failed |= !first.link(source, lexical_context, access_scope);
  if (count) {
    failed |= !count->get().link(source, lexical_context, access_scope);
  }
  BAIL_IF(failed);

  auto element = get_element_type(receiver);
  if (!element || !is_integer(first) || (count && !is_integer(count->get()))) {
    source.report(
        get_anchor(), "Slice rejects the linked operand Types."_view,
        "Use an indexable receiver and integer index, start, and count Expressions."_view);
    return False;
  }

  if (element_type && &element_type->get() != &*element) {
    source.report(
        get_anchor(), "Slice cannot change its linked element Type."_view,
        "Keep one exact element Type on this authored access."_view);
    return False;
  }
  element_type = Reference<const Type>(*element);

  if (!count) {
    return Expression::link(source, lexical_context, access_scope);
  }

  // Range count determines the complete Pack shape and is therefore a link
  // fact, not a lowering-time payload detail. Start remains ordinary dynamic
  // input because it changes which values flow, never how many slots exist.
  Expression& count_expression = count->get();

  Core::Option<Model::Pack&> folded_count;
  Core::Option<Expression::Error> fold_error;
  count_expression.fold().visit(
      [&](const Core::Option<Model::Pack&>& folded) { folded_count = folded; },
      [&](const Expression::Error& error) { fold_error = error; });
  if (fold_error || !folded_count) {
    source.report(
        count_expression.get_anchor(),
        "Slice range count did not constant-fold during linking."_view,
        "Supply one nonnegative integer Constant for the range count."_view);
    return False;
  }

  auto folded_count_expression = select_scalar(*folded_count);
  if (!folded_count_expression) {
    source.report(
        count_expression.get_anchor(),
        "Slice range count did not fold to one scalar Constant."_view,
        "Supply one nonnegative integer Constant for the range count."_view);
    return False;
  }

  Core::Option<Count> selected_count;
  Core::Option<Expression::Error> count_error;
  get_count(*folded_count_expression, count_expression)
      .visit(
          [&](const Core::Option<Count>& selected) {
            selected_count = selected;
          },
          [&](const Expression::Error& error) { count_error = error; });
  if (count_error || !selected_count) {
    source.report(
        count_expression.get_anchor(),
        "Slice range count is outside the supported nonnegative range."_view,
        "Use a nonnegative integer Constant representable as Count."_view);
    return False;
  }

  if (range_layout) {
    Bool changed = !range_count || *range_count != *selected_count;
    if (changed) {
      source.report(
          get_anchor(),
          "Slice range cannot change its linked output Layout."_view,
          "Keep one exact element Type and constant count for this access."_view);
      return False;
    }
    return True;
  }

  range_count = *selected_count;
  const auto& layout = create_layout(domain, *this, *element, *selected_count);
  range_layout = layout;
  return True;
}

auto Language::Access::Slice::get_type() const -> const Abstract& {
  if (!element_type ||
      (count && (!range_layout || !range_count || *range_count != 1))) {
    return Invalid::get_invalid();
  }

  return element_type->get();
}

auto Language::Access::Slice::get_layout() const
    -> const Ttx::Concept::Layout& {
  if (!count) {
    return Expression::get_layout();
  }

  return *range_layout;
}

auto Language::Access::Slice::resolve() const -> const Abstract& {
  if (!count) {
    return Expression::resolve();
  }

  if (!range_layout) {
    return Invalid::get_invalid();
  }

  return static_cast<const Ttx::Model::Pack&>(*this);
}

auto Language::Access::Slice::fits(const Type& target) const -> Bool {
  if (!count) {
    return Expression::fits(target);
  }

  // A range Slice is not one element with a special carrier Type. Its complete
  // Pack must negotiate with the receiving descriptor so `Fixed[T, count]`,
  // structural Layouts, and the empty Layout all observe the same flow.
  return Model::Pack::fits(target);
}

auto Language::Access::Slice::finalize() -> void {
  receiver.finalize();
  first.finalize();
  if (count) {
    count->get().finalize();
  }
  Expression::finalize();
}

auto Language::Access::Slice::evaluate()
    -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
  Core::Option<Model::Pack&> folded_receiver_pack;
  auto receiver_fold = receiver.fold();
  auto receiver_error = Core::Option<Expression::Error>{};
  receiver_fold.visit(
      [&](const Core::Option<Model::Pack&>& selected) {
        folded_receiver_pack = selected;
      },
      [&](const Expression::Error& error) { receiver_error = error; });
  if (receiver_error) {
    return *receiver_error;
  }

  Core::Option<Model::Pack&> folded_first_pack;
  auto first_fold = first.fold();
  auto first_error = Core::Option<Expression::Error>{};
  first_fold.visit(
      [&](const Core::Option<Model::Pack&>& selected) {
        folded_first_pack = selected;
      },
      [&](const Expression::Error& error) { first_error = error; });
  if (first_error) {
    return *first_error;
  }
  if (!folded_receiver_pack || !folded_first_pack) {
    return Core::Option<Model::Pack&>{};
  }

  auto folded_receiver = select_scalar(*folded_receiver_pack);
  auto folded_first = select_scalar(*folded_first_pack);
  if (!folded_receiver || !folded_first) {
    return Expression::Error(Expression::Error::Type::InvalidConstant, *this);
  }

  auto element = get_element_type(*folded_receiver);
  if (!element) {
    return Expression::Error(
        Expression::Error::Type::InvalidOperationType, *this);
  }

  auto selected_index = get_count(*folded_first, first);
  return selected_index.visit(
      [&](const Core::Option<Count>& index)
          -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
        return folded_receiver->visit<Constants::Bytes>(
            [&](const Constants::Bytes& bytes)
                -> Utility::Result<
                    Core::Option<Model::Pack&>, Expression::Error> {
              Core::View::Bytes value = bytes.get_value();
              if (!count) {
                if (!index || *index >= value.get_size()) {
                  // No payload element exists, so the exact element Type
                  // decides whether safe selection has a value or a failure.
                  auto fallback =
                      Library::Dialect::create_default(domain, *element);
                  if (!fallback) {
                    return Expression::Error(
                        Expression::Error::Type::InvalidConstant, *this);
                  }

                  return static_cast<Model::Pack&>(*fallback);
                }

                auto byte_type = get_byte_type(*element);
                if (!byte_type) {
                  return Expression::Error(
                      Expression::Error::Type::InvalidConstant, receiver);
                }
                Unsigned_64 selected = Unsigned_64(value.get_data()[*index]);
                return static_cast<Model::Pack&>(
                    Constants::Unsigned::create_synthetic(
                        domain, *byte_type, selected));
              }

              if (!range_count) {
                return Expression::Error(
                    Expression::Error::Type::InvalidConstant, *this);
              }

              auto byte_type = get_byte_type(*element);
              if (!byte_type) {
                return Expression::Error(
                    Expression::Error::Type::InvalidConstant, receiver);
              }

              // A ranged fold represents real selected payload values. Unlike
              // scalar safe access, the range form has no per-slot defaulting
              // rule. Keep an out-of-bounds constant range as authored flow
              // for lowering instead of fabricating values or attempting an
              // unbounded compile-time allocation.
              if (!index || *index > value.get_size() ||
                  *range_count > value.get_size() - *index) {
                return Core::Option<Model::Pack&>{};
              }

              Memory::Managed::Vector<Reference<Model::Pack>> entries(domain);
              entries.reset(*range_count);
              for (Count offset = 0; offset < *range_count; offset++) {
                Count position = *index + offset;
                Unsigned_64 selected = Unsigned_64(value.get_data()[position]);
                entries.insert(
                    static_cast<Model::Pack&>(
                        Constants::Unsigned::create_synthetic(
                            domain, *byte_type, selected)));
              }

              return Model::Pack::create_folded(domain, entries.get_view());
            },
            [&](const Abstract&)
                -> Utility::Result<
                    Core::Option<Model::Pack&>, Expression::Error> {
              // Bytes is the live Constant payload domain. Another legal
              // Constant stays as Slice until its payload owner exists.
              return Core::Option<Model::Pack&>{};
            });
      },
      [](const Expression::Error& error)
          -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
        return error;
      });
}
