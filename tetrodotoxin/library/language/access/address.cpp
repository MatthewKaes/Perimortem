// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/address.hpp"

#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
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

    BAIL_IF(selected);

    selected = *entry;
  }

  BAIL_IF(!selected || route.is_empty());

  const Abstract& resolved = selected->resolve();
  return resolved.select<Addressable>();
}

auto Language::Access::Address::is_accessible(
    const Addressable& candidate,
    Core::Option<const Ttx::Model::Type&> access_scope) -> Bool {
  return candidate.visit<Language::Field>(
      [&](const Language::Field& field) {
        return field.get_host().visit<Language::Types::Composite>(
            [&](const Language::Types::Composite& composite) {
              if (field.get_definition().is_published()) {
                return True;
              }

              // The receiver Layout supplies the selected identity. Caller
              // scope contributes only access authority, so it cannot invent
              // a receiver or make an unrelated private Field readable.
              return access_scope.visit(
                  []() { return False; },
                  [&](const Ttx::Model::Type& caller_scope) {
                    return composite.grants_private_access(caller_scope);
                  });
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
  BAIL_IF(!addressable);

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
    const Abstract& lexical_context,
    Materializations& materializations,
    Core::Option<const Ttx::Model::Type&> access_scope) -> Bool {
  BAIL_IF(
      !receiver.link(source, lexical_context, materializations, access_scope));

  auto source_anchor = get_anchor();
  const Abstract& receiver_type = receiver.get_type().resolve();
  return receiver_type.visit<Ttx::Model::Type>(
      [&](const Ttx::Model::Type& type) {
        auto selected = select_addressable(type.get_layout(), route);
        if (!selected || !is_accessible(*selected, access_scope)) {
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
        return Expression::link(
            source, lexical_context, materializations, access_scope);
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
