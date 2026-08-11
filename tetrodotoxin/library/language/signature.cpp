// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/signature.hpp"

#include "tetrodotoxin/library/language/parameter.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
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

struct ParsedSlot {
  Option<Language::Access::Type> type_access;
  Anchor anchor;
  View::Bytes name;
  Option<Anchor> name_anchor;
};

static auto parse_shape(
    Cursor& cursor,
    Managed::Vector<ParsedSlot>& slots,
    Bool admits_self) -> Bool {
  if (!cursor.matches(Code::Type::BracketStart)) {
    auto type = Language::Access::Type::parse(cursor);
    BAIL_IF(!type);

    slots.insert(
        ParsedSlot{
          .type_access = *type,
          .anchor = type->get_anchor(),
        });
    return True;
  }

  cursor.consume();
  if (cursor.matches(Code::Type::BracketEnd)) {
    cursor.consume();
    return True;
  }

  const Bool named = cursor.matches(Code::Type::AddressOp) ||
                     (admits_self && cursor.matches(Code::Type::Self));
  if (!named && !cursor.matches(Code::Type::Type)) {
    cursor.create_token_error(
        "Library Signature slots must be Types, a leading `self`, or named "
        "`.value : Type` declarations."_view);
    return False;
  }

  // The first slot fixes the complete shape. This grammar decision remains a
  // source fact and never needs a provisional semantic Layout.
  while (!cursor.matches(Code::Type::BracketEnd)) {
    Token opening = cursor.current();
    Token name_token;
    View::Bytes name;
    Option<Anchor> name_anchor;
    if (named) {
      if (cursor.matches(Code::Type::Self)) {
        if (!admits_self || !slots.is_empty()) {
          cursor.create_token_error(
              "Library `self` must be the first Function parameter."_view);
          return False;
        }

        Token self = cursor.consume();
        Anchor self_anchor = Anchor::create(Span(self));
        slots.insert(
            ParsedSlot{
              .type_access = {},
              .anchor = self_anchor,
              .name = "self"_view,
              .name_anchor = self_anchor,
            });
        if (cursor.matches(Code::Type::BracketEnd)) {
          break;
        }
        BAIL_IF(!cursor.require(
            Code::Type::PackingOp,
            "Library Signature slots require `,` or the closing `]`."_view));
        if (cursor.matches(Code::Type::BracketEnd)) {
          break;
        }

        continue;
      }

      if (!cursor.matches(Code::Type::AddressOp)) {
        cursor.create_token_error(
            "Named and unnamed slots cannot share one Library Signature."_view);
        return False;
      }

      cursor.consume();
      name_token = cursor.require(
          Code::Type::Addressable,
          "Named Library Signature slots require a name after `.`."_view);
      BAIL_IF(!name_token);

      name = name_token.caculate_text(cursor.get_source_text());
      if (slots.get_view().contains(
              [&](const ParsedSlot& slot) { return slot.name == name; })) {
        cursor.create_token_error(
            name_token, "Duplicate name in one Library Signature."_view);
        return False;
      }

      name_anchor = Anchor::create(Span(name_token));
      BAIL_IF(!cursor.require(
          Code::Type::Define,
          "Named Library Signature slots require `:` before the "
          "Type."_view));
    } else if (cursor.matches(Code::Type::AddressOp)) {
      cursor.create_token_error(
          "Named and unnamed slots cannot share one Library Signature."_view);
      return False;
    }

    auto type = Language::Access::Type::parse(cursor);
    BAIL_IF(!type);

    Anchor slot_anchor = type->get_anchor();
    if (name_anchor) {
      slot_anchor = Anchor::create(
          name_token, Span(opening, type->get_anchor().get_span().get_end()));
    }
    slots.insert(
        ParsedSlot{
          .type_access = *type,
          .anchor = slot_anchor,
          .name = name,
          .name_anchor = name_anchor,
        });
    if (cursor.matches(Code::Type::BracketEnd)) {
      break;
    }

    BAIL_IF(!cursor.require(
        Code::Type::PackingOp,
        "Library Signature slots require `,` or the closing `]`."_view));

    if (cursor.matches(Code::Type::BracketEnd)) {
      break;
    }
  }

  return Bool(cursor.require(
      Code::Type::BracketEnd,
      "Library Signatures require a closing `]`."_view));
}

Language::Signature::Signature(Allocator::Arena& domain)
    : domain(domain), parameters(domain), results(domain) {}

auto Language::Signature::interpret(Allocator::Arena& domain, Cursor& cursor)
    -> Option<Signature&> {
  auto transaction = cursor.branch();
  Token opening = transaction.current();
  Managed::Vector<ParsedSlot> parsed_parameters(domain);
  Managed::Vector<ParsedSlot> parsed_results(domain);
  BAIL_IF(!parse_shape(transaction, parsed_parameters, True));

  Token arrow = transaction.require(
      Code::Type::CallOp,
      "Library Function parameters require `->` before the result "
      "Signature."_view);
  BAIL_IF(!arrow || !parse_shape(transaction, parsed_results, False));

  Signature& signature = domain.construct_from<Signature>(
      [&]() -> Signature { return Signature(domain); });
  for (Count i = 0; i < parsed_parameters.get_size(); i++) {
    const ParsedSlot& slot = parsed_parameters[i];
    signature.parameters.insert(
        Slot(slot.type_access, slot.anchor, slot.name, slot.name_anchor));
  }
  for (Count i = 0; i < parsed_results.get_size(); i++) {
    const ParsedSlot& slot = parsed_results[i];
    signature.results.insert(
        Slot(slot.type_access, slot.anchor, slot.name, slot.name_anchor));
  }

  signature.anchor = Anchor::create(arrow, Span(opening, transaction.peek(-1)));
  cursor.join(transaction);
  return signature;
}

auto Language::Signature::link(
    Tetrodotoxin::Language::Monograph& source,
    const Type& host) -> Bool {
  if (linked) {
    return True;
  }

  auto context = host.select<Language::Types::Composite>();
  if (!context) {
    source.report(
        anchor,
        "Function Signature requires one exact Composite host Type."_view,
        "Retain the Function on the Composite that owns its Definition."_view);
    return False;
  }

  Bool failed = False;
  auto link_slots = [&](Managed::Vector<Slot>& slots, Bool parameters) {
    for (Count i = 0; i < slots.get_size(); i++) {
      Slot& slot = slots[i];
      Bool type_linked = slot.type_access.visit(
          [&]() {
            if (!parameters || i != 0) {
              return False;
            }

            // The only parameter without an authored Type route is reserved
            // `self`. Its Type is necessarily the Function's exact host, so
            // accepting another edge would represent an impossible signature.
            const Type& type = host;
            if (slot.type) {
              return Bool(&slot.type->get() == &type);
            }

            slot.type = Reference<const Type>(type);
            return True;
          },
          [&](const Access::Type& type_access) {
            // Signature Types use Definition hosting directly. Function
            // completion state cannot widen or narrow the declaration scope,
            // and qualified traversal retains this exact host as its caller.
            const Abstract& selected = context->resolve_type(type_access);
            return selected.visit<Type>(
                [&](const Type& type) {
                  if (slot.type) {
                    return Bool(&slot.type->get() == &type);
                  }

                  slot.type = Reference<const Type>(type);
                  return True;
                },
                [](const Abstract&) { return False; });
          });
      if (!type_linked) {
        Anchor type_anchor = slot.type_access.visit(
            [&]() { return slot.anchor; },
            [](const Access::Type& type_access) {
              return type_access.get_anchor();
            });
        source.report(
            type_anchor,
            "Function signature Type route did not resolve to one stable "
            "Type."_view,
            "Publish the named Type in this logical context before "
            "linking."_view);
        failed = True;
      }
    }
  };

  link_slots(parameters, True);
  link_slots(results, False);
  BAIL_IF(failed);

  auto construct_layout = [&](Managed::Vector<Slot>& slots,
                              Bool parameters) -> Option<const Layout&> {
    Bool named = False;
    Managed::Vector<Reference<const Abstract>> edges(domain);
    for (Count i = 0; i < slots.get_size(); i++) {
      Slot& slot = slots[i];
      BAIL_IF(!slot.type);

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
        BAIL_IF(!parameter);

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

  auto linked_parameters = construct_layout(parameters, True);
  auto linked_results = construct_layout(results, False);
  if (!linked_parameters || !linked_results) {
    source.report(
        anchor,
        "Function could not construct its linked signature Layouts."_view,
        "Keep every named Parameter backed by its authored Signature "
        "slot."_view);
    return False;
  }

  parameter_layout = *linked_parameters;
  result_layout = *linked_results;
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

    auto parameter = selected->select<Addressable>();
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

auto Language::Signature::get_parameter_type_access(Count index) const
    -> Option<const Access::Type&> {
  BAIL_IF(index >= parameters.get_size());

  return parameters.at(index).type_access.visit(
      []() -> Option<const Access::Type&> { return {}; },
      [](const Access::Type& access) -> Option<const Access::Type&> {
        return access;
      });
}

auto Language::Signature::get_result_type_access(Count index) const
    -> Option<const Access::Type&> {
  BAIL_IF(index >= results.get_size());

  return results.at(index).type_access.visit(
      []() -> Option<const Access::Type&> { return {}; },
      [](const Access::Type& access) -> Option<const Access::Type&> {
        return access;
      });
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
  BAIL_IF(index >= parameters.get_size());

  const Slot& slot = parameters.at(index);
  return slot.type_access.visit(
      [&]() -> Option<Anchor> { return slot.anchor; },
      [](const Access::Type& access) -> Option<Anchor> {
        return access.get_anchor();
      });
}

auto Language::Signature::get_result_type_anchor(Count index) const
    -> Option<Anchor> {
  BAIL_IF(index >= results.get_size());

  const Slot& slot = results.at(index);
  return slot.type_access.visit(
      [&]() -> Option<Anchor> { return slot.anchor; },
      [](const Access::Type& access) -> Option<Anchor> {
        return access.get_anchor();
      });
}

auto Language::Signature::get_parameter_type(Count index) const
    -> Option<const Type&> {
  BAIL_IF(index >= parameters.get_size());

  return parameters.at(index).type.visit(
      []() -> Option<const Type&> { return {}; },
      [](const Reference<const Type>& type) -> Option<const Type&> {
        return type.get();
      });
}

auto Language::Signature::get_result_type(Count index) const
    -> Option<const Type&> {
  BAIL_IF(index >= results.get_size());

  return results.at(index).type.visit(
      []() -> Option<const Type&> { return {}; },
      [](const Reference<const Type>& type) -> Option<const Type&> {
        return type.get();
      });
}
