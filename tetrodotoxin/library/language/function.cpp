// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/function.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
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
    auto access = signature->get_parameter_type_access(i);
    if (!type || !access || &host->resolve_exported_type(*access) != &*type) {
      return signature->get_parameter_type_anchor(i);
    }
  }
  for (Count i = 0; i < signature->get_result_size(); i++) {
    auto type = signature->get_result_type(i);
    auto access = signature->get_result_type_access(i);
    if (!type || !access || &host->resolve_exported_type(*access) != &*type) {
      return signature->get_result_type_anchor(i);
    }
  }

  return {};
}

static auto parse_body(
    Allocator::Arena& domain,
    Language::Materializations& materializations,
    Cursor& cursor,
    const Abstract& context,
    Managed::Vector<Reference<Language::Expression>>& expressions,
    Token& return_token,
    Span& return_span,
    Option<Reference<Language::Expression>>& return_expression,
    Token& closing) -> Bool {
  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart,
      "Library Function signatures require a body beginning with `{`."_view));

  Tetrodotoxin::Language::Parser::Comment::parse(cursor);
  while (!cursor.matches(Code::Type::ScopeEnd)) {
    if (cursor.matches(Code::Type::Terminal)) {
      cursor.create_token_error(
          "Library Function body reached the end of source before `}`."_view);
      return False;
    }

    if (cursor.matches(Code::Type::Return)) {
      // Return is the only retained terminal form. Closing the grammar here
      // keeps later source from appearing reachable without a statement owner.
      Token selected_return = cursor.consume();
      Option<Language::Expression&> selected_expression;
      if (!cursor.matches(Code::Type::EndStatement)) {
        selected_expression = Language::Parser::Expression::parse(
            domain, materializations, cursor, context);
        BAIL_IF(!selected_expression);
      }

      Token terminator = cursor.require(
          Code::Type::EndStatement,
          "Library Function returns require one terminating `;`."_view);
      BAIL_IF(!terminator);

      Tetrodotoxin::Language::Parser::Comment::parse(cursor);
      if (!cursor.matches(Code::Type::ScopeEnd)) {
        cursor.create_token_error(
            "A Library Function return must be the final body form."_view);
        return False;
      }

      closing = cursor.consume();
      return_token = selected_return;
      return_span = Span(selected_return, terminator);
      if (selected_expression) {
        expressions.insert(*selected_expression);
        return_expression =
            Reference<Language::Expression>(*selected_expression);
      }

      return True;
    }

    auto expression = Language::Parser::Expression::parse(
        domain, materializations, cursor, context);
    BAIL_IF(!expression);

    BAIL_IF(!cursor.require(
        Code::Type::EndStatement,
        "Library Function expressions require one terminating `;`."_view));

    // The terminator belongs to body grammar rather than the retained root.
    // Expression Span therefore ends at the last authored value Token.
    expressions.insert(*expression);
    Tetrodotoxin::Language::Parser::Comment::parse(cursor);
  }

  closing = cursor.consume();
  return True;
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
    : Base(definition),
      domain(domain),
      expressions(domain),
      expression_observations(domain) {}

auto Language::Function::complete(
    Cursor& cursor,
    Materializations& materializations) -> Bool {
  if (is_complete()) {
    cursor.create_token_error(
        "A Library Function can be completed only once."_view);
    return False;
  }

  auto transaction = cursor.branch();
  Managed::Vector<Reference<Expression>> parsed_expressions(domain);
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

  Token parsed_return_token;
  Span parsed_return_span;
  Option<Reference<Expression>> parsed_return_expression;
  Token closing;
  Bool body_complete = parse_body(
      domain, materializations, transaction, *this, parsed_expressions,
      parsed_return_token, parsed_return_span, parsed_return_expression,
      closing);
  BAIL_IF(!body_complete);
  BAIL_IF(!complete_definition(definition.get_qualifier(), closing));

  // Only complete grammar publishes Signature and Expression roots. Failed
  // Arena values remain unreachable from the reserved Function.
  signature = *parsed_signature;
  for (Count i = 0; i < parsed_expressions.get_size(); i++) {
    expressions.insert(parsed_expressions[i]);
    expression_observations.insert(parsed_expressions[i].get());
  }

  return_token = parsed_return_token;
  return_span = parsed_return_span;
  return_expression = parsed_return_expression;
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
  // double as a Type-resolution mode switch.
  return signature->link(source, get_host());
}

auto Language::Function::link_body(
    Tetrodotoxin::Language::Monograph& source,
    Materializations& materializations) -> Bool {
  if (linked) {
    return True;
  }
  BAIL_IF(!is_signature_linked());

  // Signature edges publish before body linking so every Identifier can reach
  // the exact Parameter object created for its authored declaration.
  Bool failed = False;
  View::Vector<Reference<Expression>> body = expressions;
  for (Count i = 0; i < body.get_size(); i++) {
    // Lexical context and access authority are independent facts. Function
    // owns parameter and bare-name lookup, while its exact host Type grants
    // private access only to explicit member receivers in this body.
    failed |= !body.get_data()[i].get().link(
        source, *this, materializations, get_host());
  }

  linked = !failed;
  return linked;
}

auto Language::Function::finalize(Tetrodotoxin::Language::Monograph& source)
    -> Bool {
  BAIL_IF(!linked);

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
  View::Vector<Reference<Expression>> body = expressions;
  for (Count i = 0; i < body.get_size(); i++) {
    body.get_data()[i].get().fold();
  }

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
        // the Definition-host chain.
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

auto Language::Function::get_expressions() const
    -> View::Vector<Reference<const Expression>> {
  return expression_observations;
}

auto Language::Function::get_return_expression() const
    -> Option<const Expression&> {
  return return_expression.visit(
      []() -> Option<const Expression&> { return {}; },
      [](const Reference<Expression>& expression) -> Option<const Expression&> {
        return expression.get();
      });
}

auto Language::Function::is_signature_linked() const -> Bool {
  return signature.visit(
      []() { return False; },
      [](const Signature& selected) { return selected.is_linked(); });
}
