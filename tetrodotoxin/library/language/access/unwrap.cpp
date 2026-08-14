// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/unwrap.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/constants/option.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
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

auto Language::Access::Unwrap::parse(
    Memory::Allocator::Arena& domain,
    Language::Monograph&,
    Cursor& cursor,
    Expression& receiver) -> Core::Option<Expression&> {
  auto transaction = cursor.branch();
  Token operation = transaction.require(
      Code::Type::NotOp, "Library Option unwrap requires postfix `!`."_view);
  BAIL_IF(!operation);

  auto receiver_anchor = receiver.get_anchor();
  BAIL_IF(!receiver_anchor);
  Anchor anchor =
      Anchor::create(operation, receiver_anchor->get_span(), Span(operation));
  Unwrap& unwrap = Expression::create_authored<Unwrap>(
      domain, anchor, [&](Core::Option<Anchor> source) -> Unwrap {
        return Unwrap(domain, receiver, source);
      });
  cursor.join(transaction);
  return unwrap;
}

auto Language::Access::Unwrap::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    Core::Option<const Type&> access_scope) -> Bool {
  BAIL_IF(!receiver.link(source, lexical_context, access_scope));
  auto option = receiver.get_type().resolve().select<Types::Option>();
  if (!option) {
    source.report(
        get_anchor(), "Postfix `!` requires one Option value."_view,
        "Use `!` only where absence should produce the element Type default."_view);
    return False;
  }

  if (element_type && &element_type->get() != &option->get_element_type()) {
    source.report(
        get_anchor(), "Option unwrap selected a different element Type."_view,
        "Repeat linking with the same completed Option identity."_view);
    return False;
  }

  element_type = Reference<const Type>(option->get_element_type());
  return Expression::link(source, lexical_context, access_scope);
}

auto Language::Access::Unwrap::get_type() const -> const Abstract& {
  return element_type.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Type>& selected) -> const Abstract& {
        return selected.get();
      });
}

auto Language::Access::Unwrap::finalize() -> void {
  receiver.finalize();
  Expression::finalize();
}

auto Language::Access::Unwrap::evaluate()
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
  if (!option || !element_type) {
    return Expression::Error(Expression::Error::Type::InvalidConstant, *this);
  }

  auto payload = option->get_payload();
  if (payload) {
    return const_cast<Model::Pack&>(*payload);
  }

  auto fallback = Library::Dialect::create_default(domain, element_type->get());
  if (!fallback) {
    return Expression::Error(Expression::Error::Type::InvalidConstant, *this);
  }

  const Layout& layout = fallback->get_layout();
  for (Count index = 0; index < layout.get_size(); index++) {
    auto value = layout.get_abstract(index);
    if (!value || !value->is<Constant>()) {
      return Core::Option<Model::Pack&>{};
    }
  }
  return *fallback;
}
