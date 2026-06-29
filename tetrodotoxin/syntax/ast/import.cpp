// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/ast/import.hpp"

#include "tetrodotoxin/syntax/context.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

// Parses: ( .source = "path/to/file.ttx" )
// Returns the string token text on success, or an empty view on error.
static auto parse_file_source(Context& ctx) -> View::Bytes {
  if (!ctx.require(Class::Type::PackingStart)) {
    return View::Bytes();
  }

  if (!ctx.require(Class::Type::AddressOp)) {
    return View::Bytes();
  }

  if (!ctx.matches(Class::Type::Addressable)) {
    ctx.error("Expected 'source' after '.'"_view);
    return View::Bytes();
  }

  if (ctx.get_current().get_text() != "source"_view) {
    ctx.error(
        "Unknown import pack field"_view,
        "Only '.source = \"path\"' is valid inside an import pack"_view);
  }

  ctx.advance();

  if (!ctx.require(Class::Type::Assign)) {
    return View::Bytes();
  }

  if (!ctx.matches(Class::Type::String)) {
    ctx.error("Expected a string literal for the source path"_view);
    return View::Bytes();
  }

  View::Bytes path = ctx.get_current().get_text();
  ctx.advance();
  ctx.require(Class::Type::PackingEnd);
  return path;
}

auto Ast::Import::parse(Context& ctx) -> Import {
  ctx.require(Class::Type::Import);

  if (!ctx.matches(Class::Type::Type)) {
    ctx.error(
        "Expected an import name after 'import'"_view,
        "Import names are PascalCase, e.g. 'import Graphics : Library = ...'"_view);
    return Import();
  }

  Import result;
  result.local_name = ctx.get_current().get_text();
  ctx.advance();

  if (!ctx.require(Class::Type::Define)) {
    return Import();
  }

  result.klass = ctx.get_current().get_class();

  switch (result.klass.get_type()) {
  case Class::Type::Library:
  case Class::Type::Shader:
  case Class::Type::Entity:
    break;

  default:
    ctx.error(
        "Expected a package kind after ':'"_view,
        "Valid kinds are: Library, Shader, Entity"_view);
    return Import();
  }

  ctx.advance();

  if (!ctx.require(Class::Type::Assign)) {
    return Import();
  }

  if (ctx.matches(Class::Type::PackingStart)) {
    result.source_klass = Class::Type::PackedData;
    result.file_path = parse_file_source(ctx);
  } else if (ctx.matches(Class::Type::Type)) {
    result.source_klass = Class::Type::Type;
    result.package_alias = Type::Ref::parse(ctx);
  } else {
    ctx.error(
        "Expected an import source after '='"_view,
        "Use '( .source = \"path.ttx\" )' for a file or 'Package.Name' for a "
        "registry package"_view);
    return Import();
  }

  ctx.require(Class::Type::EndStatement);
  return result;
}
