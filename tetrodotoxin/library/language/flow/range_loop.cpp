// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/flow/range_loop.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/model/parser/layout.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

Language::Flow::RangeLoop::RangeLoop(
    Allocator::Arena& domain,
    Block& lexical_context,
    View::Vector<AuthoredBinding> source_bindings,
    Language::Model::Pack& input,
    Anchor anchor)
    : domain(domain),
      lexical_context(lexical_context),
      authored_bindings(domain),
      bindings(domain),
      binding_entries(domain),
      input(input),
      anchor(anchor) {
  authored_bindings.reset(source_bindings.get_size());
  bindings.reset(source_bindings.get_size());
  binding_entries.reset(source_bindings.get_size());
  for (const AuthoredBinding& binding : source_bindings) {
    authored_bindings.insert(binding);
  }
}

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
        "Library for loop bindings require one bracketed named Layout."_view);
    return {};
  }

  Managed::Vector<AuthoredBinding> bindings(domain);
  auto binding_end = Model::Parser::Layout::parse(
      cursor, [&](Cursor& entry, Count, Option<Token> selected_name) -> Bool {
        if (!selected_name ||
            selected_name->get_code() != Code::Type::Addressable) {
          entry.create_token_error(
              "A Library for loop binding requires `.name : Type`."_view);
          return False;
        }

        View::Bytes name =
            selected_name->caculate_text(cursor.get_source_text());
        if (bindings.get_view().contains([&](const AuthoredBinding& existing) {
              return existing.name == name;
            })) {
          entry.create_token_error(
              *selected_name,
              "A Library for loop binding name must be unique."_view);
          return False;
        }

        auto selected_type = TypeReference::parse(lexical_context, entry);
        BAIL_IF(!selected_type);
        bindings.insert({*selected_name, name, *selected_type});
        return True;
      });
  BAIL_IF(!binding_end);
  if (bindings.is_empty()) {
    cursor.create_expression_error(
        Span(opening, *binding_end),
        "A Library for loop requires at least one named binding."_view,
        "Use `[.name : Type]` before the `in` keyword."_view);
    return {};
  }

  BAIL_IF(!cursor.require(
      Code::Type::In,
      "Library for loop bindings require the `in` keyword."_view));

  auto input = Parser::Expression::parse(lexical_context, cursor);
  BAIL_IF(!input);

  RangeLoop& loop = domain.construct_from<RangeLoop>([&]() -> RangeLoop {
    return RangeLoop(
        domain, lexical_context, bindings.get_view(), *input,
        Anchor::create(opening, Span(opening, cursor.peek(-1))));
  });

  auto body = Block::interpret(
      cursor, loop, function, access_scope, Reference<const Abstract>(loop));
  BAIL_IF(!body);
  loop.body = Reference<Block>(*body);
  loop.anchor = Anchor::create(opening, Span(opening, cursor.peek(-1)));
  return loop;
}

auto Language::Flow::RangeLoop::link(
    Cursor& cursor,
    const Language::Model::Type& access_scope) -> Bool {
  if (linked) {
    return True;
  }
  BAIL_IF(!body);

  Managed::Vector<Reference<const Language::Model::Type>> selected_types(
      domain);
  selected_types.reset(authored_bindings.get_size());
  for (const AuthoredBinding& binding : authored_bindings.get_view()) {
    const Abstract& shadowed = lexical_context.resolve_context(binding.name);
    if (!shadowed.is<Invalid>()) {
      auto report =
          cursor.create_report(Anchor::create(Span(binding.name_token)));
      report << "Library for binding shadows a reachable lexical binding."_view;
      auto& note = report.get_hint();
      note << "Rename this binding so every enclosing name remains "
              "unambiguous."_view;
      auto original = cursor.get_associations().find(shadowed);
      if (original) {
        Token focus = original->get_token();
        if (!focus) {
          focus = original->get_span().get_start();
        }
        if (focus) {
          note << " Original declaration: "_view << cursor.get_source_path()
               << ":"_view << focus.get_line() << ":"_view << focus.get_column()
               << "."_view;
        }
      }
      return False;
    }

    auto selected =
        binding.type_reference.resolve_authored(cursor, lexical_context);
    BAIL_IF(!selected);
    auto selected_type = selected->select<Language::Model::Type>();
    if (!selected_type || selected_type->get_layout().is_empty()) {
      cursor.create_expression_error(
          binding.type_reference.get_anchor(),
          "For loop binding did not resolve to one nonempty Type."_view,
          "Use one completed value Type for each loop binding."_view);
      return False;
    }

    selected_types.insert(*selected_type);
  }

  if (!binding_layout) {
    for (Count index = 0; index < authored_bindings.get_size(); index++) {
      const AuthoredBinding& source = authored_bindings[index];
      auto binding = Parameter::create_authored(
          domain, source.name, selected_types[index].get());
      BAIL_IF(!binding);
      bindings.insert(*binding);
      binding_entries.insert(*binding);
      cursor.get_associations().create(
          Anchor::create(Span(source.name_token)), *binding);
    }
    binding_layout = Ttx::Model::Layouts::Named(binding_entries.get_view());
  } else {
    BAIL_IF(bindings.get_size() != selected_types.get_size());
    for (Count index = 0; index < bindings.get_size(); index++) {
      BAIL_IF(
          &bindings[index].get().get_type() != &selected_types[index].get());
    }
  }

  Language::Model::Pack& retained_input = input.get();
  BAIL_IF(!retained_input.link(cursor, lexical_context, access_scope));

  Option<const Language::Model::Type&> selected_input;
  const Abstract& value_type = retained_input.get_type().resolve();
  auto direct_type = value_type.select<Language::Model::Type>();
  if (direct_type) {
    selected_input = *direct_type;
  } else {
    auto expression = retained_input.select<Language::Expression>();
    if (expression) {
      selected_input =
          expression->get_result().resolve().select<Language::Model::Type>();
    }
  }

  if (!selected_input || !selected_input->accepts_iteration(*binding_layout)) {
    cursor.create_expression_error(
        anchor,
        "For loop input cannot produce the authored binding Layout."_view,
        "Match the binding names and Types to one iterable input Type."_view);
    return False;
  }

  if (input_type && &input_type->get() != &*selected_input) {
    cursor.create_expression_error(
        anchor,
        "For loop input selected a different iterable Type identity."_view,
        "Repeat linking with the same completed declaration graph."_view);
    return False;
  }

  input_type = Reference<const Language::Model::Type>(*selected_input);
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

auto Language::Flow::RangeLoop::resolve_context(View::Bytes route) const
    -> const Abstract& {
  for (const Reference<Parameter>& binding : bindings.get_view()) {
    if (binding.get().get_name() == route) {
      return binding.get();
    }
  }

  return lexical_context.resolve_context(route);
}
