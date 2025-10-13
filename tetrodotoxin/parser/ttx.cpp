// Perimortem Engine
// Copyright © Matt Kaes

#include "parser/ttx.hpp"
#include "parser/ast/attribute.hpp"

#include <filesystem>

using namespace Tetrodotoxin::Language::Parser;

// Parsing Ttx from an existing tokenizer
auto Ttx::parse(const std::string_view& source_map, const ByteView& source)
    -> std::unique_ptr<Ttx> {
  Tokenizer tokenizer(source);

  std::unique_ptr<Ttx> ttx(new Ttx(source_map, source));
  Context ctx(source_map, source, tokenizer, ttx->errors);

  // Documentation | Attributes
  auto token = &ctx.current();
  ttx->documentation = Comment::parse(ctx);
  if (!ttx->documentation) {
    ctx.range_error(
        "TTX script is missing required documentation comment at start of "
        "file.",
        *token, ctx.current());
  }

  // Library Type
  ttx->parse_header(ctx);

  while (auto attribute = Attribute::parse(ctx)) {
    // TODO: Pull out any meta data that we want.
    ttx->config[std::string(attribute->name)] = attribute->value;
  }

  return ttx;
}

auto Ttx::parse_header(Context& ctx) -> void {
  auto token = &ctx.current();
  if (ctx.check_klass(Classifier::K_package, token->klass))
    return;

  token = &ctx.advance();
  if (ctx.check_klass(Classifier::Type, token->klass))
    return;

  name = {(char*)token->data.data(), token->data.size()};
  token = &ctx.advance();

  ctx.check_klass(Classifier::EndStatement, token->klass);
  token = &ctx.advance();
}