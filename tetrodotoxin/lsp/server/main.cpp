// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/base64.hpp"
#include "perimortem/serialization/json/node.hpp"

#include "tetrodotoxin/lexical/tokenizer.hpp"
#include "tetrodotoxin/lsp/server/rpc/executor.hpp"
#include "tetrodotoxin/syntax/formatter.hpp"
#include "tetrodotoxin/syntax/package.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Perimortem::Core;
using namespace Tetrodotoxin::Lsp;

auto main(int argc, char* argv[]) -> int {
  Diagnostics::Log::set_sink(Diagnostics::Log::stderr_sink);
  Diagnostics::Log::info("~~ TTX Lang Server ~~"_view);
  Diagnostics::Log::info("[Lsp API Version: 3.17]"_view);

  constexpr auto pipe_prefix = "--pipe="_view;
  View::Bytes pipe_name;
  for (Signed_32 i = 1; i < argc; i++) {
    auto arg = NullTerminated::to_view(argv[i]);
    if (arg.slice(0, pipe_prefix.get_size()) == pipe_prefix) {
      pipe_name = arg.slice(pipe_prefix.get_size());
    }
  }

  if (pipe_name.get_size() == 0) {
    Diagnostics::Log::error(
        "No `--pipe=` provided, closing language server."_view);
    return -1;
  }

  Diagnostics::Log::info("Creating RPC Server using pipe..."_view);
  Diagnostics::Log::info("   -- initialize"_view);
  Server::Rpc::Executor::register_method(
      "initialize"_view,
      [](const Server::Rpc::Request& request) -> Server::Rpc::Response {
        auto& arena = request.get_arena();

        Managed::Vector<Json::Member> text_doc_sync(arena);
        text_doc_sync.insert({"openClose"_view, Json::Node(True)});
        text_doc_sync.insert({"change"_view, Json::Node(Signed_64(1))});

        Managed::Vector<Json::Member> capabilities(arena);
        capabilities.insert(
            {"positionEncoding"_view, Json::Node("utf-16"_view)});
        capabilities.insert(
            {"textDocumentSync"_view, Json::Node(text_doc_sync.get_view())});

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

  Diagnostics::Log::info("   -- format"_view);
  Server::Rpc::Executor::register_method(
      "format"_view,
      [](const Server::Rpc::Request& request) -> Server::Rpc::Response {
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

        const auto name_string = args["name"_view].get_string();
        if (name_string.is_empty()) {
          return request.report_error(
              "Requested format but no `name` was provided"_view);
        }

        // Decode source, tokenize, parse, format, encode result.
        // decoded_source and encoded_result use Bibliotheca; keep them alive
        // until after report_result writes the response.
        Dynamic::Bytes decoded_source = Base64::decode(source_b64);

        Tetrodotoxin::Lexical::Tokenizer tokenizer(arena);
        tokenizer.parse(decoded_source.get_view());

        if (tokenizer.is_empty()) {
          View::Bytes encoded =
              Base64::encode(arena, decoded_source.get_view());
          Managed::Vector<Json::Member> result_obj(arena);
          result_obj.insert({"document"_view, Json::Node(encoded)});
          return request.report_result(Json::Node(result_obj.get_view()));
        }

        Tetrodotoxin::Syntax::Package pkg =
            Tetrodotoxin::Syntax::Package::parse(tokenizer, name_string);

        if (pkg.get_error_count() > 0 || !pkg.is_valid()) {
          View::Bytes encoded =
              Base64::encode(arena, decoded_source.get_view());
          Managed::Vector<Json::Member> result_obj(arena);
          result_obj.insert({"document"_view, Json::Node(encoded)});
          return request.report_result(Json::Node(result_obj.get_view()));
        }

        Tetrodotoxin::Syntax::Formatter formatter;
        pkg.format(formatter);
        View::Bytes encoded =
            Base64::encode(arena, formatter.get_output().get_view());

        Managed::Vector<Json::Member> result_obj(arena);
        result_obj.insert({"document"_view, Json::Node(encoded)});
        return request.report_result(Json::Node(result_obj.get_view()));
      });

  Diagnostics::Log::info("   -- textDocument/didOpen"_view);
  Server::Rpc::Executor::register_method(
      "textDocument/didOpen"_view,
      [](const Server::Rpc::Request& request) -> Server::Rpc::Response {
        const auto uri =
            request.get_params()["textDocument"_view]["uri"_view].get_string();
        Static::Bytes<512> buffer;
        Writer::Textual msg(buffer);
        msg << "File opened: "_view << uri;
        Diagnostics::Log::info(msg);
        return Json::Node();
      });

  Diagnostics::Log::info("   -- textDocument/didClose"_view);
  Server::Rpc::Executor::register_method(
      "textDocument/didClose"_view,
      [](const Server::Rpc::Request& request) -> Server::Rpc::Response {
        const auto uri =
            request.get_params()["textDocument"_view]["uri"_view].get_string();
        Static::Bytes<512> buffer;
        Writer::Textual msg(buffer);
        msg << "File closed: "_view << uri;
        Diagnostics::Log::info(msg);
        return Json::Node();
      });

  Diagnostics::Log::info("Starting RPC..."_view);
  Server::Rpc::Executor::execute(pipe_name);

  return 0;
}
