// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/environment/dialect.hpp"

#include "ttx/lexical/lexicon.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Ttx::Lexical;

static auto require_spelling(
    Cursor& cursor,
    Code::Type code,
    View::Bytes spelling,
    View::Bytes message) -> Bool {
  Token token = cursor.require(code, message);
  BAIL_IF(!token);
  if (token.caculate_text(cursor.get_source_text()) != spelling) {
    cursor.create_token_error(token, message);
    return False;
  }
  return True;
}

static auto parse_string(Cursor& cursor, View::Bytes message)
    -> Option<View::Bytes> {
  Token token = cursor.require(Code::Type::String, message);
  BAIL_IF(!token);
  View::Bytes source = token.caculate_text(cursor.get_source_text());
  if (!Lexicon::validate(Code::Type::String, source) || source.get_size() < 2) {
    cursor.create_token_error(
        token, "Environment String is unterminated."_view);
    return {};
  }
  return cursor.get_arena().proxy(source.slice(1, source.get_size() - 2));
}

static auto parse_field(Cursor& cursor, View::Bytes name) -> Bool {
  BAIL_IF(!cursor.require(
      Code::Type::AddressOp, "Environment fields begin with `.`."_view));
  BAIL_IF(!require_spelling(
      cursor, Code::Type::Addressable, name,
      "Environment field is missing or appears out of order."_view));
  return Bool(cursor.require(
      Code::Type::Assign,
      "Environment fields require `=` before their value."_view));
}

static auto finish_item(Cursor& cursor, Code::Type closing) -> Bool {
  if (!cursor.matches(Code::Type::PackingOp)) {
    return cursor.matches(closing);
  }
  cursor.consume();
  return True;
}

static auto parse_reachable_roots(
    Cursor& cursor,
    Managed::Vector<Environment::Plan::ReachableRoot>& roots) -> Bool {
  BAIL_IF(!cursor.require(
      Code::Type::BracketStart,
      "Environment reachable_roots require a bracketed list."_view));
  while (!cursor.matches(Code::Type::BracketEnd)) {
    if (cursor.matches(Code::Type::String)) {
      auto locator = parse_string(
          cursor, "A reachable project root requires one quoted path."_view);
      BAIL_IF(
          !locator || locator->is_empty() || (*locator)[0] == '/' ||
          (*locator)[0] == '\\');
      roots.insert({
        .kind = Environment::Plan::ReachableRoot::Kind::Project,
        .locator = *locator,
      });
    } else {
      BAIL_IF(!require_spelling(
          cursor, Code::Type::Addressable, "sdk"_view,
          "A reachable root is a confined project path or the installed sdk."_view));
      roots.insert({
        .kind = Environment::Plan::ReachableRoot::Kind::Sdk,
        .locator = "sdk"_view,
      });
    }
    BAIL_IF(!finish_item(cursor, Code::Type::BracketEnd));
  }
  return Bool(cursor.require(
      Code::Type::BracketEnd,
      "Environment reachable_roots require a closing `]`."_view));
}

static auto parse_sdk_repository(Cursor& cursor) -> Option<View::Bytes> {
  BAIL_IF(!require_spelling(
      cursor, Code::Type::Addressable, "sdk_repository"_view,
      "Environment repositories require sdk_repository locators."_view));
  BAIL_IF(!cursor.require(
      Code::Type::PackingStart,
      "sdk_repository requires `(` before its repository name."_view));
  auto repository = parse_string(
      cursor, "sdk_repository requires one quoted repository name."_view);
  BAIL_IF(!repository || repository->is_empty());
  BAIL_IF(!cursor.require(
      Code::Type::PackingEnd,
      "sdk_repository requires `)` after its repository name."_view));
  return *repository;
}

static auto parse_repositories(
    Cursor& cursor,
    Managed::Vector<View::Bytes>& repositories) -> Bool {
  BAIL_IF(!cursor.require(
      Code::Type::BracketStart,
      "Environment repositories require a bracketed list."_view));
  while (!cursor.matches(Code::Type::BracketEnd)) {
    auto repository = parse_sdk_repository(cursor);
    BAIL_IF(!repository);
    for (View::Bytes installed : repositories.get_view()) {
      if (installed == *repository) {
        cursor.create_token_error(
            "Environment repeats one SDK repository."_view);
        return False;
      }
    }
    repositories.insert(*repository);
    BAIL_IF(!finish_item(cursor, Code::Type::BracketEnd));
  }
  return Bool(cursor.require(
      Code::Type::BracketEnd,
      "Environment repositories require a closing `]`."_view));
}

static auto parse_sdk(Cursor& cursor, View::Bytes message)
    -> Option<View::Bytes> {
  BAIL_IF(
      !require_spelling(cursor, Code::Type::Addressable, "sdk"_view, message));
  BAIL_IF(!cursor.require(
      Code::Type::PackingStart,
      "sdk requires `(` before its artifact name."_view));
  auto artifact =
      parse_string(cursor, "sdk requires one quoted SDK artifact name."_view);
  BAIL_IF(!artifact || artifact->is_empty());
  BAIL_IF(!cursor.require(
      Code::Type::PackingEnd,
      "sdk requires `)` after its artifact name."_view));
  return *artifact;
}

static auto parse_plugins(Cursor& cursor, Managed::Vector<View::Bytes>& plugins)
    -> Bool {
  BAIL_IF(!cursor.require(
      Code::Type::BracketStart,
      "Environment plugins require a bracketed list."_view));
  while (!cursor.matches(Code::Type::BracketEnd)) {
    auto plugin = parse_sdk(
        cursor, "Environment plugins require explicit sdk locators."_view);
    BAIL_IF(!plugin);
    for (View::Bytes installed : plugins.get_view()) {
      if (installed == *plugin) {
        cursor.create_token_error(
            "Environment repeats one SDK plugin artifact."_view);
        return False;
      }
    }
    plugins.insert(*plugin);
    BAIL_IF(!finish_item(cursor, Code::Type::BracketEnd));
  }
  return Bool(cursor.require(
      Code::Type::BracketEnd,
      "Environment plugins require a closing `]`."_view));
}

auto Environment::Dialect::interpret(
    Ttx::Lexical::Cursor& cursor,
    const Ttx::Concept::Documentation&,
    const Ttx::Lexical::Anchor&,
    Ttx::Concept::Abstract&) -> Option<Language::Monograph&> {
  cursor.create_token_error(
      "Environment is an inline Build region rather than a root source."_view,
      "Select the Build Dialect and assign one Environment region there."_view);
  return {};
}

auto Environment::Dialect::parse_region(Cursor& cursor) const -> Option<Plan&> {
  Managed::Vector<Environment::Plan::ReachableRoot> reachable_roots(
      cursor.get_arena());
  Managed::Vector<View::Bytes> repositories(cursor.get_arena());
  Managed::Vector<View::Bytes> plugins(cursor.get_arena());
  BAIL_IF(!require_spelling(
      cursor, Code::Type::Addressable, "environment"_view,
      "Build requires one `environment = Environment { ... }` region."_view));
  BAIL_IF(!cursor.require(
      Code::Type::Assign, "Environment assignment requires `=`."_view));
  BAIL_IF(!require_spelling(
      cursor, Code::Type::Type, "Environment"_view,
      "Build must delegate its inline region to Environment."_view));
  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart, "Environment requires an opening `{`."_view));

  BAIL_IF(!parse_field(cursor, "output_root"_view));
  auto output_root = parse_string(
      cursor,
      "Environment output_root requires one quoted relative path."_view);
  BAIL_IF(
      !output_root || output_root->is_empty() || (*output_root)[0] == '/' ||
      (*output_root)[0] == '\\');
  BAIL_IF(!cursor.require(
      Code::Type::PackingOp,
      "Environment fields are separated by commas."_view));

  BAIL_IF(!parse_field(cursor, "reachable_roots"_view));
  BAIL_IF(!parse_reachable_roots(cursor, reachable_roots));
  if (reachable_roots.is_empty()) {
    cursor.create_token_error(
        "Environment requires at least one reachable root."_view,
        "Use `.` for project sources, `sdk` for installed artifacts, or both."_view);
    return {};
  }
  BAIL_IF(!cursor.require(
      Code::Type::PackingOp,
      "Environment fields are separated by commas."_view));

  BAIL_IF(!parse_field(cursor, "repositories"_view));
  BAIL_IF(!parse_repositories(cursor, repositories));
  BAIL_IF(!cursor.require(
      Code::Type::PackingOp,
      "Environment fields are separated by commas."_view));

  BAIL_IF(!parse_field(cursor, "plugins"_view));
  BAIL_IF(!parse_plugins(cursor, plugins));
  if (cursor.matches(Code::Type::PackingOp)) {
    cursor.consume();
  }
  BAIL_IF(!cursor.require(
      Code::Type::ScopeEnd, "Environment requires a closing `}`."_view));
  if (cursor.matches(Code::Type::PackingOp) ||
      cursor.matches(Code::Type::EndStatement)) {
    cursor.consume();
  }
  return cursor.get_arena().construct_from<Plan>([&]() -> Plan {
    return Plan(*output_root, reachable_roots, repositories, plugins);
  });
}
