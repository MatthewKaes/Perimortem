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

static auto should_filter_shader_keyword(::Ttx::Lexical::Class klass) -> Bool {
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

static auto classify_semantic_token(::Ttx::Lexical::Class klass) -> Signed_64 {
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
