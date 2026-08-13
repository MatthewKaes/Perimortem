// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/foreign/function.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/foreign/surface.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

using Tetrodotoxin::Language::Visibility;

auto Language::Foreign::Function::interpret(
    Allocator::Arena& domain,
    Monograph& source,
    Cursor& cursor,
    const Type& resolution_scope,
    const Surface& surface) -> Option<Function&> {
  auto transaction = cursor.branch();
  const Documentation& documentation =
      Tetrodotoxin::Language::Parser::Comment::parse(transaction);
  Token opening = transaction.current();
  Token visibility = transaction.current();
  switch (visibility.get_code().get_type()) {
  case Code::Type::Private:
    transaction.create_token_error(
        visibility,
        "Private Foreign Functions are unreachable from their parent Library."_view,
        "Declare the bodyless external Callable as `public`."_view);
    return {};
  case Code::Type::Expose:
    transaction.create_token_error(
        visibility, "Foreign Functions do not accept `expose` visibility."_view,
        "Declare the bodyless external Callable as `public`."_view);
    return {};
  case Code::Type::Public:
    transaction.consume();
    break;
  default:
    transaction.create_token_error(
        "Foreign Functions require `public` visibility."_view);
    return {};
  }

  BAIL_IF(!transaction.require(
      Code::Type::Func,
      "Foreign Function declarations require the `func` keyword."_view));
  Token name_token = transaction.require(
      Code::Type::Addressable,
      "Foreign Function requires one addressable symbol name."_view);
  BAIL_IF(!name_token);

  // Signature owns the real parameter and result Layouts. Foreign changes only
  // the closing grammar from a Library Block to one terminating token.
  auto signature = Signature::interpret(domain, source, transaction);
  BAIL_IF(!signature);
  if (signature->declares_self()) {
    transaction.create_expression_error(
        Anchor::create(Span(name_token, transaction.peek(-1))),
        "Foreign Function cannot declare a `self` receiver."_view,
        "External Callables are selected only through the source Foreign "
        "surface."_view);
    return {};
  }
  if (transaction.matches(Code::Type::ScopeStart)) {
    transaction.create_token_error(
        transaction.current(),
        "Foreign Function declarations cannot contain an authored body."_view,
        "Terminate the external signature with `;`."_view);
    return {};
  }

  Token terminator = transaction.require(
      Code::Type::EndStatement,
      "Foreign Function requires one terminating `;`."_view);
  BAIL_IF(!terminator);
  View::Bytes name =
      domain.proxy(name_token.caculate_text(transaction.get_source_text()));
  Function& function = domain.construct_from<Function>([&]() -> Function {
    return Function(
        documentation, name, name, *signature, resolution_scope, surface,
        Anchor::create(name_token, Span(opening, terminator)));
  });
  cursor.join(transaction);
  return function;
}

auto Language::Foreign::Function::link(Monograph& source) -> Bool {
  if (linked) {
    return True;
  }
  // The parent Source is the exact declaration scope for every retained Type
  // route. Reentry preserves the already linked Signature identity.
  BAIL_IF(!signature.link(source, resolution_scope));
  linked = True;
  return True;
}

auto Language::Foreign::Function::resolve() const -> const Abstract& {
  return linked ? static_cast<const Abstract&>(*this) : Invalid::get_invalid();
}

auto Language::Foreign::Function::resolve_context(View::Bytes) const
    -> const Abstract& {
  return Invalid::get_invalid();
}

auto Language::Foreign::Function::get_abi() const -> View::Bytes {
  return *surface.get_abi();
}
