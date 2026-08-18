// Perimortem Engine
// Copyright © Matt Kaes

#include "puffer/lsp/methods.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/base64.hpp"
#include "perimortem/serialization/json/blueprint.hpp"
#include "perimortem/serialization/json/node.hpp"

#include "puffer/lsp/documents.hpp"
#include "puffer/lsp/hover.hpp"
#include "puffer/lsp/rpc/executor.hpp"
#include "puffer/lsp/semantic_tokens.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Puffer;

static auto format_source(Allocator::Arena&, View::Bytes source, View::Bytes)
    -> Dynamic::Bytes {
  // The old formatter depended on the deleted Syntax tree. Until formatting
  // can consume a real tree owned by the dialect again, the LSP format request
  // keeps
  // editor behavior stable by returning the exact authored source.
  return Dynamic::Bytes(source);
}

static auto report_document(
    const Lsp::Rpc::Message& message,
    View::Bytes source) -> Lsp::Rpc::Response {
  auto& arena = message.get_arena();
  View::Bytes encoded = Base64::encode(arena, source);
  return message.report_result(
      Json::Blueprint{
        {
          {"document"_view, encoded},
        }}.construct(arena));
}

static auto publish_diagnostics(
    Lsp::Documents& documents,
    const Lsp::Rpc::Message& message,
    View::Bytes uri) -> Lsp::Rpc::Response {
  Allocator::Arena& arena = message.get_arena();
  Managed::Vector<Json::Node> diagnostics(arena);
  auto errors = documents.get_errors(uri);
  if (errors) {
    for (Count index = 0; index < errors->get_size(); index++) {
      Ttx::Lexical::Anchor anchor = errors->get_anchor(index);
      Ttx::Lexical::Token token = anchor.get_token();
      Ttx::Lexical::Span span = anchor.get_span();
      if (!token && span) {
        token = span.get_start();
      }
      Count line = token && token.get_line() != 0 ? token.get_line() - 1 : 0;
      Count column =
          token && token.get_column() != 0 ? token.get_column() - 1 : 0;
      Count width = token ? token.get_size() : 0;
      diagnostics.insert(
          Json::Blueprint{
            {
              {"range"_view,
               {
                 {"start"_view,
                  {
                    {"line"_view, line},
                    {"character"_view, column},
                  }},
                 {"end"_view,
                  {
                    {"line"_view, line},
                    {"character"_view, column + width},
                  }},
               }},
              {"severity"_view, Signed_64(1)},
              {"source"_view, "ttx"_view},
              {"message"_view, errors->get_message(index)},
            }}.construct(arena));
    }
  }

  Json::Node published(diagnostics.get_view());
  return Json::Blueprint{
    {
      {"jsonrpc"_view, "2.0"_view},
      {"method"_view, "textDocument/publishDiagnostics"_view},
      {"params"_view,
       {
         {"uri"_view, uri},
         {"diagnostics"_view, published},
       }},
    }}.construct(arena);
}

auto Puffer::Lsp::initialize(Documents&, const Rpc::Message& message)
    -> Rpc::Response {
  auto& arena = message.get_arena();
  return message.report_result(
      Json::Blueprint{
        {
          {"serverInfo"_view,
           {
             {"name"_view, "Tetrodotoxin Language Server"_view},
             {"version"_view, "1.0"_view},
           }},
          {"capabilities"_view,
           {
             {"positionEncoding"_view, "utf-16"_view},
             {"textDocumentSync"_view,
              {
                {"openClose"_view, True},
                {"change"_view, Signed_64(1)},
              }},
             {"hoverProvider"_view, True},
             {"semanticTokensProvider"_view,
              {
                {"legend"_view, Lsp::semantic_legend(arena)},
                {"full"_view, True},
              }},
           }},
        }}.construct(arena));
}

auto Puffer::Lsp::format(Documents&, const Rpc::Message& message)
    -> Rpc::Response {
  auto& arena = message.get_arena();
  const auto& args = message.get_params();
  if (args.is_null()) {
    return message.report_error("Failed to parse format request."_view);
  }

  const auto source_b64 =
      args["source"_view].decode_string(message.get_arena());
  if (source_b64.is_empty()) {
    return message.report_error(
        "Requested format but no `source` was provided"_view);
  }

  const auto name = args["name"_view].decode_string(message.get_arena());
  if (name.is_empty()) {
    return message.report_error(
        "Requested format but no `name` was provided"_view);
  }

  Dynamic::Bytes decoded_source = Base64::decode(source_b64);
  Dynamic::Bytes formatted =
      format_source(arena, decoded_source.get_view(), name);
  return report_document(message, formatted.get_view());
}

auto Puffer::Lsp::did_open(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response {
  const auto uri =
      message.get_params()["textDocument"_view]["uri"_view].decode_string(
          message.get_arena());
  const Json::Node text =
      message.get_params()["textDocument"_view]["text"_view];
  View::Bytes decoded = text.decode_string(message.get_arena());
  documents.upsert(uri, decoded);

  Diagnostics::Log::Message<512> log_message(Diagnostics::Log::Level::Info);
  log_message << "File opened: "_view << uri;
  return publish_diagnostics(documents, message, uri);
}

auto Puffer::Lsp::did_change(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response {
  const auto uri =
      message.get_params()["textDocument"_view]["uri"_view].decode_string(
          message.get_arena());
  Json::Array changes = message.get_params()["contentChanges"_view].get_array();
  if (!changes.is_empty()) {
    const Json::Node text = changes[changes.get_size() - 1]["text"_view];
    View::Bytes decoded = text.decode_string(message.get_arena());
    documents.upsert(uri, decoded);
  }

  return publish_diagnostics(documents, message, uri);
}

auto Puffer::Lsp::did_close(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response {
  const auto uri =
      message.get_params()["textDocument"_view]["uri"_view].decode_string(
          message.get_arena());
  documents.erase(uri);

  Diagnostics::Log::Message<512> log_message(Diagnostics::Log::Level::Info);
  log_message << "File closed: "_view << uri;
  return publish_diagnostics(documents, message, uri);
}

auto Puffer::Lsp::semantic_tokens(
    Documents& documents,
    const Rpc::Message& message) -> Rpc::Response {
  const auto uri =
      message.get_params()["textDocument"_view]["uri"_view].decode_string(
          message.get_arena());
  Dynamic::Bytes source = documents.get_text(uri);
  return message.report_result(
      Lsp::semantic_tokens_for(message.get_arena(), source.get_view()));
}

auto Puffer::Lsp::hover(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response {
  const Json::Node params = message.get_params();
  const auto uri = params["textDocument"_view]["uri"_view].decode_string(
      message.get_arena());
  const Json::Node line = params["position"_view]["line"_view];
  const Json::Node character = params["position"_view]["character"_view];
  if (uri.is_empty() || !line.is_number() || !character.is_number() ||
      line.get_number() < 0 || character.get_number() < 0) {
    return message.report_result(Json::Node());
  }

  auto semantic = documents.find_semantic(
      uri, Count(line.get_number()), Count(character.get_number()));
  if (!semantic) {
    return message.report_result(Json::Node());
  }
  return message.report_result(semantic_hover(message.get_arena(), *semantic));
}
