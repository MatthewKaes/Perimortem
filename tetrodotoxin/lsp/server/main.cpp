// Perimortem Engine
// Copyright © Matt Kaes

#include "language_server.hpp"

#include "parser/tokenizer.hpp"

#include <iostream>

using namespace Tetrodotoxin::LSP;

constexpr const char* LSP_SUPPORT = "3.17";

#define LSP_WRITE(klass)                                  \
  case Tetrodotoxin::Language::Parser::Classifier::klass: \
    info_stream << "[\"" #klass "\",";                    \
    break;

#define LSP_REDEFINE(category, klass)                     \
  case Tetrodotoxin::Language::Parser::Classifier::klass: \
    info_stream << "[\"" #category "\",";                 \
    break;

#define LSP_SKIP(klass)

#include <unordered_set>

auto lsp_tokens(Tetrodotoxin::Language::Parser::Tokenizer& tokenizer,
                std::string_view jsonrpc,
                int32_t id) -> std::string {
  Tetrodotoxin::Language::Parser::ClassifierFlags library_types =
      Tetrodotoxin::Language::Parser::Classifier::Package;
  std::unordered_set<std::string_view> imports;
  std::unordered_set<std::string_view> parameters;
  int scopes = 0;

  std::stringstream info_stream;
  // Assume basic jsonrpc 2.0 simplified header is enough.
  info_stream << "{\"jsonrpc\":" << jsonrpc << ",\"id\":" << id
              << ",\"result\":{\"color\":"
              << !tokenizer.get_options().has(
                     Tetrodotoxin::Language::Parser::TtxState::CppTheme)
              << ",\"tokens\":[";

  const auto& tokens = tokenizer.get_tokens();
  for (int i = 0; i < tokens.size(); i++) {
    const auto& token = tokens[i];
    switch (token.klass) {
      LSP_REDEFINE(A, Attribute)
      LSP_REDEFINE(I, Numeric)
      LSP_REDEFINE(N, Float)
      case Tetrodotoxin::Language::Parser::Classifier::Parameter:
        info_stream << "[\""
                       "P"
                       "\",";
        parameters.insert(
            std::string_view{(char*)token.data.data(), token.data.size()});
        break;
      case Tetrodotoxin::Language::Parser::Classifier::ScopeStart:
        info_stream << "[\""
                       "SS"
                       "\",";
        scopes += 1;
        break;
      case Tetrodotoxin::Language::Parser::Classifier::ScopeEnd:
        info_stream << "[\""
                       "SE"
                       "\",";

        scopes -= 1;
        if (scopes <= 0) {
          parameters.clear();
        }
        break;
        LSP_REDEFINE(GS, GroupStart)
        LSP_REDEFINE(GE, GroupEnd)
        LSP_REDEFINE(IS, IndexStart)
        LSP_REDEFINE(IE, IndexEnd)
        LSP_REDEFINE(_, Seperator)
        LSP_REDEFINE(E, EndStatement)
        LSP_REDEFINE(C, LessOp)
        LSP_REDEFINE(C, GreaterOp)
        LSP_REDEFINE(C, LessEqOp)
        LSP_REDEFINE(C, GreaterEqOp)
        LSP_REDEFINE(C, CmpOp)
        LSP_REDEFINE(C, AndOp)
        LSP_REDEFINE(C, OrOp)
        LSP_REDEFINE(O, Define)
        LSP_REDEFINE(O, AccessOp)
        LSP_REDEFINE(O, CallOp)
        LSP_REDEFINE(O, Assign)
        LSP_REDEFINE(O, AddAssign)
        LSP_REDEFINE(O, SubAssign)
        LSP_REDEFINE(O, AddOp)
        LSP_REDEFINE(O, SubOp)
        LSP_REDEFINE(O, DivOp)
        LSP_REDEFINE(O, MulOp)
        LSP_REDEFINE(O, ModOp)
        LSP_REDEFINE(O, NotOp)
        LSP_REDEFINE(Cm, Comment)
        LSP_REDEFINE(Z, This)
        LSP_REDEFINE(K, New)
        LSP_REDEFINE(Nm, OnLoad)
        LSP_REDEFINE(K, Init)
        LSP_REDEFINE(K, If)
        LSP_REDEFINE(K, For)
        LSP_REDEFINE(K, Else)
        LSP_REDEFINE(K, While)
        LSP_REDEFINE(K, Return)
        LSP_REDEFINE(K, True)
        LSP_REDEFINE(K, False)
        LSP_REDEFINE(D, FuncDef)
        LSP_REDEFINE(D, TypeDef)
        LSP_REDEFINE(L, Package)
        LSP_REDEFINE(L, Requires)
        LSP_REDEFINE(L, From)
        LSP_REDEFINE(K, Debug)
        LSP_REDEFINE(K, Warning)
        LSP_REDEFINE(K, Error)
        LSP_REDEFINE(M1, Constant)
        LSP_REDEFINE(M2, Dynamic)
        LSP_REDEFINE(M3, Hidden)
        LSP_REDEFINE(M4, Temporary)
      case Tetrodotoxin::Language::Parser::Classifier::String:
        if (i > 0) {
          switch (tokens[i - 1].klass) {
            case Tetrodotoxin::Language::Parser::Classifier::From:
              info_stream << "[\"In\",";
              break;
            default:
              info_stream << "[\"S\",";
              break;
          }
        } else {
          info_stream << "[\"S\",";
        };
        break;
      case Tetrodotoxin::Language::Parser::Classifier::Type:
        if (i < tokens.size() - 2 &&
            tokens[i + 1].klass ==
                Tetrodotoxin::Language::Parser::Classifier::Define) {
          info_stream << "[\"DT\",";
        } else if (i > 0 && library_types.has(tokens[i - 1].klass)) {
          info_stream << "[\"Nm\",";
          imports.insert(
              std::string_view{(char*)token.data.data(), token.data.size()});
        } else if (imports.contains(std::string_view{(char*)token.data.data(),
                                                     token.data.size()})) {
          info_stream << "[\"Nm\",";
        } else {
          info_stream << "[\"T\",";
        }
        break;
      case Tetrodotoxin::Language::Parser::Classifier::Identifier:
        if (i < tokens.size() - 2 &&
            tokens[i + 1].klass ==
                Tetrodotoxin::Language::Parser::Classifier::Define) {
          info_stream << "[\"DI\",";
        } else if (i > 0 &&
                   tokens[i - 1].klass ==
                       Tetrodotoxin::Language::Parser::Classifier::CallOp) {
          info_stream << "[\"Fu\",";
        } else if (i > 0 &&
                   tokens[i - 1].klass !=
                       Tetrodotoxin::Language::Parser::Classifier::AccessOp &&
                   parameters.contains(std::string_view{
                       (char*)token.data.data(), token.data.size()})) {
          info_stream << "[\"P\",";
        } else {
          info_stream << "[\"Id\",";
        }
        break;
      case Tetrodotoxin::Language::Parser::Classifier::EndOfStream:
      case Tetrodotoxin::Language::Parser::Classifier::None:
      case Tetrodotoxin::Language::Parser::Classifier::TOTAL_FLAGS:
        continue;
    }

    // LSP is zero indexed compared to LLVM, clang, and editors which starts at
    // one so we need to do a quick index conversion.
    const uint32_t start_line = token.location.line - 1;
    const uint32_t start_column = token.location.column - 1;
    // Tokens in TTX can only be one line so we can save compute / space and
    // just encode the length of the token.
    const uint32_t end_column =
        token.location.parse_index - token.location.source_index;
    info_stream << start_line << "," << start_column << "," << end_column;

    // Close data array.
    // We don't emit the End of File token so stop at the second to last token.
    if (i < tokens.size() - 2)
      info_stream << "],";
    else
      info_stream << "]";
  }
  info_stream << "]}}";

  return info_stream.str();
}

auto main(int argc, char* argv[]) -> int {
  const char* pipe_flag = "--pipe=";
  std::cout << "~~ TTX Lang Server ~~" << std::endl;
  std::cout << "[LSP_VERSION: " << LSP_SUPPORT << "]" << std::endl << std::endl;

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
        Tetrodotoxin::Language::Parser::Tokenizer tokenizer(
            Tetrodotoxin::Language::Parser::ByteView{
                (Tetrodotoxin::Language::Parser::Byte*)source_code.data(),
                source_code.size()});

        return lsp_tokens(tokenizer, jsonrpc.get<std::string>(),
                          id.get<int32_t>());
      });

  std::cout << " -- Starting JsonRPC..." << std::endl;
  jsonrpc.process();

  return 0;
}