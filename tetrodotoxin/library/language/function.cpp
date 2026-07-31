// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/function.hpp"

#include "tetrodotoxin/library/language/parser/layout.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Library;

static const Ttx::Model::Layouts::Fluid incomplete_layout;

static auto commit_cursor(Cursor& destination, const Cursor& source) -> void {
  while (destination.get_token_index() < source.get_token_index()) {
    destination.consume();
  }
}

static auto consume_body(Cursor& cursor) -> Bool {
  if (cursor.matches(Code::Type::EndStatement)) {
    cursor.create_token_error(
        "Ordinary Library Functions require an authored body."_view);
    return False;
  }

  if (!cursor.require(
          Code::Type::ScopeStart,
          "Library Function signatures require a body beginning with `{`."_view)) {
    return False;
  }

  // Scope Tokens already exclude braces carried by comments and byte values.
  // Counting only those structural Tokens proves the complete definition
  // boundary without interpreting or retaining any statement inside it.
  Count depth = 1;
  while (!cursor.matches(Code::Type::Terminal)) {
    Code code = cursor.consume().get_code();
    if (code == Code::Type::ScopeStart) {
      depth++;
      continue;
    }

    if (code == Code::Type::ScopeEnd) {
      depth--;
      if (depth == 0) {
        return True;
      }
    }
  }

  cursor.create_token_error(
      "Library Function body reached the end of source before `}`."_view);
  return False;
}

auto Language::Function::reserve(
    Allocator::Arena& domain,
    Cursor& cursor,
    const Documentation& documentation) -> Option<Function&> {
  Cursor transaction = cursor;
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

  if (!transaction.require(
          Code::Type::Func,
          "Library Function visibility must be followed by `func`."_view)) {
    return {};
  }

  Token name_token = transaction.require(
      Code::Type::Addressable,
      "Library Functions require an authored addressable name."_view);
  if (!name_token) {
    return {};
  }

  View::Bytes name = name_token.caculate_text(transaction.get_source_text());
  Function& function = domain.construct<Function>(
      Construction{}, domain, name, documentation, visibility);
  commit_cursor(cursor, transaction);
  return function;
}

Language::Function::Function(
    Construction,
    Allocator::Arena& domain,
    View::Bytes name,
    const Documentation& documentation,
    Visibility visibility)
    : domain(domain),
      name(name),
      documentation(documentation),
      visibility(visibility) {}

auto Language::Function::complete(Cursor& cursor, const Abstract& context)
    -> Bool {
  if (is_complete()) {
    cursor.create_token_error(
        "A Library Function signature can be completed only once."_view);
    return False;
  }

  // A copied Cursor keeps every parse allocation and diagnostic inside this
  // attempt while the caller remains at the signature opening. Only a complete
  // signature and body advance the caller to the next declaration.
  Cursor transaction = cursor;
  Option<const Layout&> parsed_parameters =
      Parser::Layout::parse(domain, transaction, context);
  if (!parsed_parameters) {
    return False;
  }

  if (!transaction.require(
          Code::Type::CallOp,
          "Library Function parameters require `->` before the result "
          "Layout."_view)) {
    return False;
  }

  Option<const Layout&> parsed_results =
      Parser::Layout::parse(domain, transaction, context);
  if (!parsed_results) {
    return False;
  }

  // Signature edges stay private until the complete body boundary is known.
  // A malformed body can leave Arena allocations behind, but the stable
  // Function still resolves to Invalid and exposes no partial signature.
  if (!consume_body(transaction)) {
    return False;
  }

  parameters = *parsed_parameters;
  results = *parsed_results;
  commit_cursor(cursor, transaction);
  return True;
}

auto Language::Function::resolve() const -> const Abstract& {
  if (!is_complete()) {
    return Invalid::get_invalid();
  }

  return *this;
}

auto Language::Function::resolve_context(View::Bytes) const -> const Abstract& {
  return Invalid::get_invalid();
}

auto Language::Function::get_parameters() const -> const Layout& {
  return parameters.visit(
      []() -> const Layout& { return incomplete_layout; },
      [](const Layout& selected) -> const Layout& { return selected; });
}

auto Language::Function::get_results() const -> const Layout& {
  return results.visit(
      []() -> const Layout& { return incomplete_layout; },
      [](const Layout& selected) -> const Layout& { return selected; });
}
