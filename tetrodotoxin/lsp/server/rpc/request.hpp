// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/serialization/json/node.hpp"

namespace Tetrodotoxin::Lsp::Server::Rpc {

using Response = Perimortem::Serialization::Json::Node;

class Request {
 public:
  Request(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes source);

  auto report_error(Perimortem::Core::View::Bytes error) const -> Response;
  auto report_result(const Response& result) const -> Response;

  auto is_valid() const -> Bool;
  auto is_request() const -> Bool;
  auto has_numeric_id() const -> Bool;

  auto get_method() const -> Perimortem::Core::View::Bytes;
  auto get_id() const -> Signed_64;
  auto get_params() const -> const Perimortem::Serialization::Json::Node&;
  auto get_source() const -> Perimortem::Core::View::Bytes;
  auto get_arena() const -> Perimortem::Memory::Allocator::Arena&;

 private:
  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Serialization::Json::Node parsed;
  Perimortem::Serialization::Json::Node call_params;
  Perimortem::Core::View::Bytes source;
};

}  // namespace Tetrodotoxin::Lsp::Server::Rpc
