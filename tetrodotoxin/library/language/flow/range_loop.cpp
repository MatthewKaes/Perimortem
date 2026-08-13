// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/flow/range_loop.hpp"

#include "tetrodotoxin/library/language/model/parser/layout.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

auto Language::Flow::RangeLoop::interpret(
    Allocator::Arena& domain,
    Monograph& source,
    Cursor& cursor,
    Block& lexical_context,
    Callable& function,
    const Type& access_scope) -> Option<RangeLoop&> {
  auto transaction = cursor.branch();
  if (!transaction.matches(Code::Type::For)) {
    return {};
  }

  Token opening = transaction.require(
      Code::Type::For, "Library Range loops require the `for` keyword."_view);
  BAIL_IF(!opening);

  if (!transaction.matches(Code::Type::BracketStart)) {
    transaction.create_token_error(
        "Library Range loop bindings require one bracketed named entry."_view);
    return {};
  }

  Option<Token> name;
  Option<TypeReference> type_reference;
  Count entries = 0;
  auto binding_end = Model::Parser::Layout::parse(
      transaction,
      [&](Cursor& entry, Count index, Option<Token> selected_name) -> Bool {
        if (index != 0 || !selected_name ||
            selected_name->get_code() != Code::Type::Addressable) {
          entry.create_token_error(
              "A Library Range loop requires exactly one `.name : Type` "
              "binding."_view);
          return False;
        }

        auto selected_type = TypeReference::parse(source, entry);
        BAIL_IF(!selected_type);
        name = *selected_name;
        type_reference = *selected_type;
        entries++;
        return True;
      });
  BAIL_IF(!binding_end);
  if (entries != 1 || !name || !type_reference) {
    transaction.create_expression_error(
        Span(opening, *binding_end),
        "A Library Range loop requires exactly one named binding."_view,
        "Use `[.name : Type]` before the `in` keyword."_view);
    return {};
  }

  BAIL_IF(!transaction.require(
      Code::Type::In,
      "Library Range loop bindings require the `in` keyword."_view));

  auto range = Parser::Expression::parse(domain, source, transaction);
  BAIL_IF(!range);

  View::Bytes spelling =
      domain.proxy(name->caculate_text(transaction.get_source_text()));
  RangeLoop& loop = domain.construct_from<RangeLoop>([&]() -> RangeLoop {
    return RangeLoop(
        lexical_context, *name, spelling, *type_reference, *range,
        Anchor::create(*name, Span(opening, transaction.peek(-1))));
  });

  auto body = Block::interpret(
      domain, source, transaction, loop, function, access_scope);
  BAIL_IF(!body);
  loop.body = Reference<Block>(*body);
  loop.anchor = Anchor::create(*name, Span(opening, transaction.peek(-1)));

  cursor.join(transaction);
  return loop;
}

auto Language::Flow::RangeLoop::link(
    Tetrodotoxin::Language::Monograph& source,
    const Type& access_scope) -> Bool {
  if (linked) {
    return True;
  }
  BAIL_IF(!body);

  auto composite = access_scope.select<Language::Types::Composite>();
  if (!composite) {
    source.report(
        anchor, "Range loop binding requires one Composite access scope."_view,
        "Retain the Function host Type while linking its Block."_view);
    return False;
  }

  const Abstract& selected = composite->resolve_type(type_reference);
  const Abstract& resolved =
      selected.is<Type>() ? selected : selected.resolve();
  auto selected_type = resolved.select<Type>();
  if (!selected_type || selected_type->get_layout().is_empty()) {
    source.report(
        type_reference.get_anchor(),
        "Range loop binding Type did not resolve to one addressable Type."_view,
        "Use one completed nonempty Type for the loop binding."_view);
    return False;
  }
  if (type && &type->get() != &*selected_type) {
    source.report(
        type_reference.get_anchor(),
        "Range loop binding selected a different Type identity."_view,
        "Repeat linking with the same completed declaration graph."_view);
    return False;
  }

  Model::Pack& retained_range = range.get();
  BAIL_IF(!retained_range.link(source, lexical_context, access_scope));
  const Abstract& range_type = retained_range.get_type().resolve();
  auto typed_range = range_type.select<Language::Types::Range>();
  if (!typed_range || &typed_range->get_element_type() != &*selected_type) {
    source.report(
        anchor,
        "Range loop input must be one Range with the binding's exact Type."_view,
        "Match the declared binding Type to both Range endpoints."_view);
    return False;
  }

  type = Reference<const Type>(*selected_type);
  BAIL_IF(!body->get().link(source));

  linked = True;
  return True;
}

auto Language::Flow::RangeLoop::finalize() -> void {
  range.get().finalize();
  body.visit(
      []() {}, [](Reference<Block>& selected) { selected.get().finalize(); });
}

auto Language::Flow::RangeLoop::resolve() const -> const Abstract& {
  return type ? static_cast<const Abstract&>(*this)
              : static_cast<const Abstract&>(Invalid::get_invalid());
}

auto Language::Flow::RangeLoop::resolve_context(View::Bytes route) const
    -> const Abstract& {
  if (route == name) {
    return resolve();
  }

  return lexical_context.resolve_context(route);
}
