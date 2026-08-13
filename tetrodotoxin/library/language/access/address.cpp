// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/address.hpp"

#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto select_addressable(const Layout& layout, Core::View::Bytes name)
    -> Core::Option<const Addressable&> {
  Core::Option<const Abstract&> selected;

  // Name uniqueness is proven before the category check. A Layout with two
  // matching entries is ambiguous even when only one resolves to Addressable.
  for (Count i = 0; i < layout.get_size(); i++) {
    auto entry = layout.get_abstract(i);
    if (!entry || entry->get_name() != name) {
      continue;
    }

    BAIL_IF(selected);

    selected = *entry;
  }

  BAIL_IF(!selected || name.is_empty());

  const Abstract& resolved = selected->resolve();
  return resolved.select<Addressable>();
}

static auto select_addressable(
    const Language::Types::Composite& composite,
    Core::View::Bytes name,
    Bool constant_only = False) -> Core::Option<const Addressable&> {
  Core::Option<const Addressable&> selected;
  for (const Reference<Abstract>& candidate : composite.get_addressables()) {
    if (candidate.get().get_name() != name) {
      continue;
    }

    BAIL_IF(selected);
    const Abstract& resolved = candidate.get().resolve();
    auto field = resolved.select<Language::Field>();
    if (constant_only && (!field || field->get_writability() !=
                                        Language::Writability::Constant)) {
      continue;
    }
    selected = resolved.select<Addressable>();
  }
  return selected;
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

              // The receiver identity supplies the selected Field category.
              // Caller scope contributes only authority, so it cannot invent
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
    Language::Monograph&,
    Cursor& cursor,
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

  // Token is the authored selection fact. The Arena-stable spelling exists
  // only because lookup happens after this Cursor transaction has completed.
  Core::View::Bytes name =
      domain.proxy(addressable.caculate_text(cursor.get_source_text()));
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
    const Addressable& selected) -> Address& {
  Core::Option<Reference<const Addressable>> addressable{
    Reference<const Addressable>(selected),
  };
  return Expression::create_synthetic<Address>(
      domain, [&](auto source) -> Address {
        return Address(receiver, {}, selected.get_name(), addressable, source);
      });
}

auto Language::Access::Address::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    Core::Option<const Ttx::Model::Type&> access_scope) -> Bool {
  BAIL_IF(!receiver.link(source, lexical_context, access_scope));

  auto source_anchor = get_anchor();
  if (!name_token && addressable) {
    // Swizzle owns selection from arbitrary value flow. Its synthetic Address
    // already carries the exact selected Field, so it does not re-enter the
    // authored `.` receiver rules.
    BAIL_IF(!is_accessible(addressable->get(), access_scope));
    return Expression::link(source, lexical_context, access_scope);
  }

  Core::Option<const Addressable&> selected;
  const Abstract& receiver_result = receiver.get_result();
  auto source_type = receiver_result.select<Language::Types::Source>();
  if (source_type) {
    selected = select_addressable(*source_type, name);
  } else if (
      auto receiver_addressable = receiver_result.select<Addressable>()) {
    const Abstract& output_type = receiver_addressable->get_type().resolve();
    auto composite = output_type.select<Language::Types::Composite>();
    selected = composite
                   ? select_addressable(*composite, name)
                   : select_addressable(
                         receiver_addressable->get_type().get_layout(), name);
  } else if (auto receiver_type = receiver_result.select<Ttx::Model::Type>()) {
    auto composite = receiver_type->select<Language::Types::Composite>();
    if (composite) {
      selected = select_addressable(*composite, name, True);
    }
  }

  if (!selected || !is_accessible(*selected, access_scope)) {
    source.report(
        source_anchor,
        "Address did not find one readable Field for this receiver identity."_view,
        "Use an Addressable for mutable Fields, a Type for const Fields, or "
        "Source for either category."_view);
    return False;
  }

  if (addressable && &addressable->get() != &*selected) {
    source.report(
        source_anchor, "Address cannot change its selected Addressable."_view,
        "Keep one exact Addressable bound to this authored Token."_view);
    return False;
  }

  addressable = Reference<const Addressable>(*selected);
  return Expression::link(source, lexical_context, access_scope);
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
        return selected.get().get_type();
      });
}

auto Language::Access::Address::get_result() const -> const Abstract& {
  return addressable.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Addressable>& selected) -> const Abstract& {
        return selected.get();
      });
}

auto Language::Access::Address::finalize() -> void {
  receiver.finalize();
  Expression::finalize();
}
