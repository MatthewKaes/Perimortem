// Perimortem Engine
// Copyright © Matt Kaes

#include "storage/formats/rpc_header.hpp"

using namespace Perimortem::Storage::Json;
using namespace Perimortem::Memory;

RpcHeader::RpcHeader(const ManagedString& contents) {
  uint32_t position = 0;

  parse_rpc(contents, position);
  parse_id(contents, position);
  parse_method(contents, position);
  parse_params(contents, position);
}

auto RpcHeader::parse_rpc(const Perimortem::Memory::ManagedString& contents,
                          uint32_t& position) -> void {
  const ManagedString rpc_block("jsonrpc\"");

  while (contents[position] != 'j')
    position++;

  if (!contents.block_compare(rpc_block, position)) {
    return;
  }

  // Scan to closing quote.
  position += rpc_block.get_size() + 1;
  while (contents[position] != '"')
    position++;
  auto start = ++position;

  while (contents[position] != '"')
    position++;
  auto end = position++;

  // Mark RPC as found.
  json_rpc = contents.slice(start, end - start);
}

auto RpcHeader::parse_id(const Perimortem::Memory::ManagedString& contents,
                         uint32_t& position) -> void {
  const ManagedString id_block("id\"");

  while (contents[position] != 'i')
    position++;

  // Parsing for Id Block
  if (!contents.block_compare(id_block, position)) {
    return;
  }

  // Scan up to valid numbers.
  while (position < contents.get_size() &&
         (contents[position] < '0' || contents[position] > '9')) {
    position++;
  }

  id = 0;
  while (position < contents.get_size() && contents[position] >= '0' &&
         contents[position] <= '9') {
    id *= 10;
    id += contents[position] - '0';
    position++;
  }
}

auto RpcHeader::parse_method(const Perimortem::Memory::ManagedString& contents,
                             uint32_t& position) -> void {
  const ManagedString method_block("method\"");

  while (contents[position] != 'm')
    position++;

  if (!contents.block_compare(method_block, position)) {
    return;
  }

  // Scan to closing quote.
  position += method_block.get_size() + 1;
  while (contents[position] != '"')
    position++;
  auto start = ++position;

  while (contents[position] != '"')
    position++;
  auto end = position++;

  // Mark RPC as found.
  method = contents.slice(start, end - start);
}
auto RpcHeader::parse_params(const Perimortem::Memory::ManagedString& contents,
                             uint32_t& position) -> void {
  const ManagedString params_block("params\"");

  while (contents[position] != 'p')
    position++;

  if (!contents.block_compare(params_block, position)) {
    return;
  }

  // Scan twice to get to the value.
  position += params_block.get_size() + 1;
  while (contents[position] != '{')
    position++;
  params_offset = position++;
}
