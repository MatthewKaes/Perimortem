// Perimortem Engine
// Copyright © Matt Kaes

#include "service.hpp"

#include "formatter.hpp"
#include "lexical/tokenizer.hpp"

#include <iostream>
#include <sstream>
#include <unordered_set>

using namespace Tetrodotoxin::Lsp;

#define Lsp_WRITE(klass)                                  \
  case Tetrodotoxin::Lexical::Classifier::klass: \
    info_stream << "[\"" #klass "\",";                    \
    break;

#define Lsp_REDEFINE(category, klass)                     \
  case Tetrodotoxin::Lexical::Classifier::klass: \
    info_stream << "[\"" #category "\",";                 \
    break;

#define Lsp_SKIP(klass)

auto recursive_strip(const Tetrodotoxin::Lexical::TokenStream& stream,
                     uint32_t& index) -> void {
  while (index < stream.size() - 1) {
    switch (stream[index].klass) {
      case Tetrodotoxin::Lexical::Classifier::ScopeEnd:
        return;
      case Tetrodotoxin::Lexical::Classifier::ScopeStart:
        index++;
        recursive_strip(stream, index);
        index++;
        break;
      default:
        index++;
        break;
    }
  }
}

auto Service::lsp_tokens(Tetrodotoxin::Lexical::Tokenizer& tokenizer,
                         std::string_view jsonrpc,
                         int32_t id) -> std::string {
  Tetrodotoxin::Lexical::ClassifierFlags library_types =
      Tetrodotoxin::Lexical::Classifier::Package;
  std::unordered_set<std::string_view> imports;
  std::unordered_set<std::string_view> parameters;
  uint32_t scopes = 0;

  std::stringstream info_stream;
  // Assume basic jsonrpc 2.0 simplified header is enough.
  info_stream << "{\"jsonrpc\":\"" << jsonrpc << "\",\"id\":" << id
              << ",\"result\":{\"color\":"
              << !tokenizer.get_options().has(
                     Tetrodotoxin::Lexical::TtxState::CppTheme)
              << ",\"tokens\":[";

  const auto& tokens = tokenizer.get_tokens();
  for (uint32_t i = 0; i < tokens.size(); i++) {
    const auto& token = tokens[i];
    switch (token.klass) {
      Lsp_REDEFINE(A, Attribute);
      Lsp_REDEFINE(I, Numeric);
      Lsp_REDEFINE(N, Float);
      Lsp_REDEFINE(GS, GroupStart);
      Lsp_REDEFINE(GE, GroupEnd);
      Lsp_REDEFINE(IS, IndexStart);
      Lsp_REDEFINE(IE, IndexEnd);
      Lsp_REDEFINE(_, Seperator);
      Lsp_REDEFINE(E, EndStatement);
      Lsp_REDEFINE(C, LessOp);
      Lsp_REDEFINE(C, GreaterOp);
      Lsp_REDEFINE(C, LessEqOp);
      Lsp_REDEFINE(C, GreaterEqOp);
      Lsp_REDEFINE(C, CmpOp);
      Lsp_REDEFINE(C, AndOp);
      Lsp_REDEFINE(C, OrOp);
      Lsp_REDEFINE(O, Define);
      Lsp_REDEFINE(O, AccessOp);
      Lsp_REDEFINE(O, CallOp);
      Lsp_REDEFINE(O, Assign);
      Lsp_REDEFINE(O, AddAssign);
      Lsp_REDEFINE(O, SubAssign);
      Lsp_REDEFINE(O, AddOp);
      Lsp_REDEFINE(O, SubOp);
      Lsp_REDEFINE(O, DivOp);
      Lsp_REDEFINE(O, MulOp);
      Lsp_REDEFINE(O, ModOp);
      Lsp_REDEFINE(O, NotOp);
      Lsp_REDEFINE(Cm, Comment);
      Lsp_REDEFINE(Z, Self);
      Lsp_REDEFINE(K, New);
      Lsp_REDEFINE(Nm, OnLoad);
      Lsp_REDEFINE(K, Init);
      Lsp_REDEFINE(K, If);
      Lsp_REDEFINE(K, For);
      Lsp_REDEFINE(K, Else);
      Lsp_REDEFINE(K, While);
      Lsp_REDEFINE(K, Return);
      Lsp_REDEFINE(K, True);
      Lsp_REDEFINE(K, False);
      Lsp_REDEFINE(D, Func);
      Lsp_REDEFINE(D, Alias);
      Lsp_REDEFINE(D, Object);
      Lsp_REDEFINE(D, Struct);
      Lsp_REDEFINE(P, Entity);
      Lsp_REDEFINE(P, Library);
      Lsp_REDEFINE(L, Package);
      Lsp_REDEFINE(L, Using);
      Lsp_REDEFINE(L, As);
      Lsp_REDEFINE(L, Via);
      Lsp_REDEFINE(K, Debug);
      Lsp_REDEFINE(K, Warning);
      Lsp_REDEFINE(K, Error);
      Lsp_REDEFINE(M1, Constant);
      Lsp_REDEFINE(M2, Dynamic);
      Lsp_REDEFINE(M3, Hidden);
      Lsp_REDEFINE(M4, Temporary);
      case Tetrodotoxin::Lexical::Classifier::Parameter:
        info_stream << "[\""
                       "P"
                       "\",";
        parameters.insert(
            std::string_view{(char*)token.data.data(), token.data.size()});
        break;
      case Tetrodotoxin::Lexical::Classifier::ScopeStart:
        info_stream << "[\""
                       "SS"
                       "\",";
        scopes += 1;
        break;
      case Tetrodotoxin::Lexical::Classifier::ScopeEnd:
        info_stream << "[\""
                       "SE"
                       "\",";

        scopes -= 1;
        if (scopes <= 0) {
          parameters.clear();
        }
        break;
      case Tetrodotoxin::Lexical::Classifier::String:
        if (i > 0) {
          switch (tokens[i - 1].klass) {
            case Tetrodotoxin::Lexical::Classifier::Via:
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
      case Tetrodotoxin::Lexical::Classifier::Type:
        if (i < tokens.size() - 2 &&
            tokens[i + 1].klass ==
                Tetrodotoxin::Lexical::Classifier::Define) {
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
      case Tetrodotoxin::Lexical::Classifier::Identifier:
        if (i < tokens.size() - 2 &&
            tokens[i + 1].klass ==
                Tetrodotoxin::Lexical::Classifier::Define) {
          info_stream << "[\"DI\",";
        } else if (i > 0 &&
                   tokens[i - 1].klass ==
                       Tetrodotoxin::Lexical::Classifier::CallOp) {
          info_stream << "[\"Fu\",";
        } else if (i > 0 &&
                   tokens[i - 1].klass !=
                       Tetrodotoxin::Lexical::Classifier::AccessOp &&
                   parameters.contains(std::string_view{
                       (char*)token.data.data(), token.data.size()})) {
          info_stream << "[\"P\",";
        } else {
          info_stream << "[\"Id\",";
        }
        break;

        // Disabled blocks require a bit of extra parsing for highlighting, but
        // it does condense the entire range into a single tag.
      case Tetrodotoxin::Lexical::Classifier::Disabled: {
        info_stream << "[\"Dis\",";
        i += 1;
        for (; i < tokens.size() - 1; i++) {
          auto& last_token = tokens[i - 1];
          auto& next_token = tokens[i];
          // Check if we pass through a newline between tokens. If we do then
          // break.
          auto source_view = tokenizer.get_source();

          bool found_newline = false;
          for (uint32_t k = last_token.location.parse_index;
               k < next_token.location.source_index; k++) {
            if (source_view[k] == '\n') {
              found_newline = true;
              break;
            }
          }

          if (found_newline)
            break;

          // We didn't pass through a newline so check if we need to start a new
          // scope.
          if (next_token.klass ==
              Tetrodotoxin::Lexical::Classifier::ScopeStart) {
            recursive_strip(tokens, ++i);
          }
        }

        i -= 1;
        auto& end_token = tokens[i];

        // Lsp is zero indexed compared to LLVM, clang, and editors which starts
        // at one so we need to do a quick index conversion.
        const uint32_t start_line = token.location.line - 1;
        const uint32_t start_column = token.location.column - 1;
        // Tokens in TTX can only be one line so we can save compute / space and
        // just encode the length of the token.
        const uint32_t end_line = end_token.location.line - 1;
        const uint32_t end_column = end_token.location.column - 1 +
                                    end_token.location.parse_index -
                                    end_token.location.source_index;
        info_stream << start_line << "," << start_column << "," << end_line
                    << "," << end_column;

        // Close data array.
        // We don't emit the End of File token so stop at the second to last
        // token.
        if (i < tokens.size() - 2)
          info_stream << "],";
        else
          info_stream << "]";

        // Skip emission since we handled it in the block.
        continue;
      }
      case Tetrodotoxin::Lexical::Classifier::EndOfStream:
      case Tetrodotoxin::Lexical::Classifier::None:
      case Tetrodotoxin::Lexical::Classifier::TOTAL_FLAGS:
        continue;
    }

    // Lsp is zero indexed compared to LLVM, clang, and editors which starts at
    // one so we need to do a quick index conversion.
    const uint32_t start_line = token.location.line - 1;
    const uint32_t start_column = token.location.column - 1;
    const uint32_t end_line = token.location.line - 1;
    const uint32_t end_column = token.location.column - 1 +
                                token.location.parse_index -
                                token.location.source_index;
    info_stream << start_line << "," << start_column << "," << end_line << ","
                << end_column;

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

auto Service::format(Tetrodotoxin::Lexical::Tokenizer& tokenizer,
                     std::string_view name,
                     std::string_view jsonrpc,
                     int32_t id) -> std::string {
  Formatter formatter;
  formatter.tokenized_format(tokenizer, name);
  std::string document = formatter.get_content();

  // Escape all quotes in comments as they break JsonRPC.
  auto pos = document.find('"');
  while (pos != std::string::npos) {
    document.replace(pos, 1, "\\\"");
    pos = document.find('"', pos + 2);
  }

  std::stringstream info_stream;
  // Assume basic jsonrpc 2.0 simplified header is enough.
  info_stream << "{\"jsonrpc\":\"" << jsonrpc << "\",\"id\":" << id
              << ",\"result\":{\"document\":\"" << document << "\"}}";

  return info_stream.str();
}