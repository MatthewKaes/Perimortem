// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/package.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/syntax/ast/comment.hpp"
#include "tetrodotoxin/syntax/ast/import.hpp"
#include "tetrodotoxin/syntax/ast/member.hpp"
#include "tetrodotoxin/syntax/attribute/validation.hpp"
#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/dialect/dialect.hpp"
#include "tetrodotoxin/syntax/type/declaration.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

static auto parse_type(Context& ctx, Class& type) -> Bool {
  Class klass = ctx.get_current().get_class();

  if (Dialect::is_package_kind(klass)) {
    type = klass;
    ctx.advance();
    return true;
  }

  ctx.error(
      "Unknown package type"_view,
      "Valid package types are: Library, Shader, Entity"_view);
  return false;
}

auto Package::parse(
    const Lexical::Tokenizer& tokenizer,
    View::Bytes source_path) -> Package {
  Context context(tokenizer, source_path);
  return parse(context);
}

auto Package::detect_kind(View::Vector<Token> tokens) -> Class {
  for (Count i = 0; i < tokens.get_size(); i++) {
    if (tokens[i].get_class() != Class::Type::Package) {
      continue;
    }

    Count kind = i + 1;
    if (kind < tokens.get_size() &&
        tokens[kind].get_class() == Class::Type::Define) {
      kind++;
    }

    if (kind < tokens.get_size() &&
        Dialect::is_package_kind(tokens[kind].get_class())) {
      return tokens[kind].get_class();
    }

    return Class::Type::Library;
  }

  return Class::Type::Library;
}

auto Package::parse(Context& context) -> Package {
  Context& ctx = context;

  Package result;
  result.source_path = ctx.get_path();

  result.documentation = Ast::Comment::parse(ctx);

  if (!ctx.require(Class::Type::Package)) {
    return Package();
  }

  if (!ctx.require(Class::Type::Define)) {
    return Package();
  }

  if (!parse_type(ctx, result.kind)) {
    return Package();
  }
  result.dialect = &Dialect::resolve(result.kind);
  ctx.set_dialect(result.get_dialect());

  ctx.require(Class::Type::EndStatement);

  Managed::Vector<Ast::Import> import_list(ctx.get_arena());
  while (ctx.matches(Class::Type::Import)) {
    import_list.insert(Ast::Import::parse(ctx));
  }
  result.imports = import_list.get_view();

  result.body = Type::Body::parse(
      ctx, Class::Type::EndOfStream, Class::Type::EndOfStream);
  Attribute::validate_package(
      ctx, result.get_dialect(), result.imports, result.get_members(),
      result.get_functions(), result.get_types());
  return result;
}
