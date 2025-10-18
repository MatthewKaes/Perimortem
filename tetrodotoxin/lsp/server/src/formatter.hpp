// Perimortem Engine
// Copyright © Matt Kaes

#include "parser/tokenizer.hpp"

#include <sstream>
#include <string>

namespace Tetrodotoxin::Lsp {

class Formatter {
 public:
  Formatter(Tetrodotoxin::Language::Parser::Tokenizer& tokenizer, std::string_view name);
  auto get_content() const -> std::string { return output.str(); }

 private:
  auto document_header() -> void;
  auto process_comment_block(int start_range, int end_range, int indent) -> void;
  const Tetrodotoxin::Language::Parser::TokenStream& tokens;
  std::stringstream output;
  int parse_index = 0;
};

}  // namespace Tetrodotoxin::Lsp