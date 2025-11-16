// Perimortem Engine
// Copyright © Matt Kaes

#include "lexical/tokenizer.hpp"

#include <string>
#include <string_view>

namespace Tetrodotoxin::Lsp {

class Service {
 public:
  static auto lsp_tokens(Tetrodotoxin::Lexical::Tokenizer& tokenizer,
                std::string_view jsonrpc,
                int32_t id) -> std::string;
  static auto format(Tetrodotoxin::Lexical::Tokenizer& tokenizer,
                std::string_view name,
                std::string_view jsonrpc,
                int32_t id) -> std::string;
};

}  // namespace Tetrodotoxin::Lsp