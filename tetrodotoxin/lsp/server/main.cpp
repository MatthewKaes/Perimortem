// Perimortem Engine
// Copyright © Matt Kaes

#include <iostream>

#include "perimortem/storage/formats/base64.hpp"
#include "perimortem/storage/formats/json.hpp"
#include "lexical/tokenizer.hpp"
#include "src/language_server.hpp"
#include "src/service.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Storage;
using namespace Tetrodotoxin::Lsp;
using namespace Tetrodotoxin::Lexical;

constexpr const char* Lsp_SUPPORT = "3.17";

auto main(int argc, char* argv[]) -> int {
  const char* pipe_flag = "--pipe=";
  std::cout << "~~ TTX Lang Server ~~" << std::endl;
  std::cout << "[Lsp_VERSION: " << Lsp_SUPPORT << "]" << std::endl << std::endl;

  const std::vector<std::string> args(argv, argv + argc);
  std::string pipe_name;

  for (const auto& arg : args) {
    if (arg.starts_with(pipe_flag)) {
      pipe_name = arg.substr(sizeof(pipe_flag) - 1);
    }
  }

  if (pipe_name.empty()) {
    std::cout << "No `--pipe=` provided, closing language server." << std::endl;
    return -1;
  }

  std::cout << "Creating JsonRPC Server using pipe " << pipe_name << std::endl;
  UnixJsonRPC jsonrpc(pipe_name);

  std::cout << " -- Method Registration:" << std::endl;
  std::cout << "   -- initialize" << std::endl;
  jsonrpc.register_method(
      "initialize", [](const RpcRequest& request) -> RpcResponse {
        auto response = request.create_object(
            {{"serverInfo", request.create_object({
                                {"name"_bv, "Tetrodotoxin Language Server"_bv},
                                {"version"_bv, "1.0"_bv},
                            })},
             {"capabilities",
              request.create_object({
                  {"positionEncoding"_bv, "utf-16"_bv},
                  {"textDocumentSync"_bv,
                   request.create_object(
                       {{"openClose"_bv, true}, {"change"_bv, "1"_bv}})},
              })}});

        return request.rpc_result(response);
      });

  std::cout << "   -- tokenize" << std::endl;
  jsonrpc.register_method(
      "tokenize", [](const RpcRequest& request) -> RpcResponse {
        const auto& args = request.get_params();
        if (args.null()) {
          return request.rpc_error("\"Failed to parse tokenize request.\"");
        }

        const auto source_code = args["source"].get_string();
        if (source_code.empty()) {
          return request.rpc_error(
              "Requested Fotokenizemat but no `source` was provided");
        }

        // One off tokenizer.
        Tokenizer tokenizer(request.get_arena());
        tokenizer.parse(
            Base64::Decoded(request.get_arena(), source_code).get_view(),
            false);

        return Service::lsp_tokens(tokenizer, request);
      });

  std::cout << "   -- format" << std::endl;
  jsonrpc.register_method(
      "format", [](const RpcRequest& request) -> RpcResponse {
        const auto& args = request.get_params();
        if (args.null()) {
          return request.rpc_error("\"Failed to parse format request.\"");
        }

        const auto source_code = args["source"].get_string();
        if (source_code.empty()) {
          return request.rpc_error(
              "Requested Format but no `source` was provided");
        }

        const auto name_string = args["name"].get_string();
        if (name_string.empty()) {
          return request.rpc_error(
              "Requested Format but no `name` was provided");
        }

        // auto path = std::filesystem::path(name_string->get_view());
        // std::string name = path.filename();

        // One off tokenizer.
        static Tokenizer tokenizer(request.get_arena());
        tokenizer.parse(
            Base64::Decoded(request.get_arena(), source_code).get_view(),
            false);

        return request.rpc_result(
            request.create_object({{"document"_bv, source_code}}));
      });

  std::cout << " -- Starting JsonRPC..." << std::endl;
  jsonrpc.process();

  return 0;
}