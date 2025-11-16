// Perimortem Engine
// Copyright © Matt Kaes

#include "src/language_server.hpp"
#include "src/service.hpp"

#include "lexical/tokenizer.hpp"

#include <iostream>

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
      "initialize",
      [](const json& jsonrpc, const json& id, const json& data) -> std::string {
        json ServerInfo;
        ServerInfo["name"] = "Tetrodotoxin Language Server";
        ServerInfo["version"] = "1.0";

        json CompletionOptions;
        CompletionOptions["resolveProvider"] = true;
        CompletionOptions["triggerCharacters"] = json::array({".", ">"});
        CompletionOptions["completionItem"] = {{"labelDetailsSupport", true}};

        json TextDocumentSyncOptions;
        TextDocumentSyncOptions["openClose"] = true;
        TextDocumentSyncOptions["change"] = "1";  // Send entire script

        json ServerCapabilities;
        ServerCapabilities["positionEncoding"] = "utf-16";
        ServerCapabilities["completionProvider"] = CompletionOptions;
        ServerCapabilities["textDocumentSync"] = TextDocumentSyncOptions;

        json InitializeResult;
        InitializeResult["capabilities"] = ServerCapabilities;
        InitializeResult["serverInfo"] = ServerInfo;

        json result_response;
        result_response["result"] = InitializeResult;
        result_response["jsonrpc"] = jsonrpc;
        result_response["id"] = id;

        return result_response.dump();
      });

  std::cout << "   -- tokenize" << std::endl;
  jsonrpc.register_method(
      "tokenize",
      [](const json& jsonrpc, const json& id, const json& data) -> std::string {
        if (!data.contains("source")) {
          std::stringstream result;
          result << "{\"jsonrpc\": \"2.0\", \"id\":" << id.get<uint32_t>()
                 << " \"error\": \"Requested Tokenization but no `source` was "
                    "provided!\"}";
          return result.str();
        }

        std::string source_code = data["source"].get<std::string>();
        Tokenizer tokenizer;
        tokenizer.parse(source_code, false);

        return Service::lsp_tokens(tokenizer, jsonrpc.get<std::string>(),
                                   id.get<int32_t>());
      });

  std::cout << "   -- format" << std::endl;
  jsonrpc.register_method(
      "format",
      [](const json& jsonrpc, const json& id, const json& data) -> std::string {
        if (!data.contains("source")) {
          std::stringstream result;
          result << "{\"jsonrpc\": \"2.0\", \"id\":" << id.get<uint32_t>()
                 << " \"error\": \"Requested Format but no `source` was "
                    "provided!\"}";
          return result.str();
        }

        if (!data.contains("name")) {
          std::stringstream result;
          result << "{\"jsonrpc\": \"2.0\", \"id\":" << id.get<uint32_t>()
                 << " \"error\": \"Requested Format but no `name` was "
                    "provided!\"}";
          return result.str();
        }
        std::string source_code = data["source"].get<std::string>();
        auto path = std::filesystem::path(data["name"].get<std::string>());
        path.replace_extension();
        std::string name = path.filename();

        Tokenizer tokenizer;
        tokenizer.parse(source_code, false);

        return Service::format(tokenizer, name, jsonrpc.get<std::string>(),
                               id.get<int32_t>());
      });

  std::cout << " -- Starting JsonRPC..." << std::endl;
  jsonrpc.process();

  return 0;
}