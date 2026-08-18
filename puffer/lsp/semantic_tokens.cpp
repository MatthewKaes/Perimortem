// Perimortem Engine
// Copyright © Matt Kaes

#include "puffer/lsp/semantic_tokens.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/json/blueprint.hpp"

#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Puffer;
using namespace Ttx::Lexical;

enum SemanticToken : Signed_64 {
  SemanticNamespace,
  SemanticType,
  SemanticClass,
  SemanticParameter,
  SemanticVariable,
  SemanticProperty,
  SemanticFunction,
  SemanticKeyword,
  SemanticComment,
  SemanticString,
  SemanticNumber,
  SemanticOperator,
  SemanticDecorator,
};

static auto should_filter_shader_keyword(Code code) -> Bool {
  switch (code.get_type()) {
  case Code::Type::If:
  case Code::Type::In:
  case Code::Type::For:
  case Code::Type::Break:
  case Code::Type::Continue:
  case Code::Type::Case:
  case Code::Type::Else:
  case Code::Type::Match:
  case Code::Type::While:
    return True;

  default:
    return False;
  }
}

static auto has_newline(View::Bytes text) -> Bool {
  return Algorithm::search(text, "\n"_view) != Count(-1);
}

static auto classify_semantic_token(Code code) -> Signed_64 {
  switch (code.get_type()) {
  case Code::Type::Comment:
    return SemanticComment;

  case Code::Type::String:
  case Code::Type::Embedded:
  case Code::Type::PackedData:
    return SemanticString;

  case Code::Type::Numeric:
  case Code::Type::Hex:
  case Code::Type::Float:
  case Code::Type::Bytes:
    return SemanticNumber;

  case Code::Type::Attribute:
    return SemanticDecorator;

  case Code::Type::Type:
  case Code::Type::Alias:
    return SemanticType;

  case Code::Type::Addressable:
    return SemanticVariable;

  case Code::Type::AddOp:
  case Code::Type::SubOp:
  case Code::Type::DivOp:
  case Code::Type::MulOp:
  case Code::Type::ModOp:
  case Code::Type::LessOp:
  case Code::Type::GreaterOp:
  case Code::Type::LessEqOp:
  case Code::Type::GreaterEqOp:
  case Code::Type::CmpOp:
  case Code::Type::NotEqOp:
  case Code::Type::CallOp:
  case Code::Type::AddressOp:
  case Code::Type::SwizzleOp:
  case Code::Type::ValueAccessOp:
  case Code::Type::PackingOp:
  case Code::Type::NotOp:
  case Code::Type::RangeOp:
  case Code::Type::AndOp:
  case Code::Type::OrOp:
  case Code::Type::Assign:
  case Code::Type::AddAssign:
  case Code::Type::SubAssign:
  case Code::Type::ScopeStart:
  case Code::Type::ScopeEnd:
  case Code::Type::PackingStart:
  case Code::Type::PackingEnd:
  case Code::Type::BracketStart:
  case Code::Type::BracketEnd:
  case Code::Type::Define:
  case Code::Type::TypeAccessOp:
  case Code::Type::EndStatement:
  case Code::Type::Discard:
    return SemanticOperator;

  case Code::Type::Unknown:
  case Code::Type::Terminal:
    return Signed_64(-1);

  default:
    return SemanticKeyword;
  }
}

static auto source_dialect(View::Vector<Token> tokens, View::Bytes source)
    -> View::Bytes {
  for (Count i = 0; i < tokens.get_size(); i++) {
    if (tokens[i].get_code() != Code::Type::Dialect) {
      continue;
    }

    Bool has_define = False;
    for (Count j = i + 1; j < tokens.get_size(); j++) {
      Code code = tokens[j].get_code();
      if (code == Code::Type::EndStatement) {
        break;
      }
      if (code == Code::Type::Define) {
        has_define = True;
      } else if (has_define && code == Code::Type::Type) {
        return tokens[j].caculate_text(source);
      }
    }
  }

  return "Library"_view;
}

static auto contextual_semantic_token(View::Vector<Token> tokens, Count index)
    -> Signed_64 {
  Code code = tokens[index].get_code();
  if (code != Code::Type::Addressable) {
    return classify_semantic_token(code);
  }

  Code previous =
      index == 0 ? Code::Type::Unknown : tokens[index - 1].get_code();
  if (previous == Code::Type::CallOp || previous == Code::Type::Func) {
    return SemanticFunction;
  }
  if (previous == Code::Type::AddressOp) {
    return SemanticProperty;
  }
  if (index + 2 < tokens.get_size() &&
      tokens[index + 1].get_code() == Code::Type::Define &&
      tokens[index + 2].get_code() == Code::Type::Func) {
    return SemanticFunction;
  }

  return SemanticVariable;
}

auto Lsp::semantic_legend(Allocator::Arena& arena) -> Json::Node {
  return Json::Blueprint{
    {
      {"tokenTypes"_view,
       {
         "namespace"_view,
         "type"_view,
         "class"_view,
         "parameter"_view,
         "variable"_view,
         "property"_view,
         "function"_view,
         "keyword"_view,
         "comment"_view,
         "string"_view,
         "number"_view,
         "operator"_view,
         "decorator"_view,
       }},
      Json::Blueprint::empty_array("tokenModifiers"_view),
    }}.construct(arena);
}

auto Lsp::semantic_tokens_for(Allocator::Arena& arena, View::Bytes source)
    -> Json::Node {
  Managed::Vector<Json::Node> data(arena);
  if (source.is_empty()) {
    const Json::Node data_node(data.get_view());
    return Json::Blueprint{
      {
        {"data"_view, data_node},
      }}.construct(arena);
  }

  Tokenizer tokenizer(arena, source, "lsp-buffer.ttx"_view);
  View::Vector<Token> tokens = tokenizer.get_tokens();
  Errors errors;
  Ttx::Lexical::Associations associations(tokenizer.get_arena());
  Cursor cursor(tokenizer, errors, associations);

  View::Bytes dialect = source_dialect(tokens, cursor.get_source_text());
  Unsigned_32 previous_line = 0;
  Unsigned_32 previous_column = 0;
  Bool emitted = False;
  for (Count i = 0; i < tokens.get_size(); i++) {
    Token token = tokens[i];
    if (dialect == "Shader"_view &&
        should_filter_shader_keyword(token.get_code())) {
      continue;
    }

    View::Bytes text = token.caculate_text(cursor.get_source_text());
    if (text.is_empty() || has_newline(text)) {
      continue;
    }

    Signed_64 token_type = contextual_semantic_token(tokens, i);
    if (token_type < 0) {
      continue;
    }

    Unsigned_32 line = token.get_line() - 1;
    Unsigned_32 column = token.get_column() - 1;
    Unsigned_32 delta_line = emitted ? line - previous_line : line;
    Unsigned_32 delta_column =
        emitted && delta_line == 0 ? column - previous_column : column;

    data.insert(Json::Node(Signed_64(delta_line)));
    data.insert(Json::Node(Signed_64(delta_column)));
    data.insert(Json::Node(Signed_64(text.get_size())));
    data.insert(Json::Node(token_type));
    data.insert(Json::Node(Signed_64(0)));

    previous_line = line;
    previous_column = column;
    emitted = True;
  }

  const Json::Node data_node(data.get_view());
  return Json::Blueprint{
    {
      {"data"_view, data_node},
    }}.construct(arena);
}
