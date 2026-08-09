// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/address.hpp"

#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto select_addressable(const Layout& layout, Core::View::Bytes route)
    -> Core::Option<const Addressable&> {
  Core::Option<const Abstract&> selected;

  // Name uniqueness is proven before the category check. A Layout with two
  // matching entries is ambiguous even when only one resolves to Addressable.
  for (Count i = 0; i < layout.get_size(); i++) {
    auto entry = layout.get_abstract(i);
    if (!entry || entry->get_name() != route) {
      continue;
    }

    if (selected) {
      return {};
    }

    selected = *entry;
  }

  if (!selected || route.is_empty()) {
    return {};
  }

  const Abstract& resolved = selected->resolve();
  return resolved.select<Addressable>();
}

static auto is_readable(const Addressable& selected, const Abstract& requester)
    -> Bool {
  return selected.visit<Language::Field>(
      [&](const Language::Field& field) {
        return field.get_host().visit<Language::Types::Structure>(
            [&](const Language::Types::Structure& structure) {
              return structure.is_readable(field, requester);
            },
            [](const Abstract&) { return False; });
      },
      [](const Abstract&) { return True; });
}

auto Language::Access::Address::parse(
    Memory::Allocator::Arena& domain,
    Materializations&,
    Cursor& cursor,
    const Abstract&,
    Expression& receiver) -> Core::Option<Expression&> {
  Token operation = cursor.consume();
  Token addressable = cursor.require(
      Code::Type::Addressable,
      "Address requires one addressable name after `.`."_view);
  if (!addressable) {
    return {};
  }

  const auto& receiver_anchor = receiver.get_anchor();
  if (!receiver_anchor) {
    cursor.create_expression_error(
        Anchor::create(addressable, Span(operation, addressable)),
        "Address requires an authored receiver Anchor."_view);
    return {};
  }

  Core::View::Bytes route = addressable.caculate_text(cursor.get_source_text());
  Anchor anchor = Anchor::create(
      addressable, receiver_anchor->get_span(), Span(addressable));
  return create_authored(domain, route, receiver, anchor);
}

auto Language::Access::Address::create_authored(
    Memory::Allocator::Arena& domain,
    Core::View::Bytes route,
    Expression& receiver,
    Anchor anchor) -> Address& {
  return Expression::create_authored<Address>(
      domain, anchor, [&](auto source) -> Address {
        return Address(route, receiver, {}, source);
      });
}

auto Language::Access::Address::create_synthetic(
    Memory::Allocator::Arena& domain,
    Expression& receiver,
    const Addressable& selected) -> Address& {
  Core::Option<Reference<const Addressable>> addressable{
    Reference<const Addressable>(selected),
  };
  return Expression::create_synthetic<Address>(
      domain, [&](auto source) -> Address {
        return Address(selected.get_name(), receiver, addressable, source);
      });
}

auto Language::Access::Address::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& context,
    Materializations& materializations) -> Bool {
  if (!receiver.link(source, context, materializations)) {
    return False;
  }

  auto source_anchor = get_anchor();
  const Abstract& receiver_type = receiver.get_type().resolve();
  return receiver_type.visit<Ttx::Model::Type>(
      [&](const Ttx::Model::Type& type) {
        auto selected = select_addressable(type.get_layout(), route);
        if (!selected || !is_readable(*selected, context)) {
          source.report(
              source_anchor,
              "Address did not find one readable Addressable in the receiver "
              "Layout."_view,
              "Select one Addressable admitted by this receiver Type."_view);
          return False;
        }

        if (addressable && &addressable->get() != &*selected) {
          source.report(
              source_anchor,
              "Address cannot change its selected Addressable."_view,
              "Keep one exact Addressable bound to this authored route."_view);
          return False;
        }

        addressable = Reference<const Addressable>(*selected);
        return Expression::link(source, context, materializations);
      },
      [&](const Abstract&) {
        source.report(
            source_anchor, "Address receiver did not resolve to one Type."_view,
            "Use address access only on a receiver with a named Layout."_view);
        return False;
      });
}

auto Language::Access::Address::get_documentation() const
    -> const Documentation& {
  return addressable.visit(
      []() -> const Documentation& { return Documentation::get_empty(); },
      [](const Reference<const Addressable>& selected) -> const Documentation& {
        return selected.get().get_documentation();
      });
}

auto Language::Access::Address::get_type() const -> const Abstract& {
  return addressable.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Addressable>& selected) -> const Abstract& {
        return selected.get().get_type().resolve();
      });
}

auto Language::Access::Address::get_inputs() const -> const Layout& {
  return inputs;
}

auto Language::Access::Address::get_addressable() const
    -> Core::Option<const Addressable&> {
  return addressable.visit(
      []() -> Core::Option<const Addressable&> { return {}; },
      [](const Reference<const Addressable>& selected)
          -> Core::Option<const Addressable&> { return selected.get(); });
}
