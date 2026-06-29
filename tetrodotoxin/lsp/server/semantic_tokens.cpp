// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/lsp/server/semantic_tokens.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/lexical/tokenizer.hpp"
#include "tetrodotoxin/syntax/package.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin::Lsp;

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

static auto is_shader_only_filtered(Tetrodotoxin::Lexical::Class klass)
    -> Bool {
  using Type = Tetrodotoxin::Lexical::Class::Type;

  switch (klass.get_type()) {
  case Type::If:
  case Type::In:
  case Type::For:
  case Type::Break:
  case Type::Continue:
  case Type::Case:
  case Type::Else:
  case Type::Match:
  case Type::While:
    return True;

  default:
    return False;
  }
}

static auto has_newline(View::Bytes text) -> Bool {
  return Algorithm::search(text, "\n"_view) != Count(-1);
}

static auto classify_semantic_token(Tetrodotoxin::Lexical::Class klass)
    -> Signed_64 {
  using Type = Tetrodotoxin::Lexical::Class::Type;

  switch (klass.get_type()) {
  case Type::Comment:
  case Type::Disabled:
    return SemanticComment;

  case Type::String:
  case Type::Embedded:
  case Type::PackedData:
    return SemanticString;

  case Type::Numeric:
  case Type::Float:
  case Type::Bytes:
    return SemanticNumber;

  case Type::Attribute:
    return SemanticDecorator;

  case Type::Type:
  case Type::Alias:
  case Type::Enum:
  case Type::Shader:
  case Type::Entity:
  case Type::Object:
  case Type::Struct:
  case Type::Foreign:
  case Type::Library:
    return SemanticType;

  case Type::Addressable:
    return SemanticVariable;

  case Type::AddOp:
  case Type::SubOp:
  case Type::DivOp:
  case Type::MulOp:
  case Type::ModOp:
  case Type::LessOp:
  case Type::GreaterOp:
  case Type::LessEqOp:
  case Type::GreaterEqOp:
  case Type::CmpOp:
  case Type::NotEqOp:
  case Type::CallOp:
  case Type::AddressOp:
  case Type::SwizzleOp:
  case Type::SliceOp:
  case Type::PackingOp:
  case Type::NotOp:
  case Type::RangeOp:
  case Type::AndOp:
  case Type::OrOp:
  case Type::Assign:
  case Type::AddAssign:
  case Type::SubAssign:
  case Type::ScopeStart:
  case Type::ScopeEnd:
  case Type::PackingStart:
  case Type::PackingEnd:
  case Type::IndexStart:
  case Type::IndexEnd:
  case Type::Define:
  case Type::EndStatement:
  case Type::Discard:
    return SemanticOperator;

  case Type::Unknown:
  case Type::EndOfStream:
    return Signed_64(-1);

  default:
    return SemanticKeyword;
  }
}

auto Server::semantic_legend(Allocator::Arena& arena) -> Json::Node {
  Managed::Vector<Json::Node> token_types(arena);
  token_types.insert(Json::Node("namespace"_view));
  token_types.insert(Json::Node("type"_view));
  token_types.insert(Json::Node("class"_view));
  token_types.insert(Json::Node("parameter"_view));
  token_types.insert(Json::Node("variable"_view));
  token_types.insert(Json::Node("property"_view));
  token_types.insert(Json::Node("function"_view));
  token_types.insert(Json::Node("keyword"_view));
  token_types.insert(Json::Node("comment"_view));
  token_types.insert(Json::Node("string"_view));
  token_types.insert(Json::Node("number"_view));
  token_types.insert(Json::Node("operator"_view));
  token_types.insert(Json::Node("decorator"_view));

  Managed::Vector<Json::Node> token_modifiers(arena);

  Managed::Vector<Json::Member> legend(arena);
  legend.insert({"tokenTypes"_view, Json::Node(token_types.get_view())});
  legend.insert(
      {"tokenModifiers"_view, Json::Node(token_modifiers.get_view())});
  return Json::Node(legend.get_view());
}

auto Server::semantic_tokens_for(Allocator::Arena& arena, View::Bytes source)
    -> Json::Node {
  Managed::Vector<Json::Node> data(arena);

  if (source.is_empty()) {
    Managed::Vector<Json::Member> empty_result(arena);
    empty_result.insert({"data"_view, Json::Node(data.get_view())});
    return Json::Node(empty_result.get_view());
  }

  Tetrodotoxin::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(source, false);

  View::Vector<Tetrodotoxin::Lexical::Token> tokens = tokenizer.get_tokens();
  Tetrodotoxin::Lexical::Class package_kind =
      Tetrodotoxin::Syntax::Package::detect_kind(tokens);
  Bits_32 previous_line = 0;
  Bits_32 previous_column = 0;
  Bool emitted = False;

  for (Count i = 0; i < tokens.get_size(); i++) {
    Tetrodotoxin::Lexical::Token token = tokens[i];

    if (package_kind == Tetrodotoxin::Lexical::Class::Type::Shader &&
        is_shader_only_filtered(token.get_class())) {
      continue;
    }

    View::Bytes text = token.get_text();
    if (text.is_empty() || has_newline(text)) {
      continue;
    }

    Signed_64 token_type = classify_semantic_token(token.get_class());
    if (token_type < 0) {
      continue;
    }

    Bits_32 line = token.get_line() - 1;
    Bits_32 column = token.get_column() - 1;
    Bits_32 delta_line = emitted ? line - previous_line : line;
    Bits_32 delta_column =
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

  Managed::Vector<Json::Member> result(arena);
  result.insert({"data"_view, Json::Node(data.get_view())});
  return Json::Node(result.get_view());
}
