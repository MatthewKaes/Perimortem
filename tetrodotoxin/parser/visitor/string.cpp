// Perimortem Engine
// Copyright © Matt Kaes

#include "parser/visitor/string.hpp"

using namespace Tetrodotoxin::Lexical;
using namespace Tetrodotoxin::Parser;
using namespace Tetrodotoxin::Types;
using namespace Perimortem::Memory;

auto Visitor::parse_string(Context& ctx) -> ByteView {
  if (!ctx.check_klass(Classifier::String)) {
    ctx.advance();
    return ByteView("");
  }

  auto token = &ctx.current();
  if (token->data[token->data.get_size() - 1] != '\"') {
    ctx.token_error("String is missing it's closing quote");
    ctx.advance();
    return token->data.slice(1, token->data.get_size() - 1);
  }

  ctx.advance();
  return token->data.slice(1, token->data.get_size() - 2);
}
