// Perimortem Engine
// Copyright © Matt Kaes

#include "formatter.hpp"

#include <spanstream>

using namespace Tetrodotoxin::Lsp;
using namespace Tetrodotoxin::Lexical;

constexpr int max_line_width = 80;
constexpr int indent_width = 2;

constexpr auto whitespace_only_comment(const Token& token) -> bool {
  if (token.klass != Classifier::Comment)
    return false;

  auto view = token.data;
  for (int i = 0; i < view.get_size(); i++) {
    if (view[i] != ' ' || view[i] != '\t')
      return false;
  }

  return true;
}

auto Formatter::tokenized_format(
    Tetrodotoxin::Lexical::Tokenizer& tokenizer,
    std::string_view name) -> void {
  output.clear();
  uint32_t parse_index = 0;
  const auto& tokens = tokenizer.get_tokens();
  document_header(tokens, parse_index);
  package_name(tokens, parse_index, name);

  // If the block is empty then add some info.
  if (tokens[parse_index].klass == Classifier::EndOfStream) {
    output << "// Import any dependency packages into a scoped type.\\n/> "
              "requires Type via \"./Path.ttx\";\\n\\n";
    output << "// Declare any package scoped variables or types.\\n";
    output << "/> [=/=] constant_value : Int = 1;\\n";
    output << "/> [=/=] public_static  : Int = 2;\\n";
    output << "/> [=!=] hidden_static  : Int = 3;\\n";
    output << "/> [=/=] thread_local   : Int = 4;\\n\\n";
    output << "// Loader function that runs once before any packages that "
              "import this one.\\n";
    output << "/> [***] on_load : func() -> Byt = {\\n";
    output << "  // The return value of on_load is saved on the type.\\n";
    output << "  return 0;\\n";
    output << "}\\n";
    return;
  }

  bool has_content = false;
  bool eat_space = false;
  bool in_group = false;
  uint32_t indent = 0;
  uint32_t start_range = -1;
  ClassifierFlags control_flow =
      Classifier::If | Classifier::Else | Classifier::For;

  while (tokens[parse_index].klass != Classifier::EndOfStream) {
    const auto& token = tokens[parse_index++];

    switch (token.klass) {
      case Classifier::GroupStart:
        in_group = true;
        // Special case for control flows.
        if (control_flow.has(tokens[parse_index - 2].klass))
          output << " ";
        eat_space = true;
        output << token.data.get_view();
        continue;
      case Classifier::GroupEnd:
        in_group = false;
        output << token.data.get_view();
        break;
      case Classifier::AccessOp:
      case Classifier::IndexStart:
        eat_space = true;
        output << token.data.get_view();
        continue;
      case Classifier::Seperator:
      case Classifier::IndexEnd:
        output << token.data.get_view();
        break;
      case Classifier::EndOfStream:
        output << "\\n\\n";
        break;
      case Classifier::EndStatement:
        output << ";" << (in_group ? "" : "\\n");
        has_content = false;
        break;
      case Classifier::ScopeStart:
        indent++;
        has_content = false;

        // Space content from scope blocks.
        if (tokens[parse_index].klass == Classifier::ScopeEnd) {
          output << " { }\\n";
          parse_index++;
        } else {
          output << " {\\n";
        }
        break;
      case Classifier::ScopeEnd:
        // Prevent indent underflow.
        indent = std::max(0u, indent - 1);

        if (has_content)
          output << "\\n";

        output << std::string(indent * indent_width, ' ') << token.data.get_view();

        // Fold else blocks onto the same line.
        if (tokens[parse_index].klass == Classifier::Else) {
          has_content = true;
          break;
        }

        output << "\\n";
        has_content = false;

        // Space content from scope blocks.
        if (tokens[parse_index].klass != Classifier::Comment &&
            tokens[parse_index].klass != Classifier::ScopeEnd &&
            tokens[parse_index].klass != Classifier::EndOfStream) {
          output << "\\n";
        }
        break;
      case Classifier::Comment:
        // Add a second new line if the comment is in the middle of a scope.
        if (tokens[parse_index - 2].klass != Classifier::ScopeStart)
          output << "\\n";

        start_range = parse_index - 1;

        // Gather comments
        while (tokens[parse_index].klass == Classifier::Comment)
          parse_index++;

        process_comment_block(tokens, start_range, parse_index - 1, indent);
        output << "\\n";
        has_content = false;
        break;
      case Classifier::Attribute:
        if (has_content) {
          output << (eat_space ? "@" : " @") << token.data.get_view();
        } else {
          output << std::string(indent * indent_width, ' ') << "@"
                 << token.data.get_view();
        }

        has_content = true;
        break;
      default:
        if (has_content) {
          output << (eat_space ? "" : " ") << token.data.get_view();
        } else {
          output << std::string(indent * indent_width, ' ')
                 << token.data.get_view();
        }

        has_content = true;
        break;
    }

    eat_space = false;
  }
}

auto Formatter::document_header(
    const Tetrodotoxin::Lexical::TokenStream& tokens,
    uint32_t& parse_index) -> void {
  // EoF only
  if (tokens.size() <= 1) {
    output << "//\\n// <Document String>\\n//\\n";
    return;
  }

  parse_index = 0;
  uint32_t start_range =
      tokens[parse_index].klass == Classifier::Comment ? 0 : -1;

  // Gather comments
  while (tokens[parse_index].klass == Classifier::Comment)
    parse_index++;

  output << "//\\n";

  if (start_range < 0) {
    output << "// <Document String>";
  } else {
    process_comment_block(tokens, start_range, parse_index - 1, 0);
    start_range = -1;
  }

  output << "\\n//\\n";
}

auto Formatter::package_name(
    const Tetrodotoxin::Lexical::TokenStream& tokens,
    uint32_t parse_index,
    std::string_view name) -> void {
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

    output << "package Lib;\\n\\n";
  }
}

auto Formatter::process_comment_block(
    const Tetrodotoxin::Lexical::TokenStream& tokens,
    uint32_t start_range,
    uint32_t end_range,
    uint32_t indent) -> void {
  bool has_content = false;
  int line_length = 0;

  for (int i = start_range; i <= end_range; i++) {
    if (whitespace_only_comment(tokens[i])) {
      if (has_content && i < end_range)
        output << "\\n"
               << std::string(indent * indent_width, ' ') << "//\\n"
               << std::string(indent * indent_width, ' ') << "//";

      has_content = false;
      continue;
    }

    std::ispanstream ss(tokens[i].data.get_view());
    std::string word;
    while (ss >> word) {
      if (line_length == 0) {
        has_content = true;
        output << std::string(indent * indent_width, ' ') << "//";
        line_length += indent * indent_width + 2;
      }

      if (word.size() + line_length > max_line_width) {
        output << "\\n" << std::string(indent * indent_width, ' ') << "//";
        line_length = indent * indent_width;
      }

      line_length += word.size() + 1;
      output << " " << word;
    }
  }
}