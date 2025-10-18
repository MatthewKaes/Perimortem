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

Formatter::Formatter(Tetrodotoxin::Language::Parser::Tokenizer& tokenizer,
                     std::string_view name)
    : tokens(tokenizer.get_tokens()) {
  document_header();
  package_name(name);

  // If the block is empty then add some info.
  if (tokens[parse_index].klass == Classifier::EndOfStream) {
    output << "/> <Add package variables here>\\n\\n// Package loader\\n";
    output << "[***] on_load : func() -> Byt = {\\n\\n";
    output << "  /> <Code to run during package load>\\n";
    output << "  return 0;\\n\\n";
    output << "}\\n";
  }

  const ClassifierFlags new_line_klasses = Classifier::EndOfStream | Classifier::ScopeEnd | Classifier::ScopeEnd;
  bool has_content = false;
  while (tokens[parse_index].klass != Classifier::EndOfStream) {
    output << (has_content ? " " : "") << tokens[parse_index].to_string();
    if (new_line_klasses.has(tokens[parse_index].klass)) {
      output << "\\n";
      has_content = false;
    } else {
      has_content = true;
    }

    parse_index++;
  }
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

  output << "\\n//\\n";
}

auto Formatter::package_name(std::string_view name) -> void {
  if (tokens[parse_index].klass != Classifier::Package) {
    std::string sanatized_name;
    for (int i = 0; i < name.size(); i++) {
      switch (name[i]) {
        case 'A' ... 'Z':
          sanatized_name += name[i];
          break;
        case 'a' ... 'z':
          if (sanatized_name.empty() || (i > 0 && name[i - 1] == '_'))
            sanatized_name += name[i] - ('a' - 'A');
          else
            sanatized_name += name[i];
          break;
        case '0' ... '1':
          if (!sanatized_name.empty())
            sanatized_name += name[i];
          break;
        default:
          break;
      }
    }

    if (sanatized_name.empty()) {
      sanatized_name = "PackageType";
    }

    output << "package " << sanatized_name << ";\\n\\n";
  }
}