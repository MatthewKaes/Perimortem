// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/foreign/state.hpp"

#include "tetrodotoxin/language/parser/comment.hpp"
#include "tetrodotoxin/library/language/foreign/surface.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

using Tetrodotoxin::Language::Visibility;

static auto parse_visibility(Cursor& cursor, Token& token)
    -> Option<Visibility> {
  token = cursor.current();
  switch (token.get_code().get_type()) {
  case Code::Type::Public:
    cursor.consume();
    return Visibility::Public;
  case Code::Type::Private:
    cursor.consume();
    return Visibility::Private;
  case Code::Type::Expose:
    cursor.consume();
    return Visibility::Exposed;
  default:
    cursor.create_token_error(
        "Foreign State requires `public` or `expose` visibility."_view);
    return {};
  }
}

auto Language::Foreign::State::interpret(
    Allocator::Arena& domain,
    Monograph& source,
    Cursor& cursor,
    const Type& resolution_scope,
    const Surface& surface) -> Option<State&> {
  auto transaction = cursor.branch();
  const Documentation& documentation =
      Tetrodotoxin::Language::Parser::Comment::parse(transaction);
  Token opening = transaction.current();
  Token visibility_token;
  auto visibility = parse_visibility(transaction, visibility_token);
  BAIL_IF(!visibility);

  // Foreign has no private member authority. Visibility therefore describes
  // the parent Library's exact read and write access instead of publication
  // through another enclosing semantic object.
  if (*visibility == Visibility::Private) {
    transaction.create_token_error(
        visibility_token,
        "Private Foreign State is unreachable from its parent Library."_view,
        "Use `public state` for reads and writes or `expose state` for reads."_view);
    return {};
  }

  // External storage cannot stand in for a declaration owned compile time
  // value. Keeping this rejection here leaves a future loader contract with
  // one clear owner rather than weakening Constant semantics.
  Token policy = transaction.current();
  if (policy.get_code() == Code::Type::Const) {
    transaction.create_token_error(
        policy,
        "Foreign const declarations require a loader or embedding contract."_view,
        "Use State for external storage until a compile time value owner is "
        "available."_view);
    return {};
  }
  BAIL_IF(!transaction.require(
      Code::Type::State,
      "Foreign data declarations require the `state` policy."_view));

  Token name_token = transaction.require(
      Code::Type::Addressable,
      "Foreign State requires one addressable symbol name."_view);
  BAIL_IF(!name_token);
  BAIL_IF(!transaction.require(
      Code::Type::Define, "Foreign State requires `:` before its Type."_view));

  auto type_reference = TypeReference::parse(source, transaction);
  BAIL_IF(!type_reference);
  Token terminator = transaction.require(
      Code::Type::EndStatement,
      "Foreign State requires one terminating `;`."_view);
  BAIL_IF(!terminator);

  View::Bytes name =
      domain.proxy(name_token.caculate_text(transaction.get_source_text()));
  State& state = domain.construct_from<State>([&]() -> State {
    return State(
        documentation, *visibility, name, *type_reference, resolution_scope,
        surface, Anchor::create(name_token, Span(opening, terminator)));
  });
  cursor.join(transaction);
  return state;
}

auto Language::Foreign::State::link(Monograph& source) -> Bool {
  auto context = resolution_scope.select<Language::Types::Composite>();
  if (!context) {
    source.report(
        anchor, "Foreign State requires one Library Composite scope."_view,
        "Retain the declaration on its exact Library Source."_view);
    return False;
  }

  // The retained TypeReference resolves under the parent Source authority.
  // State keeps that one exact edge and rejects empty value flow because an
  // Addressable must name at least one value.
  const Abstract& selected = context->resolve_type(type_reference);
  auto selected_type = selected.select<Type>();
  if (!selected_type) {
    source.report(
        type_reference.get_anchor(),
        "Foreign State Type route did not resolve to one stable Type."_view,
        "Publish the exact Type in this Library source before linking."_view);
    return False;
  }
  if (selected_type->get_layout().is_empty()) {
    source.report(
        type_reference.get_anchor(),
        "Foreign State cannot bind an empty Type Layout."_view,
        "Choose a Type that supplies at least one value."_view);
    return False;
  }

  if (type && &type->get() != &*selected_type) {
    source.report(
        type_reference.get_anchor(),
        "Foreign State cannot change its linked Type identity."_view,
        "Repeat completion with the original resolved Type."_view);
    return False;
  }

  type = Reference<const Type>(*selected_type);
  return True;
}

auto Language::Foreign::State::resolve() const -> const Abstract& {
  return type ? static_cast<const Abstract&>(*this) : Invalid::get_invalid();
}

auto Language::Foreign::State::get_abi() const -> View::Bytes {
  return *surface.get_abi();
}
