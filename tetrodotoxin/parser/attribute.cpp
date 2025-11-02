// Perimortem Engine
// Copyright © Matt Kaes

#include "parser/attribute.hpp"

using namespace Tetrodotoxin::Language::Parser;

auto Attribute::parse(Context& ctx) -> std::optional<Attribute> {
  auto token = &ctx.current();
  if (token->klass != Classifier::Attribute)
    return std::nullopt;

  auto start_token = token;
  Attribute attribute = Attribute(token->data);

  // Check if we don't have an assignment
  token = &ctx.advance();
  if (token->klass != Classifier::Assign) {
    return attribute;
  }

  // = token
  ctx.advance();
  token = &ctx.current();

  // Didn't get any string data even though it was requested.
  if (token->klass != Classifier::String) {
    ctx.range_error(std::format("TTX Script Attribute @{} expected a String after `=` but got {}", attribute.name, klass_name(token->klass)), *token, *start_token, *token);
    ctx.advance();
    return std::nullopt;
  }

  attribute.value = token->data;
  ctx.advance();
  return attribute;
}
