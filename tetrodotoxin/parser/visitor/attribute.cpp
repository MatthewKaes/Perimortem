// Perimortem Engine
// Copyright © Matt Kaes

#include "parser/visitor/attribute.hpp"

#include "parser/visitor/string.hpp"

using namespace Tetrodotoxin::Lexical;
using namespace Tetrodotoxin::Parser;
using namespace Tetrodotoxin::Types;

auto Visitor::parse_attribute(Context& ctx) -> Compiler::Attribute* {
  Types::Compiler::Attribute* attribute =
      ctx.get_allocator().construct<Types::Compiler::Attribute>();
  auto token = &ctx.current();

  if (token->data.get_size() == 1) {
    ctx.token_error("TTX Script Attribute has an empty name.");
  }

  attribute->name =
      Perimortem::Memory::ByteView(token->data);

  auto start_token = token;

  // Check if we don't have an assignment for storing data.
  token = &ctx.advance();
  if (token->klass != Classifier::Assign) {
    return attribute;
  }

  // Check for string data since it was requested.
  token = &ctx.advance();
  if (token->klass != Classifier::String) {
    ctx.range_error(
        std::format(
            "TTX Script Attribute {} expected a String after `=` but got {}",
            attribute->name.get_view(), klass_name(token->klass)),
        *token, *start_token, *token);
    ctx.advance();
  } else {
    attribute->value = parse_string(ctx);
  }

  // If we correctly have an end statement then consume it, otherwise report a
  // non-fatal token class error.
  if (ctx.check_klass(Classifier::EndStatement)) {
    // Eat end statement.
    ctx.advance();
  }
  return attribute;
}
