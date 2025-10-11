// Perimortem Engine
// Copyright © Matt Kaes

#include "parser/ast/comment.hpp"

#include <sstream>

using namespace Tetrodotoxin::Language::Parser;

auto Comment::parse(Context& ctx) -> std::optional<Comment> {
  std::optional<Comment> comment;

  auto token = &ctx.current();
  while (token->klass == Classifier::Comment) {
    // Don't concat empty comments
    if (token->data.empty()) {
      token = &ctx.advance();
      continue;
    }

    // Join comment lines with a space and treat it as one single line.
    if (!comment)
      comment = Comment();
    else
      comment->body.push_back(' ');

    comment->body.append({(char*)token->data.data(), token->data.size()});

    token = &ctx.advance();
  }

  return comment;
}
