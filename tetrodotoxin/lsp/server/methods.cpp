// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/lsp/server/methods.hpp"

#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/base64.hpp"
#include "perimortem/serialization/escaped_text.hpp"
#include "perimortem/serialization/json/node.hpp"

#include "tetrodotoxin/format/source.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "tetrodotoxin/lsp/server/documents.hpp"
#include "tetrodotoxin/lsp/server/rpc/executor.hpp"
#include "tetrodotoxin/lsp/server/semantic_tokens.hpp"
#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/ttx.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin::Lsp;

static auto has_only_formatter_recoverable_errors(
    const Tetrodotoxin::Syntax::Context& ctx) -> Bool {
  if (ctx.get_error_count() == 0) {
    return True;
  }

  View::Bytes errors = ctx.get_errors();
  return Algorithm::search(
             errors,
             "Expected Foreign declarations before dialect body"_view) !=
             Count(-1) ||
         Algorithm::search(
             errors,
             "Expected Foreign declarations before ordinary members"_view) !=
             Count(-1);
}

static auto format_source(
    Allocator::Arena& arena,
    View::Bytes source,
    View::Bytes name) -> Dynamic::Bytes {
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(source);

  if (tokenizer.is_empty()) {
    return Dynamic::Bytes(source);
  }

  Tetrodotoxin::Syntax::Context ctx(tokenizer, name);
  Tetrodotoxin::Syntax::Ttx pkg = Tetrodotoxin::Syntax::Ttx::parse(ctx);

  if (!pkg.is_valid() || !has_only_formatter_recoverable_errors(ctx)) {
    return Dynamic::Bytes(source);
  }

  return Tetrodotoxin::Format::format(pkg);
}

static auto report_document(
    const Server::Rpc::Request& request,
    View::Bytes source) -> Server::Rpc::Response {
  auto& arena = request.get_arena();
  View::Bytes encoded = Base64::encode(arena, source);

  Managed::Vector<Json::Member> result_obj(arena);
  result_obj.insert({"document"_view, Json::Node(encoded)});
  return request.report_result(Json::Node(result_obj.get_view()));
}

static auto register_initialize(Server::Rpc::Executor& executor) -> void {
  Diagnostics::Log::info("   -- initialize"_view);
  executor.register_method(
      "initialize"_view,
      [](Server::Rpc::Executor&,
         const Server::Rpc::Request& request) -> Server::Rpc::Response {
        auto& arena = request.get_arena();

        Managed::Vector<Json::Member> text_doc_sync(arena);
        text_doc_sync.insert({"openClose"_view, Json::Node(True)});
        text_doc_sync.insert({"change"_view, Json::Node(Signed_64(1))});

        Managed::Vector<Json::Member> semantic_tokens(arena);
        semantic_tokens.insert({"legend"_view, Server::semantic_legend(arena)});
        semantic_tokens.insert({"full"_view, Json::Node(True)});

        Managed::Vector<Json::Member> capabilities(arena);
        capabilities.insert(
            {"positionEncoding"_view, Json::Node("utf-16"_view)});
        capabilities.insert(
            {"textDocumentSync"_view, Json::Node(text_doc_sync.get_view())});
        capabilities.insert(
            {"semanticTokensProvider"_view,
             Json::Node(semantic_tokens.get_view())});

        Managed::Vector<Json::Member> server_info(arena);
        server_info.insert(
            {"name"_view, Json::Node("Tetrodotoxin Language Server"_view)});
        server_info.insert({"version"_view, Json::Node("1.0"_view)});

        Managed::Vector<Json::Member> result_obj(arena);
        result_obj.insert(
            {"serverInfo"_view, Json::Node(server_info.get_view())});
        result_obj.insert(
            {"capabilities"_view, Json::Node(capabilities.get_view())});

        return request.report_result(Json::Node(result_obj.get_view()));
      });
}

static auto register_format(Server::Rpc::Executor& executor) -> void {
  Diagnostics::Log::info("   -- format"_view);
  executor.register_method(
      "format"_view,
      [](Server::Rpc::Executor&,
         const Server::Rpc::Request& request) -> Server::Rpc::Response {
        auto& arena = request.get_arena();
        const auto& args = request.get_params();

        if (args.is_null()) {
          return request.report_error("Failed to parse format request."_view);
        }

        const auto source_b64 = args["source"_view].get_string();
        if (source_b64.is_empty()) {
          return request.report_error(
              "Requested format but no `source` was provided"_view);
        }

        const auto name = args["name"_view].get_string();
        if (name.is_empty()) {
          return request.report_error(
              "Requested format but no `name` was provided"_view);
        }

        Dynamic::Bytes decoded_source = Base64::decode(source_b64);
        Dynamic::Bytes formatted =
            format_source(arena, decoded_source.get_view(), name);
        return report_document(request, formatted.get_view());
      });
}

static auto register_document_state(Server::Rpc::Executor& executor) -> void {
  // File URI logs intentionally keep more room than normal status messages.
  // The URI is the useful part of the log, so truncating it usually removes the
  // context we were trying to keep.
  constexpr Count file_uri_log_capacity = 512;

  Diagnostics::Log::info("   -- textDocument/didOpen"_view);
  executor.register_method(
      "textDocument/didOpen"_view,
      [](Server::Rpc::Executor& executor,
         const Server::Rpc::Request& request) -> Server::Rpc::Response {
        auto& arena = request.get_arena();
        const auto uri =
            request.get_params()["textDocument"_view]["uri"_view].get_string();
        const auto text =
            request.get_params()["textDocument"_view]["text"_view].get_string();
        executor.get_documents().upsert(uri, EscapedText::decode(arena, text));

        Diagnostics::Log::Message<file_uri_log_capacity> log_message(
            Diagnostics::Log::Level::Info);
        log_message << "File opened: "_view << uri;
        return Json::Node();
      });

  Diagnostics::Log::info("   -- textDocument/didChange"_view);
  executor.register_method(
      "textDocument/didChange"_view,
      [](Server::Rpc::Executor& executor,
         const Server::Rpc::Request& request) -> Server::Rpc::Response {
        auto& arena = request.get_arena();
        const auto uri =
            request.get_params()["textDocument"_view]["uri"_view].get_string();
        Json::Array changes =
            request.get_params()["contentChanges"_view].get_array();

        if (!changes.is_empty()) {
          const auto text =
              changes[changes.get_size() - 1]["text"_view].get_string();
          executor.get_documents().upsert(
              uri, EscapedText::decode(arena, text));
        }

        return Json::Node();
      });

  Diagnostics::Log::info("   -- textDocument/didClose"_view);
  executor.register_method(
      "textDocument/didClose"_view,
      [](Server::Rpc::Executor& executor,
         const Server::Rpc::Request& request) -> Server::Rpc::Response {
        const auto uri =
            request.get_params()["textDocument"_view]["uri"_view].get_string();
        executor.get_documents().erase(uri);

        Diagnostics::Log::Message<file_uri_log_capacity> log_message(
            Diagnostics::Log::Level::Info);
        log_message << "File closed: "_view << uri;
        return Json::Node();
      });
}

static auto register_semantic_tokens(Server::Rpc::Executor& executor) -> void {
  Diagnostics::Log::info("   -- textDocument/semanticTokens/full"_view);
  executor.register_method(
      "textDocument/semanticTokens/full"_view,
      [](Server::Rpc::Executor& executor,
         const Server::Rpc::Request& request) -> Server::Rpc::Response {
        const auto uri =
            request.get_params()["textDocument"_view]["uri"_view].get_string();
        Dynamic::Bytes source = executor.get_documents().get_text(uri);
        return request.report_result(
            Server::semantic_tokens_for(
                request.get_arena(), source.get_view()));
      });
}

auto Tetrodotoxin::Lsp::Server::register_methods(Rpc::Executor& executor)
    -> void {
  register_initialize(executor);
  register_format(executor);
  register_document_state(executor);
  register_semantic_tokens(executor);
}
