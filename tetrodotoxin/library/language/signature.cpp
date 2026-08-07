// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/signature.hpp"

#include "tetrodotoxin/library/language/parameter.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static const Layouts::Fluid incomplete_layout;

struct ParsedType {
  View::Bytes route;
  Anchor anchor;
};

struct ParsedSlot {
  View::Bytes route;
  Anchor anchor;
  Anchor type_anchor;
  View::Bytes name;
  Option<Anchor> name_anchor;
};

static auto parse_type(Cursor& cursor) -> Option<ParsedType> {
  Token first = cursor.require(
      Code::Type::Type, "Library Signatures require a Type name."_view);
  if (!first) {
    return {};
  }

  // The authored route stays intact because its eventual context owns
  // qualification. Signature needs the spelling and Anchor while TTX Layout
  // receives only the linked Type identity.
  Token last = first;
  while (cursor.matches(Code::Type::TypeAccessOp)) {
    Token separator = cursor.current();
    Count previous_end = Count(last.get_offset()) + Count(last.get_size());
    if (separator.get_offset() != previous_end) {
      cursor.create_expression_error(
          Span(first, separator),
          "Qualified Type routes cannot contain whitespace around `::`."_view);
      return {};
    }

    cursor.consume();
    Token segment = cursor.require(
        Code::Type::Type,
        "Qualified Type routes require a Type segment after `::`."_view);
    if (!segment) {
      return {};
    }

    Count separator_end =
        Count(separator.get_offset()) + Count(separator.get_size());
    if (segment.get_offset() != separator_end) {
      cursor.create_expression_error(
          Span(first, segment),
          "Qualified Type routes cannot contain whitespace around `::`."_view);
      return {};
    }

    last = segment;
  }

  Count route_start = first.get_offset();
  Count route_end = Count(last.get_offset()) + Count(last.get_size());
  View::Bytes route =
      cursor.get_source_text().slice(route_start, route_end - route_start);
  Span span(first, last);
  return ParsedType{
    .route = route,
    .anchor = Anchor::create(first, span),
  };
}

static auto contains_name(View::Vector<ParsedSlot> slots, View::Bytes candidate)
    -> Bool {
  for (Count i = 0; i < slots.get_size(); i++) {
    if (slots.get_data()[i].name == candidate) {
      return True;
    }
  }

  return False;
}

static auto parse_shape(Cursor& cursor, Managed::Vector<ParsedSlot>& slots)
    -> Bool {
  if (!cursor.matches(Code::Type::LayoutStart)) {
    auto type = parse_type(cursor);
    if (!type) {
      return False;
    }

    slots.insert(
        ParsedSlot{
          .route = type->route,
          .anchor = type->anchor,
          .type_anchor = type->anchor,
        });
    return True;
  }

  cursor.consume();
  if (cursor.matches(Code::Type::LayoutEnd)) {
    cursor.consume();
    return True;
  }

  const Bool named = cursor.matches(Code::Type::AddressOp);
  if (!named && !cursor.matches(Code::Type::Type)) {
    cursor.create_token_error(
        "Library Signature slots must be Types or named `.value : Type` "
        "declarations."_view);
    return False;
  }

  // The first slot fixes the complete shape. This grammar decision remains a
  // source fact and never needs a provisional semantic Layout.
  while (!cursor.matches(Code::Type::LayoutEnd)) {
    Token opening = cursor.current();
    Token name_token;
    View::Bytes name;
    Option<Anchor> name_anchor;
    if (named) {
      if (!cursor.matches(Code::Type::AddressOp)) {
        cursor.create_token_error(
            "Named and unnamed slots cannot share one Library Signature."_view);
        return False;
      }

      cursor.consume();
      name_token = cursor.require(
          Code::Type::Addressable,
          "Named Library Signature slots require a name after `.`."_view);
      if (!name_token) {
        return False;
      }

      name = name_token.caculate_text(cursor.get_source_text());
      if (contains_name(slots, name)) {
        cursor.create_token_error(
            name_token, "Duplicate name in one Library Signature."_view);
        return False;
      }

      name_anchor = Anchor::create(Span(name_token));
      if (!cursor.require(
              Code::Type::Define,
              "Named Library Signature slots require `:` before the "
              "Type."_view)) {
        return False;
      }
    } else if (cursor.matches(Code::Type::AddressOp)) {
      cursor.create_token_error(
          "Named and unnamed slots cannot share one Library Signature."_view);
      return False;
    }

    auto type = parse_type(cursor);
    if (!type) {
      return False;
    }

    Anchor slot_anchor = type->anchor;
    if (name_anchor) {
      slot_anchor = Anchor::create(
          name_token, Span(opening, type->anchor.get_span().get_end()));
    }
    slots.insert(
        ParsedSlot{
          .route = type->route,
          .anchor = slot_anchor,
          .type_anchor = type->anchor,
          .name = name,
          .name_anchor = name_anchor,
        });
    if (cursor.matches(Code::Type::LayoutEnd)) {
      break;
    }

    if (!cursor.require(
            Code::Type::PackingOp,
            "Library Signature slots require `,` or the closing `]`."_view)) {
      return False;
    }

    if (cursor.matches(Code::Type::LayoutEnd)) {
      break;
    }
  }

  return Bool(cursor.require(
      Code::Type::LayoutEnd, "Library Signatures require a closing `]`."_view));
}

Language::Signature::Signature(Allocator::Arena& domain)
    : domain(domain), parameters(domain), results(domain) {}

auto Language::Signature::interpret(Allocator::Arena& domain, Cursor& cursor)
    -> Option<Signature&> {
  auto transaction = cursor.branch();
  Token opening = transaction.current();
  Managed::Vector<ParsedSlot> parsed_parameters(domain);
  Managed::Vector<ParsedSlot> parsed_results(domain);
  if (!parse_shape(transaction, parsed_parameters)) {
    return {};
  }

  Token arrow = transaction.require(
      Code::Type::CallOp,
      "Library Function parameters require `->` before the result "
      "Signature."_view);
  if (!arrow || !parse_shape(transaction, parsed_results)) {
    return {};
  }

  Signature& signature = domain.construct_from<Signature>(
      [&]() -> Signature { return Signature(domain); });
  for (Count i = 0; i < parsed_parameters.get_size(); i++) {
    const ParsedSlot& slot = parsed_parameters[i];
    signature.parameters.insert(Slot(
        slot.route, slot.anchor, slot.type_anchor, slot.name,
        slot.name_anchor));
  }
  for (Count i = 0; i < parsed_results.get_size(); i++) {
    const ParsedSlot& slot = parsed_results[i];
    signature.results.insert(Slot(
        slot.route, slot.anchor, slot.type_anchor, slot.name,
        slot.name_anchor));
  }

  signature.anchor = Anchor::create(arrow, Span(opening, transaction.peek(-1)));
  cursor.join(transaction);
  return signature;
}

auto Language::Signature::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& context) -> Bool {
  if (linked) {
    return True;
  }

  Bool failed = False;
  auto link_slots = [&](Managed::Vector<Slot>& slots, Bool parameters) {
    for (Count i = 0; i < slots.get_size(); i++) {
      Slot& slot = slots[i];
      const Abstract& selected = context.resolve_context(slot.route).resolve();
      Bool type_linked = selected.visit<Type>(
          [&](const Type& type) {
            if (slot.type) {
              return &slot.type->get() == &type ? True : False;
            }

            slot.type = Reference<const Type>(type);
            return True;
          },
          [](const Abstract&) { return False; });
      if (!type_linked) {
        source.report(
            slot.type_anchor,
            "Function signature Type route did not resolve to one stable "
            "Type."_view,
            "Publish the named Type in this logical context before "
            "linking."_view);
        failed = True;
      }

      if (!parameters || !slot.is_named()) {
        continue;
      }

      const Abstract& occupied = context.resolve_context(slot.name);
      if (&occupied != &Invalid::get_invalid()) {
        source.report(
            slot.name_anchor,
            "Function parameter name collides with an occupied parent "
            "name."_view,
            "Choose a local name that does not shadow its parent context."_view);
        failed = True;
      }
    }
  };

  link_slots(parameters, True);
  link_slots(results, False);
  if (failed) {
    return False;
  }

  auto construct_layout = [&](Managed::Vector<Slot>& slots,
                              Bool parameters) -> Option<const Layout&> {
    Bool named = False;
    Managed::Vector<Reference<const Abstract>> edges(domain);
    for (Count i = 0; i < slots.get_size(); i++) {
      Slot& slot = slots[i];
      if (!slot.type) {
        return {};
      }

      const Type& type = slot.type->get();
      named |= slot.is_named();
      if (!slot.is_named()) {
        edges.insert(type);
        continue;
      }

      // Parameter identity belongs to the callable scope. Results need only
      // an Alias because their authored name does not introduce an address.
      if (parameters) {
        auto parameter = Parameter::create_authored(domain, *this, i, type);
        if (!parameter) {
          return {};
        }

        edges.insert(*parameter);
      } else {
        edges.insert(domain.construct<Alias>(slot.name, type));
      }
    }

    if (named) {
      return domain.construct<Layouts::Named>(edges.get_view());
    }

    return domain.construct<Layouts::Fluid>(edges.get_view());
  };

  auto parameters_projection = construct_layout(parameters, True);
  auto results_projection = construct_layout(results, False);
  if (!parameters_projection || !results_projection) {
    source.report(
        anchor,
        "Function could not construct its linked signature Layouts."_view,
        "Keep every named Parameter backed by its authored Signature "
        "slot."_view);
    return False;
  }

  parameter_layout = *parameters_projection;
  result_layout = *results_projection;
  linked = True;
  return True;
}

auto Language::Signature::get_parameters() const -> const Layout& {
  return parameter_layout.visit(
      []() -> const Layout& { return incomplete_layout; },
      [](const Layout& selected) -> const Layout& { return selected; });
}

auto Language::Signature::get_results() const -> const Layout& {
  return result_layout.visit(
      []() -> const Layout& { return incomplete_layout; },
      [](const Layout& selected) -> const Layout& { return selected; });
}

auto Language::Signature::resolve_parameter(View::Bytes route) const
    -> const Abstract& {
  const Layout& layout = get_parameters();
  for (Count i = 0; i < layout.get_size(); i++) {
    auto selected = layout.get_abstract(i);
    if (!selected) {
      continue;
    }

    auto parameter = selected->visit<Addressable>(
        [](const Addressable& addressable) -> Option<const Addressable&> {
          return addressable;
        },
        [](const Abstract&) -> Option<const Addressable&> { return {}; });
    if (parameter && parameter->get_name() == route) {
      return *parameter;
    }
  }

  return Invalid::get_invalid();
}

auto Language::Signature::get_parameter_name(Count index) const -> View::Bytes {
  return index < parameters.get_size() ? parameters.at(index).name
                                       : View::Bytes();
}

auto Language::Signature::get_result_name(Count index) const -> View::Bytes {
  return index < results.get_size() ? results.at(index).name : View::Bytes();
}

auto Language::Signature::get_parameter_anchor(Count index) const
    -> Option<Anchor> {
  return index < parameters.get_size()
             ? Option<Anchor>(parameters.at(index).anchor)
             : Option<Anchor>();
}

auto Language::Signature::get_result_anchor(Count index) const
    -> Option<Anchor> {
  return index < results.get_size() ? Option<Anchor>(results.at(index).anchor)
                                    : Option<Anchor>();
}

auto Language::Signature::get_parameter_type_anchor(Count index) const
    -> Option<Anchor> {
  return index < parameters.get_size()
             ? Option<Anchor>(parameters.at(index).type_anchor)
             : Option<Anchor>();
}

auto Language::Signature::get_result_type_anchor(Count index) const
    -> Option<Anchor> {
  return index < results.get_size()
             ? Option<Anchor>(results.at(index).type_anchor)
             : Option<Anchor>();
}

auto Language::Signature::get_parameter_type(Count index) const
    -> Option<const Type&> {
  if (index >= parameters.get_size()) {
    return {};
  }

  return parameters.at(index).type.visit(
      []() -> Option<const Type&> { return {}; },
      [](const Reference<const Type>& type) -> Option<const Type&> {
        return type.get();
      });
}

auto Language::Signature::get_result_type(Count index) const
    -> Option<const Type&> {
  if (index >= results.get_size()) {
    return {};
  }

  return results.at(index).type.visit(
      []() -> Option<const Type&> { return {}; },
      [](const Reference<const Type>& type) -> Option<const Type&> {
        return type.get();
      });
}
