// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/propagate.hpp"

#include "tetrodotoxin/library/language/constants/option.hpp"
#include "tetrodotoxin/library/language/flow/scope.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto select_option_constant(Language::Model::Pack& source)
    -> Core::Option<Language::Constants::Option&> {
  auto direct = source.select<Language::Constants::Option>();
  if (direct) {
    return *direct;
  }

  const Layout& layout = source.get_layout();
  BAIL_IF(layout.get_size() != 1);
  return layout.get_abstract(0).visit(
      []() -> Core::Option<Language::Constants::Option&> { return {}; },
      [](const Abstract& selected)
          -> Core::Option<Language::Constants::Option&> {
        auto pack =
            const_cast<Abstract&>(selected).select<Language::Model::Pack>();
        return pack ? pack->select<Language::Constants::Option>()
                    : Core::Option<Language::Constants::Option&>();
      });
}

auto Language::Access::Propagate::parse(
    const Abstract&,
    Cursor& cursor,
    Expression& receiver) -> Core::Option<Expression&> {
  Memory::Allocator::Arena& domain = cursor.get_arena();
  Token operation = cursor.require(
      Code::Type::QuestionOp,
      "Library Option propagation requires postfix `?`."_view);
  BAIL_IF(!operation);

  auto receiver_anchor = receiver.get_anchor();
  BAIL_IF(!receiver_anchor);
  Anchor anchor =
      Anchor::create(operation, receiver_anchor->get_span(), Span(operation));
  // Absence exits through real empty Pack flow. Keeping that Pack on this
  // operation lets link negotiate with the enclosing Function result instead
  // of encoding Function policy in the parser.
  Model::Pack& empty_return = Model::Pack::create_empty(domain);
  Propagate& propagate = Expression::create_authored<Propagate>(
      domain, anchor, [&](Core::Option<Anchor> source) -> Propagate {
        return Propagate(receiver, empty_return, source);
      });
  return propagate;
}

auto Language::Access::Propagate::link(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  BAIL_IF(!receiver.link(cursor, lexical_context, access_scope));
  auto option = receiver.get_type().resolve().select<Types::Option>();
  if (!option) {
    cursor.create_expression_error(
        get_anchor(), "Postfix `?` requires one Option value."_view,
        "Use `?` only where absence should return from the Function."_view);
    return False;
  }

  auto scope = lexical_context.select<Flow::Scope>();
  // The empty path is valid only when the enclosing Function can receive it.
  // The present path keeps the exact payload Type and continues normally.
  BAIL_IF(!empty_return.link(cursor, lexical_context, access_scope));
  if (!scope || !empty_return.fits(scope->get_function_results())) {
    cursor.create_expression_error(
        get_anchor(),
        "Postfix `?` cannot return empty flow from this Function."_view,
        "Use `[]` or one Option result Layout for the enclosing Function."_view);
    return False;
  }

  if (element_type && &element_type->get() != &option->get_element_type()) {
    cursor.create_expression_error(
        get_anchor(),
        "Option propagation selected a different element Type."_view,
        "Repeat linking with the same completed Option identity."_view);
    return False;
  }

  element_type =
      Reference<const Language::Model::Type>(option->get_element_type());
  return Expression::link(cursor, lexical_context, access_scope);
}

auto Language::Access::Propagate::get_type() const -> const Abstract& {
  return element_type.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Language::Model::Type>& selected)
          -> const Abstract& { return selected.get(); });
}

auto Language::Access::Propagate::finalize(Cursor& cursor) -> void {
  receiver.finalize(cursor);
  empty_return.finalize(cursor);
  Expression::finalize(cursor);
}

auto Language::Access::Propagate::lower(Llvm::Builder& body) const -> Bool {
  auto folded = lower_folded(body);
  if (folded) {
    return *folded;
  }

  auto carrier = receiver.get_type().resolve().select<Ttx::Model::Type>();
  auto element = get_type().resolve().select<Ttx::Model::Type>();

  if (!carrier || !element) {
    return False;
  }

  Bool receiver_lowered = receiver.lower(body);
  if (!receiver_lowered) {
    return False;
  }

  return body.propagate(*carrier, *element, *this, receiver);
}

auto Language::Access::Propagate::evaluate()
    -> Utility::Result<Core::Option<Model::Pack&>, Expression::Error> {
  Core::Option<Model::Pack&> folded;
  Core::Option<Expression::Error> error;
  receiver.fold().visit(
      [&](const Core::Option<Model::Pack&>& selected) { folded = selected; },
      [&](const Expression::Error& selected) { error = selected; });
  if (error) {
    return *error;
  }
  if (!folded) {
    return Core::Option<Model::Pack&>{};
  }

  auto option = select_option_constant(*folded);
  if (!option) {
    return Expression::Error(Expression::Error::Type::InvalidConstant, *this);
  }

  auto payload = option->get_payload();
  // An absent folded Option produces no values. Runtime lowering observes the
  // same empty path and owns the actual early return control transfer.
  return payload
             ? Core::Option<Model::Pack&>(const_cast<Model::Pack&>(*payload))
             : Core::Option<Model::Pack&>();
}
