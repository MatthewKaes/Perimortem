// Perimortem Engine
// Copyright © Matt Kaes

#include "parser/ast/attribute.hpp"

#include <sstream>

using namespace Tetrodotoxin::Language::Parser;

auto Attribute::parse(Context &ctx) -> std::optional<Attribute> {
  auto token = &ctx.current();
  if (token->klass != Classifier::Attribute)
    return std::nullopt;

  auto start_token = token;
  Attribute attribute = Attribute(token->to_string());

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
    std::stringstream details;
    details << "Attribute " << attribute.name << " expected a String but got "
            << klass_name(token->klass) << " '" << token->to_string() << "'";
    ctx.range_error(details.view(), *token, *start_token, *token);
    ctx.advance();
    return std::nullopt;
  }

  attribute.value = token->to_string();
  ctx.advance();
  return attribute;
}
