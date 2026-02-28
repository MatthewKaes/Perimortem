// Perimortem Engine
// Copyright © Matt Kaes

#include "lexical/tokenizer.hpp"
#include "rpc_request.hpp"

namespace Tetrodotoxin::Lsp {

class Service {
 public:
  static auto lsp_tokens(Tetrodotoxin::Lexical::Tokenizer& tokenizer,
                         const RpcRequest& header)
      -> Perimortem::Storage::Json::Node;
};

}  // namespace Tetrodotoxin::Lsp