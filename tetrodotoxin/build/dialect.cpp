// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/build/dialect.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/build/monograph.hpp"
#include "tetrodotoxin/environment/provider.h"
#include "tetrodotoxin/language/error.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"
#include "ttx/lexical/lexicon.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

class BuildProductionError final : public Language::Error {
 public:
  TTX_NAME("Build production error"_view);

  void describe(Ttx::Lexical::Errors::Report& report) const override {
    report
        << "Build production requires the Monograph created by the Build Dialect."_view;
  }
};

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
    cursor.create_token_error(token, "Build String is unterminated."_view);
    return {};
  }
  return cursor.get_arena().proxy(source.slice(1, source.get_size() - 2));
}

static auto parse_field(Cursor& cursor, View::Bytes name) -> Bool {
  BAIL_IF(!cursor.require(
      Code::Type::AddressOp,
      "Environment and product fields begin with `.`."_view));
  BAIL_IF(!require_spelling(
      cursor, Code::Type::Addressable, name,
      "Build field is missing or appears out of order."_view));
  return Bool(cursor.require(
      Code::Type::Assign, "Build fields require `=` before their value."_view));
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

static auto parse_product(Cursor& cursor) -> Option<Build::ProductDescription> {
  BAIL_IF(!cursor.require(
      Code::Type::Public, "Build products must be public declarations."_view));
  Token name = cursor.require(
      Code::Type::Type, "Build product requires one exported name."_view);
  BAIL_IF(!name);
  BAIL_IF(!cursor.require(
      Code::Type::Define, "Build product name requires `:`."_view));
  BAIL_IF(!require_spelling(
      cursor, Code::Type::Addressable, "product"_view,
      "Build product declarations require the `product` form."_view));
  BAIL_IF(!cursor.require(
      Code::Type::Assign, "Build product declarations require `=`."_view));
  BAIL_IF(!require_spelling(
      cursor, Code::Type::Type, "Product"_view,
      "Build products begin from the private Product Package locator."_view));
  BAIL_IF(!cursor.require(
      Code::Type::TypeAccessOp,
      "Build product requires a Package export after `Product::`."_view));
  Token package_route = cursor.require(
      Code::Type::Type, "Build product requires one Package export name."_view);
  BAIL_IF(!package_route);
  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart, "Build product requires an opening `{`."_view));
  BAIL_IF(!parse_field(cursor, "terminal"_view));
  auto terminal_artifact = parse_sdk(
      cursor,
      "Build terminal requires one explicit sdk artifact locator."_view);
  BAIL_IF(!terminal_artifact);
  BAIL_IF(!cursor.require(
      Code::Type::TypeAccessOp,
      "Build terminal requires one exported provider after `::`."_view));
  Token terminal_export = cursor.require(
      Code::Type::Type,
      "Build terminal requires one provider export name."_view);
  BAIL_IF(!terminal_export);
  if (cursor.matches(Code::Type::PackingOp)) {
    cursor.consume();
  }
  BAIL_IF(!cursor.require(
      Code::Type::ScopeEnd, "Build product requires a closing `}`."_view));
  BAIL_IF(!cursor.require(
      Code::Type::EndStatement,
      "Build product declarations require a terminating `;`."_view));

  return Build::ProductDescription{
    .name =
        cursor.get_arena().proxy(name.caculate_text(cursor.get_source_text())),
    .package_route = cursor.get_arena().proxy(
        package_route.caculate_text(cursor.get_source_text())),
    .terminal_artifact = *terminal_artifact,
    .terminal_export = cursor.get_arena().proxy(
        terminal_export.caculate_text(cursor.get_source_text())),
  };
}

auto Build::Dialect::interpret(
    Cursor& cursor,
    const Documentation& documentation,
    const Anchor&,
    Abstract& context) -> Option<Language::Monograph&> {
  Managed::Vector<ProductDescription> products(cursor.get_arena());

  Language::Parser::Comment::parse(cursor);
  auto description = environment.parse_region(cursor);
  BAIL_IF(!description);
  while (!cursor.matches(Code::Type::Terminal)) {
    Language::Parser::Comment::parse(cursor);
    if (cursor.matches(Code::Type::Terminal)) {
      break;
    }
    auto product = parse_product(cursor);
    BAIL_IF(!product);
    products.insert(*product);
  }

  return cursor.get_arena().construct<Monograph>(
      cursor.get_arena(), *this, documentation, context,
      cursor.get_arena().proxy(cursor.get_source_path()), *description,
      products);
}

void Build::Dialect::produce(
    ttx_context context,
    Allocator::Arena& arena,
    tetrodotoxin_workspace_view workspace,
    const Language::Monograph& monograph,
    tetrodotoxin_production_result result) const {
  auto build = monograph.select<Build::Monograph>();
  if (!build) {
    auto& error = arena.construct<BuildProductionError>();
    result.operations->failed(result.self, error.get_abi());
    return;
  }
  if (workspace.operations == nullptr || workspace.self == nullptr ||
      workspace.operations->root == nullptr) {
    auto& error = arena.construct<BuildProductionError>();
    result.operations->failed(result.self, error.get_abi());
    return;
  }
  const ttx_abstract graph_handle = workspace.operations->root(workspace.self);
  auto graph = Abstract::from_handle(graph_handle);
  if (!graph) {
    auto& error = arena.construct<BuildProductionError>();
    result.operations->failed(result.self, error.get_abi());
    return;
  }
  environment.produce_build(
      build->get_environment(), build->get_source_path(),
      build->get_package_locator(), build->get_products(), *graph, context,
      arena, result);
}
