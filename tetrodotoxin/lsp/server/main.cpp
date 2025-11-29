// Perimortem Engine
// Copyright © Matt Kaes

#include "src/language_server.hpp"
#include "src/service.hpp"

#include "core/storage/formats/json.hpp"
#include "lexical/tokenizer.hpp"

#include <iostream>
#include <sstream>

using namespace Perimortem::Memory;
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
      [](Arena& arena, const ManagedString& source,
         const RpcHeader& info) -> std::string {
        std::stringstream result;
        result << "{\"jsonrpc\":\"" << info.get_version().get_view()
               << "\",\"id\":" << info.get_id() << ",\"result\":{";
        result << "\"serverInfo\":{\"name\":\"Tetrodotoxin Language "
                  "Server\",\"version\":\"1.0\"},";
        result << "\"capabilities\":{\"positionEncoding\":\"utf-16\",";
        // TODO: Add support for completion characters [\".\",\">\"]
        // result << "\"completionProvider\":{"
        //           "\"resolveProvider\":true,"
        //           "\"triggerCharacters\":[\".\",\">\"],"
        //           "\"completionItem\":{\"labelDetailsSupport\":true}"
        //           "},";
        result << "\"textDocumentSync\":{"
                  "\"openClose\":true,"
                  "\"change\":\"1\""
                  "}";
        result << "}}}";

        return result.str();
      });

  std::cout << "   -- tokenize" << std::endl;
  jsonrpc.register_method(
      "tokenize",
      [](Arena& arena, const ManagedString& source,
         const RpcHeader& info) -> std::string {
        std::stringstream result;
        auto position = info.get_params_offset();
        auto data = Perimortem::Storage::Json::parse(arena, source, position);
        if (!data->at("source")->null()) {
          result << "{\"jsonrpc\": \"" << info.get_version().get_view()
                 << "\", \"id\":" << info.get_id()
                 << " \"error\": \"Requested Tokenization but no `source` was "
                    "provided!\"}";
          return result.str();
        }

        // One off tokenizer.
        static Tokenizer tokenizer;
        tokenizer.parse(data->at("source")->get_string(), false);

        return Service::lsp_tokens(tokenizer, info);
      });

  std::cout << "   -- format" << std::endl;
  jsonrpc.register_method(
      "format",
      [](Arena& arena, const ManagedString& source,
         const RpcHeader& info) -> std::string {
        std::stringstream result;
        auto position = info.get_params_offset();
        auto data = Perimortem::Storage::Json::parse(arena, source, position);
        if (data->null()) {
          result << "{\"jsonrpc\": \"" << info.get_version().get_view()
                 << "\", \"id\":" << info.get_id()
                 << " \"error\": \"Failed to parse format request.\"}";
          return result.str();
        }

        const auto source_code = data->at("source")->get_string();
        if (!data->contains("source")) {
          result << "{\"jsonrpc\": \"" << info.get_version().get_view()
                 << "\", \"id\":" << info.get_id()
                 << " \"error\": \"Requested Format but no `source` was "
                    "provided!\"}";
          return result.str();
        }

        const auto name_string = data->at("name");
        if (!name_string) {
          std::stringstream result;
          result << "{\"jsonrpc\": \"" << info.get_version().get_view()
                 << "\", \"id\":" << info.get_id()
                 << " \"error\": \"Requested Format but no `name` was "
                    "provided!\"}";
          return result.str();
        }

        // auto path = std::filesystem::path(name_string->get_view());
        // std::string name = path.filename();

        Tokenizer tokenizer;
        tokenizer.parse(source_code, false);

        // TODO: For now just return back the source.
        result << "{\"jsonrpc\": \"" << info.get_version().get_view()
               << "\", \"id\":" << info.get_id()
               << ",\"result\":{\"document\":\"" << source_code.get_view()
               << "\"}}";
        return result.str();
      });

  std::cout << " -- Starting JsonRPC..." << std::endl;
  jsonrpc.process();

  return 0;
}