// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/lsp/methods.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/serialization/base64.hpp"
#include "perimortem/serialization/escaped_text.hpp"
#include "perimortem/serialization/json/node.hpp"

#include "tetrodotoxin/puffer/lsp/documents.hpp"
#include "tetrodotoxin/puffer/lsp/rpc/executor.hpp"
#include "tetrodotoxin/puffer/lsp/semantic_tokens.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin::Puffer;

static auto format_source(Allocator::Arena&, View::Bytes source, View::Bytes)
    -> Dynamic::Bytes {
  // The old formatter depended on the deleted Syntax tree. Until formatting
  // can consume a real dialect-owned tree again, the LSP format request keeps
  // editor behavior stable by returning the exact authored source.
  return Dynamic::Bytes(source);
}

static auto report_document(
    const Lsp::Rpc::Message& message,
    View::Bytes source) -> Lsp::Rpc::Response {
  auto& arena = message.get_arena();
  View::Bytes encoded = Base64::encode(arena, source);
  return message.report_result(
      Json::Node::construct(
          arena, Json::Blueprint{{
                   {"document"_view, encoded},
                 }}));
}

auto Tetrodotoxin::Puffer::Lsp::initialize(
    Documents&,
    const Rpc::Message& message) -> Rpc::Response {
  auto& arena = message.get_arena();
  return message.report_result(
      Json::Node::construct(
          arena, Json::Blueprint{{
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
                      {"semanticTokensProvider"_view,
                       {
                         {"legend"_view, Lsp::semantic_legend(arena)},
                         {"full"_view, True},
                       }},
                    }},
                 }}));
}

auto Tetrodotoxin::Puffer::Lsp::format(Documents&, const Rpc::Message& message)
    -> Rpc::Response {
  auto& arena = message.get_arena();
  const auto& args = message.get_params();
  if (args.is_null()) {
    return message.report_error("Failed to parse format request."_view);
  }

  const auto source_b64 = args["source"_view].get_string();
  if (source_b64.is_empty()) {
    return message.report_error(
        "Requested format but no `source` was provided"_view);
  }

  const auto name = args["name"_view].get_string();
  if (name.is_empty()) {
    return message.report_error(
        "Requested format but no `name` was provided"_view);
  }

  Dynamic::Bytes decoded_source = Base64::decode(source_b64);
  Dynamic::Bytes formatted =
      format_source(arena, decoded_source.get_view(), name);
  return report_document(message, formatted.get_view());
}

auto Tetrodotoxin::Puffer::Lsp::did_open(
    Documents& documents,
    const Rpc::Message& message) -> Rpc::Response {
  auto& arena = message.get_arena();
  const auto uri =
      message.get_params()["textDocument"_view]["uri"_view].get_string();
  const auto text =
      message.get_params()["textDocument"_view]["text"_view].get_string();
  documents.upsert(uri, EscapedText::decode(arena, text));

  Diagnostics::Log::Message<512> log_message(Diagnostics::Log::Level::Info);
  log_message << "File opened: "_view << uri;
  return Json::Node();
}

auto Tetrodotoxin::Puffer::Lsp::did_change(
    Documents& documents,
    const Rpc::Message& message) -> Rpc::Response {
  auto& arena = message.get_arena();
  const auto uri =
      message.get_params()["textDocument"_view]["uri"_view].get_string();
  Json::Array changes = message.get_params()["contentChanges"_view].get_array();
  if (!changes.is_empty()) {
    const auto text = changes[changes.get_size() - 1]["text"_view].get_string();
    documents.upsert(uri, EscapedText::decode(arena, text));
  }

  return Json::Node();
}

auto Tetrodotoxin::Puffer::Lsp::did_close(
    Documents& documents,
    const Rpc::Message& message) -> Rpc::Response {
  const auto uri =
      message.get_params()["textDocument"_view]["uri"_view].get_string();
  documents.erase(uri);

  Diagnostics::Log::Message<512> log_message(Diagnostics::Log::Level::Info);
  log_message << "File closed: "_view << uri;
  return Json::Node();
}

auto Tetrodotoxin::Puffer::Lsp::semantic_tokens(
    Documents& documents,
    const Rpc::Message& message) -> Rpc::Response {
  const auto uri =
      message.get_params()["textDocument"_view]["uri"_view].get_string();
  Dynamic::Bytes source = documents.get_text(uri);
  return message.report_result(
      Lsp::semantic_tokens_for(message.get_arena(), source.get_view()));
}
