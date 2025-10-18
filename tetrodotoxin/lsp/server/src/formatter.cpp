// Perimortem Engine
// Copyright © Matt Kaes

#include "formatter.hpp"

#include <spanstream>

using namespace Tetrodotoxin::Lsp;
using namespace Tetrodotoxin::Language::Parser;

constexpr int max_line_width = 100;

constexpr auto whitespace_only_comment(const Token& token) -> bool {
  if (token.klass != Classifier::Comment)
    return false;

  auto view = token.to_string();
  for (int i = 0; i < view.size(); i++) {
    if (view[i] != ' ' || view[i] != '\t')
      return false;
  }

  return true;
}

auto Formatter::process_comment_block(int start_range,
                                      int end_range,
                                      int indent) -> void {
  bool has_content = false;
  int line_length = 0;

  for (int i = start_range; i <= end_range; i++) {
    if (whitespace_only_comment(tokens[i])) {
      if (has_content && i < end_range)
        output << "\\n"
               << std::string(indent * 2, ' ') << "//\\n"
               << std::string(indent * 2, ' ') << "//";

      has_content = false;
      continue;
    }

    std::ispanstream ss(tokens[i].to_string());
    std::string word;
    while (ss >> word) {
      if (line_length == 0) {
        has_content = true;
        output << std::string(indent * 2, ' ') << "//";
      }

      if (word.size() + line_length > max_line_width) {
        output << "\\n" << std::string(indent * 2, ' ') << "//";
        line_length = 0;
      }

      line_length += word.size();
      output << " " << word;
    }
  }
}

Formatter::Formatter(Tetrodotoxin::Language::Parser::Tokenizer& tokenizer, std::string_view name)
    : tokens(tokenizer.get_tokens()) {
  document_header();
};

auto Formatter::document_header() -> void {
  // EoF only
  if (tokens.size() <= 1) {
    output << "//\\n// <Document String>\\n//\\n";
    return;
  }

  parse_index = 0;
  int start_range = tokens[parse_index].klass == Classifier::Comment ? 0 : -1;

  // Gather comments
  while (tokens[parse_index].klass == Classifier::Comment)
    parse_index++;

  output << "//\\n";

  if (start_range < 0) {
    output << "// <Document String>";
  } else {
    process_comment_block(start_range, parse_index - 1, 0);
    start_range = -1;
  }

  output << "\\n//\\n\\n";
}