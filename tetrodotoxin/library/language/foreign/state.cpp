// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/foreign.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

using Tetrodotoxin::Language::Visibility;

static auto parse_visibility(
    Cursor& cursor,
    Token& token,
    Visibility& visibility) -> Bool {
  token = cursor.current();
  switch (token.get_code().get_type()) {
  case Code::Type::Public:
    cursor.consume();
    visibility = Visibility::Public;
    return True;
  case Code::Type::Private:
    cursor.consume();
    visibility = Visibility::Private;
    return True;
  case Code::Type::Expose:
    cursor.consume();
    visibility = Visibility::Exposed;
    return True;
  default:
    cursor.create_token_error(
        "Foreign State requires `public` or `expose` visibility."_view);
    return False;
  }
}

auto Language::Foreign::State::interpret(
    Foreign& host,
    Cursor& cursor,
    const Documentation& documentation,
    View::Bytes abi) -> Option<State&> {
  Allocator::Arena& domain = cursor.get_arena();
  Token opening = cursor.current();
  Token visibility_token;
  Visibility visibility = Visibility::Private;
  BAIL_IF(!parse_visibility(cursor, visibility_token, visibility));

  // Foreign has no private member authority. Visibility therefore describes
  // the parent Library's exact read and write access instead of publication
  // through another enclosing semantic object.
  if (visibility == Visibility::Private) {
    cursor.create_token_error(
        visibility_token,
        "Private Foreign State is unreachable from its parent Library."_view,
        "Use `public state` for reads and writes or `expose state` for reads."_view);
    return {};
  }

  // External storage cannot stand in for a declaration owned compile time
  // value. Keeping this rejection here leaves a future loader contract with
  // one clear owner rather than weakening Constant semantics.
  Token policy = cursor.current();
  if (policy.get_code() == Code::Type::Const) {
    cursor.create_token_error(
        policy,
        "Foreign const declarations require a loader or embedding contract."_view,
        "Use State for external storage until a compile time value owner is "
        "available."_view);
    return {};
  }
  Token qualifier = cursor.require(
      Code::Type::State,
      "Foreign data declarations require the `state` policy."_view);
  BAIL_IF(!qualifier);

  Token name_token = cursor.require(
      Code::Type::Addressable,
      "Foreign State requires one addressable symbol name."_view);
  BAIL_IF(!name_token);
  BAIL_IF(!cursor.require(
      Code::Type::Define, "Foreign State requires `:` before its Type."_view));

  auto type_reference = TypeReference::parse(host, cursor);
  BAIL_IF(!type_reference);
  Token terminator = cursor.require(
      Code::Type::EndStatement,
      "Foreign State requires one terminating `;`."_view);
  BAIL_IF(!terminator);

  auto& definition = Tetrodotoxin::Language::Definition::create_authored(
      cursor, documentation, host, {}, {}, visibility, visibility_token,
      name_token.caculate_text(cursor.get_source_text()), name_token, qualifier,
      Anchor::create(name_token, Span(opening, terminator)));
  State& state = domain.construct_from<State>(
      [&]() -> State { return State(definition, *type_reference, abi); });
  return state;
}

auto Language::Foreign::State::link(Cursor& cursor) -> Bool {
  // Foreign forwards unchanged Type names to its Source. State can therefore
  // retain its real declaration host without acquiring Source internals.
  auto selected =
      type_reference.resolve_authored(cursor, definition.get_host());
  BAIL_IF(!selected);
  auto selected_type = selected->select<Language::Model::Type>();
  if (!selected_type) {
    cursor.create_expression_error(
        type_reference.get_anchor(),
        "Foreign State Type route did not resolve to one stable Type."_view,
        "Publish the exact Type in this Library source before linking."_view);
    return False;
  }
  if (selected_type->get_layout().is_empty()) {
    cursor.create_expression_error(
        type_reference.get_anchor(),
        "Foreign State cannot bind an empty Type Layout."_view,
        "Choose a Type that supplies at least one value."_view);
    return False;
  }

  if (type && &type->get() != &*selected_type) {
    cursor.create_expression_error(
        type_reference.get_anchor(),
        "Foreign State cannot change its linked Type identity."_view,
        "Repeat completion with the original resolved Type."_view);
    return False;
  }

  type = Reference<const Language::Model::Type>(*selected_type);
  return True;
}

auto Language::Foreign::State::resolve() const -> const Abstract& {
  return type ? static_cast<const Abstract&>(*this) : Invalid::get_invalid();
}
