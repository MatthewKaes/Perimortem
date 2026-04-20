// Perimortem Engine
// Copyright © Matt Kaes

#include "service.hpp"

#include "lexical/tokenizer.hpp"

using namespace Tetrodotoxin::Lsp;
using namespace Tetrodotoxin::Lexical;
using namespace Perimortem::Memory;

using NodeArray =
    Perimortem::Memory::Managed::Vector<Perimortem::Storage::Json::Node>;
using NodeObject =
    Perimortem::Memory::Managed::Table<Perimortem::Storage::Json::Node>;

auto recursive_strip(const Tetrodotoxin::Lexical::Tokenizer::Tokens stream,
                     uint32_t& index) -> void {
  while (index < stream.get_size() - 1) {
    switch (stream[index].klass) {
      case Classifier::ScopeEnd:
        return;
      case Classifier::ScopeStart:
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

auto Service::lsp_tokens(Tokenizer& tokenizer, const RpcRequest& header)
    -> Perimortem::Storage::Json::Node {
  ClassifierFlags library_types = Classifier::Package;
  Managed::Vector<View::Bytes> imports(header.get_arena());
  Managed::Vector<View::Bytes> parameters(header.get_arena());
  uint32_t scopes = 0;

  NodeArray token_stream(header.get_arena());
  const auto& tokens = tokenizer.get_tokens();
  for (uint32_t i = 0; i < tokens.get_size(); i++) {
    NodeArray token_data(header.get_arena());
    const auto& token = tokens[i];
    switch (token.klass) {
      case Classifier::Attribute:
        token_data.insert("A"_view);
      case Classifier::Numeric:
        token_data.insert("I"_view);
      case Classifier::Float:
        token_data.insert("N"_view);
      case Classifier::GroupStart:
        token_data.insert("GS"_view);
      case Classifier::GroupEnd:
        token_data.insert("GE"_view);
      case Classifier::IndexStart:
        token_data.insert("IS"_view);
      case Classifier::IndexEnd:
        token_data.insert("IE"_view);
      case Classifier::Seperator:
        token_data.insert("_"_view);
      case Classifier::EndStatement:
        token_data.insert("E"_view);
      case Classifier::LessOp:
      case Classifier::GreaterOp:
      case Classifier::LessEqOp:
      case Classifier::GreaterEqOp:
      case Classifier::CmpOp:
      case Classifier::AndOp:
      case Classifier::OrOp:
        token_data.insert("C"_view);
      case Classifier::Define:
      case Classifier::AccessOp:
      case Classifier::CallOp:
      case Classifier::Assign:
      case Classifier::AddAssign:
      case Classifier::SubAssign:
      case Classifier::AddOp:
      case Classifier::SubOp:
      case Classifier::DivOp:
      case Classifier::MulOp:
      case Classifier::ModOp:
      case Classifier::NotOp:
        token_data.insert("O"_view);
      case Classifier::Comment:
        token_data.insert("Cm"_view);
      case Classifier::Self:
        token_data.insert("Z"_view);
      case Classifier::OnLoad:
        token_data.insert("Nm"_view);
      case Classifier::New:
      case Classifier::Init:
      case Classifier::If:
      case Classifier::For:
      case Classifier::Else:
      case Classifier::While:
      case Classifier::Return:
      case Classifier::True:
      case Classifier::False:
      case Classifier::Debug:
      case Classifier::Warning:
      case Classifier::Error:
        token_data.insert("K"_view);
      case Classifier::Func:
      case Classifier::Alias:
      case Classifier::Object:
      case Classifier::Struct:
        token_data.insert("D"_view);
      case Classifier::Entity:
      case Classifier::Library:
        token_data.insert("P"_view);
      case Classifier::Package:
      case Classifier::Using:
      case Classifier::As:
      case Classifier::Via:
        token_data.insert("L"_view);
      case Classifier::Constant:
        token_data.insert("M1"_view);
      case Classifier::Dynamic:
        token_data.insert("M2"_view);
      case Classifier::Hidden:
        token_data.insert("M3"_view);
      case Classifier::Temporary:
        token_data.insert("M4"_view);
      case Classifier::Parameter:
        token_data.insert("P"_view);
        parameters.insert(token.data);
        break;
      case Classifier::ScopeStart:
        token_data.insert("SS"_view);
        scopes += 1;
        break;
      case Classifier::ScopeEnd:
        token_data.insert("SE"_view);
        scopes -= 1;
        if (scopes <= 0) {
          parameters.clear();
        }
        break;
      case Classifier::String:
        if (i > 0 && tokens[i - 1].klass == Classifier::Via) {
          token_data.insert("In"_view);
        } else {
          token_data.insert("S"_view);
        };
        break;
      case Classifier::Type:
        if (i < tokens.get_size() - 2 &&
            tokens[i + 1].klass == Classifier::Define) {
          token_data.insert("DT"_view);
        } else if (i > 0 && library_types.has(tokens[i - 1].klass)) {
          token_data.insert("Nm"_view);
          imports.insert(token.data);
        } else if (imports.contains(token.data)) {
          token_data.insert("Nm"_view);
        } else {
          token_data.insert("T"_view);
        }
        break;
      case Classifier::Identifier:
        if (i < tokens.get_size() - 2 &&
            tokens[i + 1].klass == Classifier::Define) {
          token_data.insert("DI"_view);
        } else if (i > 0 && tokens[i - 1].klass == Classifier::CallOp) {
          token_data.insert("Fu"_view);
        } else if (i > 0 && tokens[i - 1].klass != Classifier::AccessOp &&
                   parameters.contains(token.data)) {
          token_data.insert("P"_view);
        } else {
          token_data.insert("Id"_view);
        }
        break;

        // Disabled blocks require a bit of extra parsing for highlighting, but
        // it does condense the entire range into a single tag.
      case Classifier::Disabled: {
        i += 1;
        for (; i < tokens.get_size() - 1; i++) {
          auto& last_token = tokens[i - 1];
          auto& next_token = tokens[i];
          // Check if we pass through a newline between tokens. If we do then
          // break.
          auto source_view = tokenizer.get_source();

          Bool found_newline = false;
          for (uint32_t k = last_token.location.parse_index;
               k < next_token.location.source_index; k++) {
            if (source_view[k] == '\n') {
              found_newline = true;
              break;
            }
          }

          if (found_newline) {
            break;
          }

          // We didn't pass through a newline so check if we started a scope.
          // If we did then disable it and all subscopes.
          if (next_token.klass == Classifier::ScopeStart) {
            recursive_strip(tokens, ++i);
          }
        }

        i -= 1;
        auto& end_token = tokens[i];

        // Lsp is zero indexed compared to LLVM, clang, and editors which
        // starts at one so we need to do a quick index conversion.
        const int64_t start_line = token.location.line - 1;
        const int64_t start_column = token.location.column - 1;
        // Tokens in TTX can only be one line so we can save compute / space
        // and just encode the length of the token.
        const int64_t end_line = end_token.location.line - 1;
        const int64_t end_column = end_token.location.column - 1 +
                                   end_token.location.parse_index -
                                   end_token.location.source_index;

        token_data.insert(start_line);
        token_data.insert(start_column);
        token_data.insert(end_line);
        token_data.insert(end_column);
        continue;
      }
      case Classifier::EndOfStream:
      case Classifier::None:
      case Classifier::TOTAL_FLAGS:
        continue;
    }

    // Lsp is zero indexed compared to LLVM, clang, and editors which starts at
    // one so we need to do a quick index conversion.
    const int64_t start_line = token.location.line - 1;
    const int64_t start_column = token.location.column - 1;
    const int64_t end_line = token.location.line - 1;
    const int64_t end_column = token.location.column - 1 +
                               token.location.parse_index -
                               token.location.source_index;
    token_data.insert(start_line);
    token_data.insert(start_column);
    token_data.insert(end_line);
    token_data.insert(end_column);
  }

  NodeObject result(header.get_arena());
  result.insert("color"_view, !tokenizer.get_options().has(TtxState::CppTheme));
  result.insert("tokens"_view, token_stream.get_view());
  return result.get_view();
}
