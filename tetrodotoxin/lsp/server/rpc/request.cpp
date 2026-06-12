// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/lsp/server/rpc/request.hpp"

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

using namespace Tetrodotoxin::Lsp;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;

Server::Rpc::Request::Request(
    Perimortem::Memory::Allocator::Arena& arena,
    Perimortem::Core::View::Bytes source)
    : arena(arena), source(source) {
  parsed.parse(arena, source);
  call_params = parsed["params"_view];
}

auto Server::Rpc::Request::is_valid() const -> Bool {
  return !parsed.is_null() && !parsed["method"_view].get_string().empty() &&
         !parsed["jsonrpc"_view].get_string().empty();
}

auto Server::Rpc::Request::is_request() const -> Bool {
  return parsed.contains("id"_view);
}

auto Server::Rpc::Request::get_method() const -> Perimortem::Core::View::Bytes {
  return parsed["method"_view].get_string();
}

auto Server::Rpc::Request::get_params() const
    -> const Perimortem::Serialization::Json::Node& {
  return call_params;
}

auto Server::Rpc::Request::get_source() const -> Perimortem::Core::View::Bytes {
  return source;
}

auto Server::Rpc::Request::get_arena() const
    -> Perimortem::Memory::Allocator::Arena& {
  return arena;
}

auto Server::Rpc::Request::report_error(
    Perimortem::Core::View::Bytes error) const -> Server::Rpc::Response {
  Managed::Bytes sanitized(arena);
  sanitized.proxy(error);
  sanitized.convert('"', '`');

  const Json::Blueprint entries[] = {
    {"jsonrpc"_view, parsed["jsonrpc"_view].get_string()},
    {"id"_view, parsed["id"_view].get_number()},
    {"error"_view, sanitized.get_view()},
  };
  return Json::Node::construct(arena, entries);
}

auto Server::Rpc::Request::report_result(const Response& result) const
    -> Server::Rpc::Response {
  const Json::Blueprint entries[] = {
    {"jsonrpc"_view, parsed["jsonrpc"_view].get_string()},
    {"id"_view, parsed["id"_view].get_number()},
    {"result"_view, result},
  };
  return Json::Node::construct(arena, entries);
}
