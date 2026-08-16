// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/address.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

auto Language::Access::Address::parse(
    const Abstract&,
    Cursor& cursor,
    Expression& receiver) -> Core::Option<Expression&> {
  Memory::Allocator::Arena& domain = cursor.get_arena();
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

  // The source transaction Arena retains this spelling for delayed lookup.
  Core::View::Bytes name = addressable.caculate_text(cursor.get_source_text());
  Anchor anchor = Anchor::create(
      addressable, receiver_anchor->get_span(), Span(addressable));
  return Expression::create_authored<Address>(
      domain, anchor, [&](auto source) -> Address {
        return Address(receiver, addressable, name, {}, source);
      });
}

auto Language::Access::Address::create_synthetic(
    Memory::Allocator::Arena& domain,
    Expression& receiver,
    const Language::Model::Addressable& selected) -> Address& {
  Core::Option<Reference<const Language::Model::Addressable>> addressable{
    Reference<const Language::Model::Addressable>(selected),
  };
  return Expression::create_synthetic<Address>(
      domain, [&](auto source) -> Address {
        return Address(receiver, {}, selected.get_name(), addressable, source);
      });
}

auto Language::Access::Address::link(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& lexical_context,
    Core::Option<const Abstract&> access_scope) -> Bool {
  BAIL_IF(!receiver.link(cursor, lexical_context, access_scope));

  auto source_anchor = get_anchor();
  if (!name_token && addressable) {
    // Swizzle owns selection from arbitrary value flow. Its synthetic Address
    // already carries the exact selected Field, so it does not enter the
    // authored `.` receiver rules.
    return Expression::link(cursor, lexical_context, access_scope);
  }

  const Abstract& receiver_result = receiver.get_result();
  const Abstract& host = access_scope.visit(
      [&]() -> const Abstract& { return lexical_context; },
      [](const Abstract& selected) -> const Abstract& { return selected; });
  const Abstract& candidate = receiver_result.visit<Language::Model::Type>(
      [&](const Language::Model::Type& type) -> const Abstract& {
        return type.resolve_type_access(
            host, name, Language::Model::Type::Access::Static);
      },
      [&](const Abstract& receiver) -> const Abstract& {
        return receiver.resolve_access(host, name);
      });
  auto selected = candidate.resolve().select<Language::Model::Addressable>();

  if (!selected) {
    cursor.create_expression_error(
        source_anchor,
        "Address did not find one readable Addressable for this receiver "
        "identity."_view,
        "Use the receiver's exact contextual Addressable."_view);
    return False;
  }

  if (addressable && &addressable->get() != &*selected) {
    cursor.create_expression_error(
        source_anchor, "Address cannot change its selected Addressable."_view,
        "Keep one exact Addressable bound to this authored Token."_view);
    return False;
  }

  addressable = Reference<const Language::Model::Addressable>(*selected);
  return Expression::link(cursor, lexical_context, access_scope);
}

auto Language::Access::Address::get_documentation() const
    -> const Documentation& {
  return addressable.visit(
      []() -> const Documentation& { return Documentation::get_empty(); },
      [](const Reference<const Language::Model::Addressable>& selected)
          -> const Documentation& {
        return selected.get().get_documentation();
      });
}

auto Language::Access::Address::get_type() const -> const Abstract& {
  return addressable.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Language::Model::Addressable>& selected)
          -> const Abstract& { return selected.get().get_type(); });
}

auto Language::Access::Address::get_result() const -> const Abstract& {
  return addressable.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Language::Model::Addressable>& selected)
          -> const Abstract& { return selected.get(); });
}

auto Language::Access::Address::finalize(Cursor& cursor) -> void {
  receiver.finalize(cursor);
  Expression::finalize(cursor);
}
