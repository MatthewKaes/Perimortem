// Perimortem Engine
// Copyright © Matt Kaes

#include "lexical/tokenizer.hpp"
#include "storage/formats/rpc_header.hpp"
using RpcHeader = Perimortem::Storage::Json::RpcHeader;

#include <string>
#include <string_view>

namespace Tetrodotoxin::Lsp {

class Service {
 public:
  static auto lsp_tokens(Tetrodotoxin::Lexical::Tokenizer& tokenizer,
                         const Perimortem::Storage::Json::RpcHeader& header)
      -> std::string;
  static auto format(Tetrodotoxin::Lexical::Tokenizer& tokenizer,
                     std::string_view name,
                     const Perimortem::Storage::Json::RpcHeader& header)
      -> std::string;
};

}  // namespace Tetrodotoxin::Lsp