// Perimortem Engine
// Copyright © Matt Kaes

#include "parser/tokenizer.hpp"

#include <string>
#include <string_view>

namespace Tetrodotoxin::Lsp {

class Service {
 public:
  static auto lsp_tokens(Tetrodotoxin::Language::Parser::Tokenizer& tokenizer,
                std::string_view jsonrpc,
                int32_t id) -> std::string;
  static auto format(Tetrodotoxin::Language::Parser::Tokenizer& tokenizer,
                std::string_view name,
                std::string_view jsonrpc,
                int32_t id) -> std::string;
};

}  // namespace Tetrodotoxin::Lsp