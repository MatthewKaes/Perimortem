// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/function.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static const Layouts::Fluid incomplete_layout;

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
  if (!cursor.require(
          Code::Type::ScopeStart,
          "Library Function signatures require a body beginning with `{`."_view)) {
    return False;
  }

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
        if (!selected_expression) {
          return False;
        }
      }

      Token terminator = cursor.require(
          Code::Type::EndStatement,
          "Library Function returns require one terminating `;`."_view);
      if (!terminator) {
        return False;
      }

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
    if (!expression) {
      return False;
    }

    if (!cursor.require(
            Code::Type::EndStatement,
            "Library Function expressions require one terminating `;`."_view)) {
      return False;
    }

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
    const Documentation& documentation,
    Tetrodotoxin::Language::Monograph& source,
    const Type& host,
    Materializations& materializations) -> Option<Function&> {
  auto transaction = cursor.branch();
  Token opening = transaction.current();
  Visibility visibility;
  if (transaction.matches(Code::Type::Public)) {
    transaction.consume();
    visibility = Visibility::Public;
  } else if (transaction.matches(Code::Type::Private)) {
    transaction.consume();
    visibility = Visibility::Private;
  } else {
    transaction.create_token_error(
        "Library Functions require `public` or `private` visibility."_view);
    return {};
  }

  Token token = transaction.require(
      Code::Type::Func,
      "Library Function visibility must be followed by `func`."_view);
  if (!token) {
    return {};
  }

  Token name_token = transaction.require(
      Code::Type::Addressable,
      "Library Functions require an authored addressable name."_view);
  if (!name_token) {
    return {};
  }

  View::Bytes name = name_token.caculate_text(transaction.get_source_text());
  Function& function = domain.construct_from<Function>([&]() -> Function {
    return Function(
        domain, name, documentation, visibility, source, host, materializations,
        opening, token, name_token);
  });
  cursor.join(transaction);
  return function;
}

Language::Function::Function(
    Allocator::Arena& domain,
    View::Bytes name,
    const Documentation& documentation,
    Visibility visibility,
    Tetrodotoxin::Language::Monograph& source,
    const Type& host,
    Materializations& materializations,
    Token opening,
    Token token,
    Token name_token)
    : domain(domain),
      name(name),
      documentation(documentation),
      visibility(visibility),
      source(source),
      host(host),
      materializations(materializations),
      opening(opening),
      token(token),
      name_token(name_token),
      expressions(domain),
      expression_observations(domain) {}

auto Language::Function::complete(Cursor& cursor) -> Bool {
  if (is_complete()) {
    cursor.create_token_error(
        "A Library Function can be completed only once."_view);
    return False;
  }

  auto transaction = cursor.branch();
  Managed::Vector<Reference<Expression>> parsed_expressions(domain);
  auto parsed_signature = Signature::interpret(domain, transaction);
  if (!parsed_signature) {
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
  if (!body_complete) {
    return False;
  }

  // Only complete grammar publishes Signature and Expression roots. Failed
  // Arena values remain unreachable from the reserved Function.
  signature = *parsed_signature;
  for (Count i = 0; i < parsed_expressions.get_size(); i++) {
    expressions.insert(parsed_expressions[i]);
    expression_observations.insert(parsed_expressions[i].get());
  }

  span = Span(opening, closing);
  return_token = parsed_return_token;
  return_span = parsed_return_span;
  return_expression = parsed_return_expression;
  completed = True;
  cursor.join(transaction);
  return True;
}

auto Language::Function::link_signature() -> Bool {
  if (is_signature_linked()) {
    return True;
  }

  if (!completed || !signature) {
    source.report(
        Anchor::create(token, Span(opening, name_token)),
        "An incomplete Function cannot enter semantic linking."_view,
        "Complete its signature and body grammar before linking."_view);
    return False;
  }

  return signature->link(source, *this);
}

auto Language::Function::link_body() -> Bool {
  if (linked) {
    return True;
  }
  if (!is_signature_linked()) {
    return False;
  }

  // Signature edges publish before body linking so every Identifier can reach
  // the exact Parameter object created for its authored declaration.
  Bool failed = False;
  View::Vector<Reference<Expression>> body = expressions;
  for (Count i = 0; i < body.get_size(); i++) {
    failed |= !body.get_data()[i].get().link(source, *this, materializations);
  }

  linked = !failed;
  return linked;
}

auto Language::Function::link() -> Bool {
  Bool signature_linked = link_signature();
  if (!signature_linked) {
    return False;
  }

  return link_body();
}

auto Language::Function::finalize() -> Bool {
  if (!linked) {
    return False;
  }

  // Optional folding records a projection for later consumers. A dynamic
  // result or failure remains queryable but cannot turn an otherwise complete
  // Function into a semantic failure without a Constant requirement.
  View::Vector<Reference<Expression>> body = expressions;
  for (Count i = 0; i < body.get_size(); i++) {
    body.get_data()[i].get().fold();
  }

  return True;
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

  return host.visit<Language::Types::Structure>(
      [&](const Language::Types::Structure& structure) -> const Abstract& {
        return structure.resolve_context(route, *this);
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

auto Language::Function::get_expressions()
    -> View::Vector<Reference<Expression>> {
  return expressions;
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
