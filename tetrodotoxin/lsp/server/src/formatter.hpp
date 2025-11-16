// Perimortem Engine
// Copyright © Matt Kaes

#include "lexical/tokenizer.hpp"

#include <sstream>
#include <string>

namespace Tetrodotoxin::Lsp {

class Formatter {
 public:
  Formatter() = default;

  // A simple formatter that looks at tokenized state. It doesn't requrie any
  // parse state and does best effort based on the token stream.
  auto tokenized_format(Tetrodotoxin::Lexical::Tokenizer& tokenizer,
                        std::string_view name) -> void;

  auto get_content() const -> std::string { return output.str(); }

 private:
  auto document_header(
      const Tetrodotoxin::Lexical::TokenStream& tokens,
      uint32_t& parse_index) -> void;
  auto package_name(const Tetrodotoxin::Lexical::TokenStream& tokens,
                    uint32_t parse_index,
                    std::string_view name) -> void;
  auto process_comment_block(
      const Tetrodotoxin::Lexical::TokenStream& tokens,
      uint32_t start_range,
      uint32_t end_range,
      uint32_t indent) -> void;
  std::stringstream output;
};

}  // namespace Tetrodotoxin::Lsp