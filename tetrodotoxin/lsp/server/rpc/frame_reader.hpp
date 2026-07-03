// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Tetrodotoxin::Lsp::Server::Rpc {

class FrameReader {
 public:
  auto append(Perimortem::Core::View::Bytes bytes) -> void;
  auto next_message() -> Perimortem::Core::View::Bytes;
  auto consume_message() -> void;

 private:
  auto read_header() -> Bool;

  Perimortem::Memory::Dynamic::Bytes data_stream;
  Bits_64 data_bytes_to_read = 0;
  Bool header_found = False;
};

}  // namespace Tetrodotoxin::Lsp::Server::Rpc
