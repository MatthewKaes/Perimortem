// Perimortem Engine
// Copyright © Matt Kaes

#include "puffer/lsp/methods.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/base64.hpp"
#include "perimortem/serialization/json/blueprint.hpp"
#include "perimortem/serialization/json/node.hpp"

#include "puffer/lsp/documents.hpp"
#include "puffer/lsp/rpc/executor.hpp"
#include "puffer/lsp/semantic_tokens.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Puffer;

static auto hex_value(Unsigned_8 value) -> Signed_32 {
  if (value >= '0' && value <= '9') {
    return value - '0';
  }

  if (value >= 'A' && value <= 'F') {
    return value - 'A' + 10;
  }

  if (value >= 'a' && value <= 'f') {
    return value - 'a' + 10;
  }

  return -1;
}

static auto read_hex_quad(
    View::Bytes source,
    Count position,
    Unsigned_32& value) -> Bool {
  if (position + 4 > source.get_size()) {
    return False;
  }

  value = 0;
  for (Count i = 0; i < 4; i++) {
    Signed_32 digit = hex_value(source[position + i]);
    if (digit < 0) {
      return False;
    }

    value = (value << 4) | Unsigned_32(digit);
  }

  return True;
}

static auto is_high_surrogate(Unsigned_32 value) -> Bool {
  return value >= 0xD800 && value <= 0xDBFF;
}

static auto is_low_surrogate(Unsigned_32 value) -> Bool {
  return value >= 0xDC00 && value <= 0xDFFF;
}

static auto append_utf8(Managed::Bytes& output, Unsigned_32 codepoint) -> void {
  if (codepoint <= 0x7F) {
    output.append(Unsigned_8(codepoint));
    return;
  }

  if (codepoint <= 0x7FF) {
    output.append(Unsigned_8(0xC0 | (codepoint >> 6)));
    output.append(Unsigned_8(0x80 | (codepoint & 0x3F)));
    return;
  }

  if (codepoint <= 0xFFFF) {
    output.append(Unsigned_8(0xE0 | (codepoint >> 12)));
    output.append(Unsigned_8(0x80 | ((codepoint >> 6) & 0x3F)));
    output.append(Unsigned_8(0x80 | (codepoint & 0x3F)));
    return;
  }

  if (codepoint <= 0x10FFFF) {
    output.append(Unsigned_8(0xF0 | (codepoint >> 18)));
    output.append(Unsigned_8(0x80 | ((codepoint >> 12) & 0x3F)));
    output.append(Unsigned_8(0x80 | ((codepoint >> 6) & 0x3F)));
    output.append(Unsigned_8(0x80 | (codepoint & 0x3F)));
  }
}

static auto read_unicode_escape(
    View::Bytes source,
    Count slash,
    Unsigned_32& codepoint,
    Count& consumed) -> Bool {
  if (slash + 6 > source.get_size() || source[slash] != '\\' ||
      source[slash + 1] != 'u') {
    return False;
  }

  Unsigned_32 first = 0;
  Bool first_valid = read_hex_quad(source, slash + 2, first);
  if (!first_valid) {
    return False;
  }

  if (!is_high_surrogate(first)) {
    if (is_low_surrogate(first)) {
      return False;
    }

    codepoint = first;
    consumed = 6;
    return True;
  }

  if (slash + 12 > source.get_size() || source[slash + 6] != '\\' ||
      source[slash + 7] != 'u') {
    return False;
  }

  Unsigned_32 second = 0;
  Bool second_valid = read_hex_quad(source, slash + 8, second);
  if (!second_valid || !is_low_surrogate(second)) {
    return False;
  }

  codepoint = 0x10000 + ((first - 0xD800) << 10) + (second - 0xDC00);
  consumed = 12;
  return True;
}

// Json::Node preserves escape spelling in string views. Document storage owns
// source bytes rather than JSON spelling, so the LSP boundary decodes that one
// transport representation before publishing the text to concurrent readers.
static auto decode_document_text(Allocator::Arena& arena, View::Bytes source)
    -> View::Bytes {
  Managed::Bytes decoded(arena);
  for (Count i = 0; i < source.get_size(); i++) {
    if (source[i] != '\\' || i + 1 >= source.get_size()) {
      decoded.append(source[i]);
      continue;
    }

    Count slash = i;
    i++;
    switch (source[i]) {
    case 'n':
      decoded.append('\n');
      break;
    case 'r':
      decoded.append('\r');
      break;
    case 't':
      decoded.append('\t');
      break;
    case '"':
      decoded.append('"');
      break;
    case '\\':
      decoded.append('\\');
      break;
    case '/':
      decoded.append('/');
      break;
    case 'b':
      decoded.append('\b');
      break;
    case 'f':
      decoded.append('\f');
      break;
    case 'u': {
      Unsigned_32 codepoint = 0;
      Count consumed = 0;
      Bool valid = read_unicode_escape(source, slash, codepoint, consumed);
      if (valid) {
        append_utf8(decoded, codepoint);
        i = slash + consumed - 1;
        break;
      }

      decoded.append('\\');
      decoded.append(source[i]);
      break;
    }
    default:
      decoded.append('\\');
      decoded.append(source[i]);
      break;
    }
  }

  return decoded.get_view();
}

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

auto Puffer::Lsp::did_open(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response {
  const auto uri =
      message.get_params()["textDocument"_view]["uri"_view].get_string();
  const auto text =
      message.get_params()["textDocument"_view]["text"_view].get_string();
  View::Bytes decoded = decode_document_text(message.get_arena(), text);
  documents.upsert(uri, decoded);

  Diagnostics::Log::Message<512> log_message(Diagnostics::Log::Level::Info);
  log_message << "File opened: "_view << uri;
  return Json::Node();
}

auto Puffer::Lsp::did_change(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response {
  const auto uri =
      message.get_params()["textDocument"_view]["uri"_view].get_string();
  Json::Array changes = message.get_params()["contentChanges"_view].get_array();
  if (!changes.is_empty()) {
    const auto text = changes[changes.get_size() - 1]["text"_view].get_string();
    View::Bytes decoded = decode_document_text(message.get_arena(), text);
    documents.upsert(uri, decoded);
  }

  return Json::Node();
}

auto Puffer::Lsp::did_close(Documents& documents, const Rpc::Message& message)
    -> Rpc::Response {
  const auto uri =
      message.get_params()["textDocument"_view]["uri"_view].get_string();
  documents.erase(uri);

  Diagnostics::Log::Message<512> log_message(Diagnostics::Log::Level::Info);
  log_message << "File closed: "_view << uri;
  return Json::Node();
}

auto Puffer::Lsp::semantic_tokens(
    Documents& documents,
    const Rpc::Message& message) -> Rpc::Response {
  const auto uri =
      message.get_params()["textDocument"_view]["uri"_view].get_string();
  Dynamic::Bytes source = documents.get_text(uri);
  return message.report_result(
      Lsp::semantic_tokens_for(message.get_arena(), source.get_view()));
}
