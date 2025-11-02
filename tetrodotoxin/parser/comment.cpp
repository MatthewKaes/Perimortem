// Perimortem Engine
// Copyright © Matt Kaes

#include "parser/comment.hpp"

#include <spanstream>
#include <sstream>

using namespace Tetrodotoxin::Language::Parser;

constexpr auto whitespace_only_comment(const Token& token) -> bool {
  if (token.klass != Classifier::Comment)
    return false;

  auto view = token.data;
  for (int i = 0; i < view.size(); i++) {
    if (view[i] != ' ' || view[i] != '\t')
      return false;
  }

  return true;
}

auto Comment::parse(Context& ctx) -> std::optional<std::string> {
  auto token = &ctx.current();
  if (token->klass != Classifier::Comment)
    return std::nullopt;

  std::stringstream output;
  while (token->klass == Classifier::Comment) {
    output << token->data << "\n";
    token = &ctx.advance();
  }

  return output.str();
}
