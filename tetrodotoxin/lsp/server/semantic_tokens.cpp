// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/lsp/server/semantic_tokens.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "ttx/dialect/source/source.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/tokenizer.hpp"

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

static auto should_filter_shader_keyword(::Ttx::Lexical::Class klass)
    -> Bool {
  switch (klass.get_type()) {
  case ::Ttx::Lexical::Class::Type::If:
  case ::Ttx::Lexical::Class::Type::In:
  case ::Ttx::Lexical::Class::Type::For:
  case ::Ttx::Lexical::Class::Type::Break:
  case ::Ttx::Lexical::Class::Type::Continue:
  case ::Ttx::Lexical::Class::Type::Case:
  case ::Ttx::Lexical::Class::Type::Else:
  case ::Ttx::Lexical::Class::Type::Match:
  case ::Ttx::Lexical::Class::Type::While:
    return True;

  default:
    return False;
  }
}

static auto has_newline(View::Bytes text) -> Bool {
  return Algorithm::search(text, "\n"_view) != Count(-1);
}

static auto classify_semantic_token(::Ttx::Lexical::Class klass)
    -> Signed_64 {
  switch (klass.get_type()) {
  case ::Ttx::Lexical::Class::Type::Comment:
  case ::Ttx::Lexical::Class::Type::Disabled:
    return SemanticComment;

  case ::Ttx::Lexical::Class::Type::String:
  case ::Ttx::Lexical::Class::Type::Embedded:
  case ::Ttx::Lexical::Class::Type::PackedData:
    return SemanticString;

  case ::Ttx::Lexical::Class::Type::Numeric:
  case ::Ttx::Lexical::Class::Type::Float:
  case ::Ttx::Lexical::Class::Type::Bytes:
    return SemanticNumber;

  case ::Ttx::Lexical::Class::Type::Attribute:
    return SemanticDecorator;

  case ::Ttx::Lexical::Class::Type::Type:
  case ::Ttx::Lexical::Class::Type::Alias:
    return SemanticType;

  case ::Ttx::Lexical::Class::Type::Addressable:
    return SemanticVariable;

  case ::Ttx::Lexical::Class::Type::AddOp:
  case ::Ttx::Lexical::Class::Type::SubOp:
  case ::Ttx::Lexical::Class::Type::DivOp:
  case ::Ttx::Lexical::Class::Type::MulOp:
  case ::Ttx::Lexical::Class::Type::ModOp:
  case ::Ttx::Lexical::Class::Type::LessOp:
  case ::Ttx::Lexical::Class::Type::GreaterOp:
  case ::Ttx::Lexical::Class::Type::LessEqOp:
  case ::Ttx::Lexical::Class::Type::GreaterEqOp:
  case ::Ttx::Lexical::Class::Type::CmpOp:
  case ::Ttx::Lexical::Class::Type::NotEqOp:
  case ::Ttx::Lexical::Class::Type::CallOp:
  case ::Ttx::Lexical::Class::Type::AddressOp:
  case ::Ttx::Lexical::Class::Type::SwizzleOp:
  case ::Ttx::Lexical::Class::Type::SliceOp:
  case ::Ttx::Lexical::Class::Type::PackingOp:
  case ::Ttx::Lexical::Class::Type::NotOp:
  case ::Ttx::Lexical::Class::Type::RangeOp:
  case ::Ttx::Lexical::Class::Type::AndOp:
  case ::Ttx::Lexical::Class::Type::OrOp:
  case ::Ttx::Lexical::Class::Type::Assign:
  case ::Ttx::Lexical::Class::Type::AddAssign:
  case ::Ttx::Lexical::Class::Type::SubAssign:
  case ::Ttx::Lexical::Class::Type::ScopeStart:
  case ::Ttx::Lexical::Class::Type::ScopeEnd:
  case ::Ttx::Lexical::Class::Type::PackingStart:
  case ::Ttx::Lexical::Class::Type::PackingEnd:
  case ::Ttx::Lexical::Class::Type::IndexStart:
  case ::Ttx::Lexical::Class::Type::IndexEnd:
  case ::Ttx::Lexical::Class::Type::Define:
  case ::Ttx::Lexical::Class::Type::TypeAccessOp:
  case ::Ttx::Lexical::Class::Type::EndStatement:
  case ::Ttx::Lexical::Class::Type::Discard:
    return SemanticOperator;

  case ::Ttx::Lexical::Class::Type::Unknown:
  case ::Ttx::Lexical::Class::Type::EndOfStream:
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

  ::Ttx::Lexical::Tokenizer tokenizer(
      arena, source, "lsp-buffer.ttx"_view, False);
  View::Vector<::Ttx::Lexical::Token> tokens = tokenizer.get_tokens();
  ::Ttx::Lexical::Cursor cursor(tokenizer);
  void* parsed_source = ::Ttx::Dialect::Source::Source::parse(cursor);
  auto* source_info =
      static_cast<::Ttx::Dialect::Source::Source*>(parsed_source);
  View::Bytes dialect =
      source_info == nullptr ? View::Bytes() : source_info->get_dialect().get_name();
  Bits_32 previous_line = 0;
  Bits_32 previous_column = 0;
  Bool emitted = False;

  for (Count i = 0; i < tokens.get_size(); i++) {
    ::Ttx::Lexical::Token token = tokens[i];

    if (dialect == "Shader"_view &&
        should_filter_shader_keyword(token.get_class())) {
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
