// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/return.hpp"

#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/access/swizzle.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static const Layouts::Fluid empty_source;

static auto selects_fitting_type(
    const Abstract& value,
    const Language::Expression& expression) -> Bool {
  const Abstract& selected = value.visit<Addressable>(
      [](const Addressable& addressable) -> const Abstract& {
        return addressable.get_type();
      },
      [](const Abstract& abstract) -> const Abstract& { return abstract; });
  auto type = selected.select<Type>();
  if (!type) {
    type = selected.resolve().select<Type>();
  }

  return type && expression.fits(*type);
}

namespace {

// ScalarSource is the one value flow owned by an ordinary returned Expression.
// Call and Swizzle already expose their complete result Layouts, so this local
// view never copies or flattens those owners.
class ScalarSource : public Layout {
 public:
  constexpr explicit ScalarSource(const Language::Expression& expression)
      : expression(expression) {}

  constexpr auto get_size() const -> Count override { return 1; }

  constexpr auto get_abstract(Count index) const
      -> Core::Option<const Abstract&> override {
    BAIL_IF(index != 0);
    return expression;
  }

  auto fits_at(const Layout& target, Count target_offset) const
      -> Bool override {
    BAIL_IF(!has_target_segment(target, target_offset));
    return target.get_abstract(target_offset)
        .visit(
            []() { return False; },
            [&](const Abstract& target_entry) {
              return selects_fitting_type(target_entry, expression);
            });
  }

  auto get_fitted_at(
      const Layout& target,
      Count target_offset,
      Count target_index) const
      -> Utility::Result<const Abstract&, Errors> override {
    if (target_index != 0) {
      return Errors::IndexOutOfBounds;
    }
    if (!has_target_segment(target, target_offset)) {
      return Errors::SizeMismatch;
    }
    if (!fits_at(target, target_offset)) {
      return Errors::IncompatibleFit;
    }

    return expression;
  }

 private:
  const Language::Expression& expression;
};

}  // namespace

static auto get_source_layout(
    const Language::Expression& expression,
    const ScalarSource& scalar) -> const Layout& {
  return expression.visit<Language::Access::Call>(
      [](const Language::Access::Call& call) -> const Layout& {
        return call.get_results();
      },
      [&](const Abstract& not_call) -> const Layout& {
        return not_call.visit<Language::Access::Swizzle>(
            [](const Language::Access::Swizzle& swizzle) -> const Layout& {
              return swizzle.get_results();
            },
            [&](const Abstract&) -> const Layout& { return scalar; });
      });
}

auto Language::Return::interpret(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context) -> Core::Option<Return&> {
  auto transaction = cursor.branch();
  Token operation = transaction.require(
      Code::Type::Return,
      "Library return statements require the `return` keyword."_view);
  BAIL_IF(!operation);

  Core::Option<Expression&> expression;
  if (!transaction.matches(Code::Type::EndStatement)) {
    expression = Parser::Expression::parse(
        domain, materializations, transaction, source_context);
    BAIL_IF(!expression);
  }

  Token terminator = transaction.require(
      Code::Type::EndStatement,
      "Library return statements require one terminating `;`."_view);
  BAIL_IF(!terminator);

  Core::Option<Reference<Expression>> retained_expression;
  if (expression) {
    retained_expression = Reference<Expression>(*expression);
  }
  Return& result = domain.construct_from<Return>([&]() -> Return {
    return Return(
        Anchor::create(operation, Span(operation, terminator)),
        retained_expression);
  });
  cursor.join(transaction);
  return result;
}

auto Language::Return::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    Materializations& materializations,
    const Type& access_scope,
    const Layout& results) -> Bool {
  if (linked) {
    return True;
  }

  Bool fits = empty_source.fits(results);
  if (expression) {
    Expression& selected = expression->get();
    BAIL_IF(!selected.link(
        source, lexical_context, materializations, access_scope));
    ScalarSource scalar(selected);
    fits = get_source_layout(selected, scalar).fits(results);
  }

  if (!fits) {
    source.report(
        anchor,
        "Return value Layout does not fit the Function result Layout."_view,
        "Return the complete ordered values required by the Function "
        "signature."_view);
    return False;
  }

  linked = True;
  return True;
}

auto Language::Return::finalize() -> void {
  expression.visit(
      []() {}, [](Reference<Expression>& selected) { selected.get().fold(); });
}

auto Language::Return::get_expression() const
    -> Core::Option<const Expression&> {
  return expression.visit(
      []() -> Core::Option<const Expression&> { return {}; },
      [](const Reference<Expression>& selected)
          -> Core::Option<const Expression&> { return selected.get(); });
}
