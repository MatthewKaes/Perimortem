// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/function.hpp"

#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

using Tetrodotoxin::Language::Visibility;

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

auto Language::Function::complete(Monograph& source, Cursor& cursor) -> Bool {
  if (is_complete()) {
    cursor.create_token_error(
        "A Library Function can be completed only once."_view);
    return False;
  }

  auto transaction = cursor.branch();
  auto parsed_signature = Signature::interpret(domain, source, transaction);
  BAIL_IF(!parsed_signature);

  // Receiver role is already the signature shape. An exported Function must
  // therefore have no reserved self parameter rather than another role flag.
  const auto& definition = get_definition();
  auto native_anchor = get_native_attribute_anchor(definition.get_attributes());
  if (native_anchor && parsed_signature->declares_self()) {
    transaction.create_expression_error(
        *native_anchor,
        "Native publication Attributes require a Static Function."_view);
    return False;
  }

  auto parsed_body =
      Block::interpret(domain, source, transaction, *this, *this, get_host());
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

auto Language::Function::link_body(Tetrodotoxin::Language::Monograph& source)
    -> Bool {
  BAIL_IF(!is_signature_linked());
  BAIL_IF(!body);

  // Signature edges publish before Block linking so every Identifier can reach
  // the exact Parameter object created for its authored declaration.
  BAIL_IF(!body->link(source));
  if (!get_results().is_empty() && body->reaches_next_statement()) {
    source.report(
        body->get_anchor(),
        "Function result Layout requires a terminal return statement."_view,
        "Return the complete ordered values required by the Function "
        "signature."_view);
    return False;
  }

  return True;
}

auto Language::Function::finalize(Tetrodotoxin::Language::Monograph& source)
    -> Bool {
  BAIL_IF(!body);

  Bool valid = True;
  if (get_definition().is_published()) {
    valid = signature.visit(
        []() { return False; },
        [&](const Signature& selected) {
          return selected.validate_publication(source, get_host());
        });
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
    const Abstract& parameter =
        signature->get_parameters().resolve_named(route);
    if (&parameter != &Invalid::get_invalid()) {
      return parameter;
    }
  }

  const Type& host = get_host();
  return host.visit<Language::Types::Composite>(
      [&](const Language::Types::Composite& composite) -> const Abstract& {
        // Only a Function hosted directly by Source receives bare source
        // Addressables. A nested Composite grants access authority, but it
        // never supplies an implicit receiver or leaks Source bindings through
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
  // Callable Layouts are total only after resolve() proves this Function's
  // Signature. Returning an empty Layout here would launder incomplete state
  // into valid zero value flow, so the lifecycle precondition remains explicit.
  return signature->get_parameters();
}

auto Language::Function::get_results() const -> const Layout& {
  return signature->get_results();
}

auto Language::Function::declares_self() const -> Bool {
  return signature.visit(
      []() { return False; },
      [](const Signature& selected) { return selected.declares_self(); });
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
