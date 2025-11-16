// Perimortem Engine
// Copyright © Matt Kaes

#include "parser/visitor/string.hpp"

using namespace Tetrodotoxin::Lexical;
using namespace Tetrodotoxin::Parser;
using namespace Tetrodotoxin::Types;

auto Visitor::parse_string(Context& ctx) -> std::string_view {
  if (!ctx.check_klass(Classifier::String)) {
    ctx.advance();
    return "";
  }

  auto token = &ctx.current();
  if (token->data.data()[token->data.size() - 1] != '\"') {
    ctx.token_error("String is missing it's closing quote");
    ctx.advance();
    return token->data.substr(1, token->data.size() - 1);
  }

  ctx.advance();
  return token->data.substr(1, token->data.size() - 2);
}
