// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/function.hpp"

#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

using Tetrodotoxin::Language::Visibility;

static const Layouts::Fluid incomplete_layout;

static auto get_native_attribute_anchor(
    View::Vector<Tetrodotoxin::Language::Attribute> attributes)
    -> Option<Anchor> {
  for (Count i = 0; i < attributes.get_size(); i++) {
    const auto& attribute = attributes.get_data()[i];
    if (attribute.get_key() == "abi"_view ||
        attribute.get_key() == "symbol"_view) {
      return attribute.get_anchor();
    }
  }

  return {};
}

static auto validate_attributes(
    Cursor& cursor,
    Visibility visibility,
    View::Vector<Tetrodotoxin::Language::Attribute> attributes) -> Bool {
  // Function validates only its local authored requests. Target carriers and
  // the complete symbol set remain facts for later compilation owners.
  Bool abi = False;
  Option<Anchor> symbol_anchor;
  for (Count i = 0; i < attributes.get_size(); i++) {
    const Tetrodotoxin::Language::Attribute& attribute =
        attributes.get_data()[i];
    const View::Bytes* value = attribute.get_value().find<View::Bytes>();
    if (attribute.get_key() == "abi"_view) {
      if (abi) {
        cursor.create_expression_error(
            attribute.get_anchor(),
            "A Function cannot repeat its `abi` request."_view);
        return False;
      }
      if (!value || *value != "C"_view) {
        cursor.create_expression_error(
            attribute.get_anchor(),
            "Function `abi` requires the exact supported string `C`."_view);
        return False;
      }
      abi = True;
      continue;
    }

    if (attribute.get_key() == "symbol"_view) {
      if (symbol_anchor) {
        cursor.create_expression_error(
            attribute.get_anchor(),
            "A Function cannot repeat its `symbol` request."_view);
        return False;
      }
      if (!value || value->is_empty()) {
        cursor.create_expression_error(
            attribute.get_anchor(),
            "Function `symbol` requires one nonempty string."_view);
        return False;
      }
      symbol_anchor = attribute.get_anchor();
      continue;
    }
  }

  auto native_anchor = get_native_attribute_anchor(attributes);
  if (native_anchor && visibility != Visibility::Public) {
    cursor.create_expression_error(
        *native_anchor,
        "Native publication Attributes require a public Function."_view);
    return False;
  }
  if (symbol_anchor && !abi) {
    cursor.create_expression_error(
        *symbol_anchor,
        "Function `symbol` requires an accompanying `abi` request."_view);
    return False;
  }

  return True;
}

static auto validate_authored_function(
    Cursor& cursor,
    const Tetrodotoxin::Language::Definition& definition) -> Bool {
  if (definition.get_name_token().get_code() != Code::Type::Addressable) {
    cursor.create_token_error(
        definition.get_name_token(),
        "Library Functions require an authored addressable name."_view);
    return False;
  }

  if (definition.get_visibility() == Visibility::Exposed) {
    cursor.create_token_error(
        definition.get_visibility_token(),
        "Library Functions accept only `public` or `private` visibility."_view);
    return False;
  }
  if (!definition.get_modifiers().is_empty()) {
    cursor.create_token_error(
        definition.get_modifiers().get_data()[0],
        "Library Functions do not accept evaluation modifiers."_view);
    return False;
  }

  return validate_attributes(
      cursor, definition.get_visibility(), definition.get_attributes());
}

static auto get_unreachable_publication(const Language::Function& function)
    -> Option<Anchor> {
  if (!function.get_definition().is_published()) {
    return {};
  }

  auto host = function.get_host().select<Language::Types::Composite>();
  auto signature = function.get_signature();
  if (!host || !signature) {
    Token name = function.get_definition().get_name_token();
    return name ? Option<Anchor>(Anchor::create(Span(name))) : Option<Anchor>();
  }

  Bool receives_self = function.is_type_bound(function.get_host());
  for (Count i = 0; i < signature->get_parameter_size(); i++) {
    if (i == 0 && receives_self) {
      continue;
    }

    auto type = signature->get_parameter_type(i);
    auto access = signature->get_parameter_type_reference(i);
    if (!type || !access || &host->resolve_exported_type(*access) != &*type) {
      return signature->get_parameter_type_anchor(i);
    }
  }
  for (Count i = 0; i < signature->get_result_size(); i++) {
    auto type = signature->get_result_type(i);
    auto access = signature->get_result_type_reference(i);
    if (!type || !access || &host->resolve_exported_type(*access) != &*type) {
      return signature->get_result_type_anchor(i);
    }
  }

  return {};
}

auto Language::Function::reserve(
    Allocator::Arena& domain,
    Cursor& cursor,
    Tetrodotoxin::Language::Definition& definition) -> Option<Function&> {
  auto transaction = cursor.branch();
  BAIL_IF(!validate_authored_function(transaction, definition));
  BAIL_IF(!definition.get_host().is<Language::Types::Composite>());

  BAIL_IF(!transaction.require(
      Code::Type::Func,
      "Library Function definitions require the `func` qualifier."_view));
  BAIL_IF(!transaction.require(
      Code::Type::Assign,
      "Library Function qualifiers require `=` before their signature."_view));

  Function& function = domain.construct_from<Function>(
      [&]() -> Function { return Function(domain, definition); });
  cursor.join(transaction);
  return function;
}

Language::Function::Function(
    Allocator::Arena& domain,
    Tetrodotoxin::Language::Definition& definition)
    : Base(definition), domain(domain) {}

auto Language::Function::complete(
    Cursor& cursor,
    Materializations& materializations) -> Bool {
  if (is_complete()) {
    cursor.create_token_error(
        "A Library Function can be completed only once."_view);
    return False;
  }

  auto transaction = cursor.branch();
  auto parsed_signature = Signature::interpret(domain, transaction);
  BAIL_IF(!parsed_signature);

  // Receiver role is already the signature shape. An exported Function must
  // therefore have no reserved self parameter rather than another role flag.
  const auto& definition = get_definition();
  auto native_anchor = get_native_attribute_anchor(definition.get_attributes());
  if (native_anchor && parsed_signature->get_parameter_size() > 0 &&
      parsed_signature->get_parameter_name(0) == "self"_view) {
    transaction.create_expression_error(
        *native_anchor,
        "Native publication Attributes require a Static Function."_view);
    return False;
  }

  auto parsed_body = Block::interpret(
      domain, materializations, transaction, *this, get_host());
  BAIL_IF(!parsed_body);
  BAIL_IF(!complete_definition(
      definition.get_qualifier(),
      parsed_body->get_anchor().get_span().get_end()));

  // Only complete grammar publishes Signature and Block edges. Failed Arena
  // values remain unreachable from the reserved Function.
  signature = *parsed_signature;
  body = *parsed_body;
  completed = True;
  cursor.join(transaction);
  return True;
}

auto Language::Function::link_signature(
    Tetrodotoxin::Language::Monograph& source) -> Bool {
  if (is_signature_linked()) {
    return True;
  }

  if (!completed || !signature) {
    source.report(
        Anchor::create(
            get_definition().get_qualifier(),
            get_definition().get_anchor().get_span()),
        "An incomplete Function cannot enter semantic linking."_view,
        "Complete its signature and body grammar before linking."_view);
    return False;
  }

  // Signature routes receive the host Type directly. They therefore use the
  // same access authority as the body without making an incomplete Function
  // double as a Type resolution mode switch.
  return signature->link(source, get_host());
}

auto Language::Function::link_body(
    Tetrodotoxin::Language::Monograph& source,
    Materializations& materializations) -> Bool {
  BAIL_IF(!is_signature_linked());
  BAIL_IF(!body);

  // Signature edges publish before Block linking so every Identifier can reach
  // the exact Parameter object created for its authored declaration.
  return body->link(source, materializations);
}

auto Language::Function::finalize(Tetrodotoxin::Language::Monograph& source)
    -> Bool {
  BAIL_IF(!body);

  Bool valid = True;
  auto unreachable = get_unreachable_publication(*this);
  if (unreachable) {
    source.report(
        unreachable,
        "Externally readable Function publishes an unreachable Type route."_view,
        "Keep the Function private or publish its authored Type route."_view);
    valid = False;
  }

  // Optional folding records a cached Constant for later consumers. A dynamic
  // result or failure remains queryable but cannot turn an otherwise complete
  // Function into a semantic failure without a Constant requirement.
  body->finalize();

  return valid;
}

auto Language::Function::resolve() const -> const Abstract& {
  if (!is_signature_linked()) {
    return Invalid::get_invalid();
  }

  return *this;
}

auto Language::Function::resolve_context(View::Bytes route) const
    -> const Abstract& {
  if (signature) {
    const Abstract& parameter = signature->resolve_parameter(route);
    if (&parameter != &Invalid::get_invalid()) {
      return parameter;
    }
  }

  const Type& host = get_host();
  return host.visit<Language::Types::Composite>(
      [&](const Language::Types::Composite& composite) -> const Abstract& {
        // Only a Function hosted directly by Source receives bare Static
        // Addressables. A nested Composite grants access authority, but it
        // never supplies an implicit receiver or leaks Source statics through
        // the Definition host chain.
        // Signature linking uses the explicit Type resolver and never enters
        // this lexical surface. Source Addressables therefore need no lifecycle
        // switch: this owner is always describing body name lookup.
        const Abstract& addressable = composite.visit<Language::Types::Source>(
            [&](const Language::Types::Source& source) -> const Abstract& {
              return source.resolve_lexical_addressable(route, host);
            },
            [](const Abstract&) -> const Abstract& {
              return Invalid::get_invalid();
            });
        if (&addressable != &Invalid::get_invalid()) {
          return addressable;
        }

        return composite.resolve_type_root(route, host);
      },
      [&](const Abstract&) -> const Abstract& {
        return host.resolve_context(route);
      });
}

auto Language::Function::get_parameters() const -> const Layout& {
  return signature.visit(
      []() -> const Layout& { return incomplete_layout; },
      [](const Signature& selected) -> const Layout& {
        return selected.get_parameters();
      });
}

auto Language::Function::get_results() const -> const Layout& {
  return signature.visit(
      []() -> const Layout& { return incomplete_layout; },
      [](const Signature& selected) -> const Layout& {
        return selected.get_results();
      });
}

auto Language::Function::get_signature() const -> Option<const Signature&> {
  return signature.visit(
      []() -> Option<const Signature&> { return {}; },
      [](const Signature& selected) -> Option<const Signature&> {
        return selected;
      });
}

auto Language::Function::get_body() const -> Option<const Block&> {
  return body.visit(
      []() -> Option<const Block&> { return {}; },
      [](const Block& selected) -> Option<const Block&> { return selected; });
}

auto Language::Function::is_signature_linked() const -> Bool {
  return signature.visit(
      []() { return False; },
      [](const Signature& selected) { return selected.is_linked(); });
}
