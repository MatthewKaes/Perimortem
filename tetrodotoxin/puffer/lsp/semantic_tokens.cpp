// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/lsp/semantic_tokens.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/puffer/isa/boot/virtual_machine.hpp"
#include "tetrodotoxin/puffer/toolchain.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin::Puffer;

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

static auto should_filter_shader_keyword(::Ttx::Lexical::Code code) -> Bool {
  switch (code.get_type()) {
  case ::Ttx::Lexical::Code::Type::If:
  case ::Ttx::Lexical::Code::Type::In:
  case ::Ttx::Lexical::Code::Type::For:
  case ::Ttx::Lexical::Code::Type::Break:
  case ::Ttx::Lexical::Code::Type::Continue:
  case ::Ttx::Lexical::Code::Type::Case:
  case ::Ttx::Lexical::Code::Type::Else:
  case ::Ttx::Lexical::Code::Type::Match:
  case ::Ttx::Lexical::Code::Type::While:
    return True;

  default:
    return False;
  }
}

static auto has_newline(View::Bytes text) -> Bool {
  return Algorithm::search(text, "\n"_view) != Count(-1);
}

static auto classify_semantic_token(::Ttx::Lexical::Code code) -> Signed_64 {
  switch (code.get_type()) {
  case ::Ttx::Lexical::Code::Type::Comment:
  case ::Ttx::Lexical::Code::Type::Disabled:
    return SemanticComment;

  case ::Ttx::Lexical::Code::Type::String:
  case ::Ttx::Lexical::Code::Type::Embedded:
  case ::Ttx::Lexical::Code::Type::PackedData:
    return SemanticString;

  case ::Ttx::Lexical::Code::Type::Numeric:
  case ::Ttx::Lexical::Code::Type::Hex:
  case ::Ttx::Lexical::Code::Type::Float:
  case ::Ttx::Lexical::Code::Type::Bytes:
    return SemanticNumber;

  case ::Ttx::Lexical::Code::Type::Attribute:
    return SemanticDecorator;

  case ::Ttx::Lexical::Code::Type::Type:
  case ::Ttx::Lexical::Code::Type::Alias:
    return SemanticType;

  case ::Ttx::Lexical::Code::Type::Addressable:
    return SemanticVariable;

  case ::Ttx::Lexical::Code::Type::AddOp:
  case ::Ttx::Lexical::Code::Type::SubOp:
  case ::Ttx::Lexical::Code::Type::DivOp:
  case ::Ttx::Lexical::Code::Type::MulOp:
  case ::Ttx::Lexical::Code::Type::ModOp:
  case ::Ttx::Lexical::Code::Type::LessOp:
  case ::Ttx::Lexical::Code::Type::GreaterOp:
  case ::Ttx::Lexical::Code::Type::LessEqOp:
  case ::Ttx::Lexical::Code::Type::GreaterEqOp:
  case ::Ttx::Lexical::Code::Type::CmpOp:
  case ::Ttx::Lexical::Code::Type::NotEqOp:
  case ::Ttx::Lexical::Code::Type::CallOp:
  case ::Ttx::Lexical::Code::Type::AddressOp:
  case ::Ttx::Lexical::Code::Type::SwizzleOp:
  case ::Ttx::Lexical::Code::Type::SliceOp:
  case ::Ttx::Lexical::Code::Type::PackingOp:
  case ::Ttx::Lexical::Code::Type::NotOp:
  case ::Ttx::Lexical::Code::Type::RangeOp:
  case ::Ttx::Lexical::Code::Type::AndOp:
  case ::Ttx::Lexical::Code::Type::OrOp:
  case ::Ttx::Lexical::Code::Type::Assign:
  case ::Ttx::Lexical::Code::Type::AddAssign:
  case ::Ttx::Lexical::Code::Type::SubAssign:
  case ::Ttx::Lexical::Code::Type::ScopeStart:
  case ::Ttx::Lexical::Code::Type::ScopeEnd:
  case ::Ttx::Lexical::Code::Type::PackingStart:
  case ::Ttx::Lexical::Code::Type::PackingEnd:
  case ::Ttx::Lexical::Code::Type::LayoutStart:
  case ::Ttx::Lexical::Code::Type::LayoutEnd:
  case ::Ttx::Lexical::Code::Type::Define:
  case ::Ttx::Lexical::Code::Type::TypeAccessOp:
  case ::Ttx::Lexical::Code::Type::EndStatement:
  case ::Ttx::Lexical::Code::Type::Discard:
    return SemanticOperator;

  case ::Ttx::Lexical::Code::Type::Unknown:
  case ::Ttx::Lexical::Code::Type::Terminal:
    return Signed_64(-1);

  default:
    return SemanticKeyword;
  }
}

auto Lsp::semantic_legend(Allocator::Arena& arena) -> Json::Node {
  return Json::Node::construct(
      arena, Json::Blueprint{{
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
             }});
}

auto Lsp::semantic_tokens_for(Allocator::Arena& arena, View::Bytes source)
    -> Json::Node {
  Managed::Vector<Json::Node> data(arena);
  if (source.is_empty()) {
    const Json::Node data_node(data.get_view());
    return Json::Node::construct(
        arena, Json::Blueprint{{
                 {"data"_view, data_node},
               }});
  }

  ::Ttx::Lexical::Tokenizer tokenizer(
      arena, source, "lsp-buffer.ttx"_view, False);
  View::Vector<::Ttx::Lexical::Token> tokens = tokenizer.get_tokens();
  ::Ttx::Lexical::Cursor cursor(tokenizer, arena);
  const auto isa_registry =
      ::Tetrodotoxin::Puffer::Toolchain::standard_registry();
  auto* boot = ::Tetrodotoxin::Puffer::Isa::Boot::VirtualMachine::evaluate(
      cursor, isa_registry);
  View::Bytes isa = boot == nullptr ? View::Bytes() : boot->get_isa();
  Unsigned_32 previous_line = 0;
  Unsigned_32 previous_column = 0;
  Bool emitted = False;
  for (Count i = 0; i < tokens.get_size(); i++) {
    ::Ttx::Lexical::Token token = tokens[i];
    if (isa == "Shader"_view &&
        should_filter_shader_keyword(token.get_code())) {
      continue;
    }

    View::Bytes text = token.get_text();
    if (text.is_empty() || has_newline(text)) {
      continue;
    }

    Signed_64 token_type = classify_semantic_token(token.get_code());
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
  return Json::Node::construct(
      arena, Json::Blueprint{{
               {"data"_view, data_node},
             }});
}
