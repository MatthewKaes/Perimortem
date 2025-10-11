// Perimortem Engine
// Copyright © Matt Kaes

#include "parser/ttx.hpp"

#include <filesystem>
#include <sstream>

using namespace Tetrodotoxin::Language::Parser;

// Parsing Ttx from an existing tokenizer
Ttx::Ttx(const std::string_view& source_map, const ByteView& source_)
    : source_map(source_map), source(source_.begin(), source_.end()) {
  Tokenizer tokenizer(source);

  Context ctx(source_map, source, tokenizer, errors);

  // Schema
  // Comment
  // attribute<Ttx (values: Library)>
  // (Class | Function | __init)

  // Documentation | Attributes
  documentation = Comment::parse(ctx);
  if (!documentation) {
    TTX_ERROR(
        "TTX script does not start with the required documentation comment.");
  }

  // Library Type
  parse_header(ctx);
}

auto Ttx::parse_header(Context& ctx) -> void {
  auto token = &ctx.current();
  switch (token->klass) {
    case Classifier::K_library:
      type = Type::Library;
      break;
    case Classifier::K_game_object:
      type = Type::Object;
      break;
    default: {
      TTX_TOKEN_FATAL("Expected TTX header type (library|game_object) but got "
                      << klass_name(token->klass));
      break;
    }
  }

  token = &ctx.advance();
  if (token->klass != Classifier::Type) {
    TTX_TOKEN_FATAL("Expected TTX header type (library|game_object) but got "
                    << klass_name(token->klass));
  }

  name = {(char*)token->data.data(), token->data.size()};
  token = &ctx.advance();
  if (token->klass != Classifier::EndStatement) {
    TTX_TOKEN_FATAL("Expected " << klass_name(Classifier::EndStatement)
                                << " but got " << klass_name(token->klass));
  }

  token = &ctx.advance();
}