// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/flow/range_loop.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"
#include "tetrodotoxin/library/language/model/parser/layout.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/types/contiguous.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

auto Language::Flow::RangeLoop::interpret(
    Cursor& cursor,
    Block& lexical_context,
    Language::Model::Callable& function,
    const Language::Model::Type& access_scope) -> Option<RangeLoop&> {
  Allocator::Arena& domain = cursor.get_arena();
  Token opening = cursor.require(
      Code::Type::For, "Library for loops require the `for` keyword."_view);
  BAIL_IF(!opening);

  if (!cursor.matches(Code::Type::BracketStart)) {
    cursor.create_token_error(
        "Library for loop bindings require one bracketed named entry."_view);
    return {};
  }

  Option<Token> name;
  Option<TypeReference> type_reference;
  Count entries = 0;
  // The binding borrows the ordinary named Layout grammar but retains its
  // TypeReference until link. Parsing does not require the declaration graph
  // to be complete merely because the loop body needs a lexical name.
  auto binding_end = Model::Parser::Layout::parse(
      cursor,
      [&](Cursor& entry, Count index, Option<Token> selected_name) -> Bool {
        if (index != 0 || !selected_name ||
            selected_name->get_code() != Code::Type::Addressable) {
          entry.create_token_error(
              "A Library for loop requires exactly one `.name : Type` "
              "binding."_view);
          return False;
        }

        auto selected_type = TypeReference::parse(lexical_context, entry);
        BAIL_IF(!selected_type);
        name = *selected_name;
        type_reference = *selected_type;
        entries++;
        return True;
      });
  BAIL_IF(!binding_end);
  if (entries != 1 || !name || !type_reference) {
    cursor.create_expression_error(
        Span(opening, *binding_end),
        "A Library for loop requires exactly one named binding."_view,
        "Use `[.name : Type]` before the `in` keyword."_view);
    return {};
  }

  BAIL_IF(!cursor.require(
      Code::Type::In,
      "Library for loop bindings require the `in` keyword."_view));

  auto input = Parser::Expression::parse(lexical_context, cursor);
  BAIL_IF(!input);

  View::Bytes spelling = name->caculate_text(cursor.get_source_text());
  RangeLoop& loop = domain.construct_from<RangeLoop>([&]() -> RangeLoop {
    return RangeLoop(
        lexical_context, *name, spelling, *type_reference, *input,
        Anchor::create(*name, Span(opening, cursor.peek(-1))));
  });

  // The loop must exist before its body because it is the lexical owner of the
  // new binding. The body receives that real identity rather than a temporary
  // scope map that would need to be reconciled after parsing.
  auto body = Block::interpret(
      cursor, loop, function, access_scope, Reference<const Abstract>(loop));
  BAIL_IF(!body);
  loop.body = Reference<Block>(*body);
  loop.anchor = Anchor::create(*name, Span(opening, cursor.peek(-1)));

  return loop;
}

auto Language::Flow::RangeLoop::link(
    Ttx::Lexical::Cursor& cursor,
    const Language::Model::Type& access_scope) -> Bool {
  if (linked) {
    return True;
  }
  BAIL_IF(!body);

  auto selected = type_reference.resolve_authored(cursor, lexical_context);
  BAIL_IF(!selected);
  auto selected_type = selected->select<Language::Model::Type>();
  if (!selected_type || selected_type->get_layout().is_empty()) {
    cursor.create_expression_error(
        type_reference.get_anchor(),
        "For loop binding Type did not resolve to one addressable Type."_view,
        "Use one completed nonempty Type for the loop binding."_view);
    return False;
  }
  if (type && &type->get() != &*selected_type) {
    cursor.create_expression_error(
        type_reference.get_anchor(),
        "For loop binding selected a different Type identity."_view,
        "Repeat linking with the same completed declaration graph."_view);
    return False;
  }

  Model::Pack& retained_input = input.get();
  // The authored binding and input element must select the exact same Type.
  // Layout compatibility alone would permit a loop variable to change identity
  // when two Types happen to share a representation.
  BAIL_IF(!retained_input.link(cursor, lexical_context, access_scope));
  const Abstract& input_type = retained_input.get_type().resolve();
  auto typed_range = input_type.select<Language::Types::Range>();
  auto typed_contiguous = input_type.select<Language::Types::Contiguous>();
  const Language::Model::Type* element =
      typed_range        ? &typed_range->get_element_type()
      : typed_contiguous ? &typed_contiguous->get_element_type()
                         : nullptr;
  if (element == nullptr || element != &*selected_type) {
    cursor.create_expression_error(
        anchor,
        "For loop input must be one Range or contiguous value with the "
        "binding's exact Type."_view,
        "Match the declared binding Type to the input element Type."_view);
    return False;
  }

  type = Reference<const Language::Model::Type>(*selected_type);
  BAIL_IF(!body->get().link(cursor));

  linked = True;
  return True;
}

auto Language::Flow::RangeLoop::finalize(Cursor& cursor) -> void {
  input.get().finalize(cursor);
  body.visit(
      []() {},
      [&](Reference<Block>& selected) { selected.get().finalize(cursor); });
}

auto Language::Flow::RangeLoop::lower(Llvm::Builder& target) const -> Bool {
  Bool input_lowered = input.get().lower(target);
  if (!input_lowered) {
    return False;
  }

  Bool began = target.begin_range(*this, input.get(), anchor);
  if (!began) {
    return False;
  }

  Bool body_lowered = body->get().lower(target);
  if (!body_lowered) {
    return False;
  }

  return target.end_range(*this);
}

auto Language::Flow::RangeLoop::resolve() const -> const Abstract& {
  return type ? static_cast<const Abstract&>(*this)
              : static_cast<const Abstract&>(Invalid::get_invalid());
}

auto Language::Flow::RangeLoop::resolve_context(View::Bytes route) const
    -> const Abstract& {
  // The loop contributes one local binding and delegates every other query to
  // its real lexical parent. No copied inventory can drift from Block lookup.
  if (route == name) {
    return resolve();
  }

  return lexical_context.resolve_context(route);
}
