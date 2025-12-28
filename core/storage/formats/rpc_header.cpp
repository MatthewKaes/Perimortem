// Perimortem Engine
// Copyright © Matt Kaes

#include "storage/formats/rpc_header.hpp"

using namespace Perimortem::Storage::Json;
using namespace Perimortem::Memory;

RpcHeader::RpcHeader(Arena& arena, ByteView contents) : arena(arena) {
  uint32_t position = 0;

  parse_rpc(contents, position);
  parse_id(contents, position);
  parse_method(contents, position);
  parse_params(contents, position);
}

auto RpcHeader::parse_rpc(const Perimortem::Memory::ByteView& contents,
                          uint32_t& position) -> void {
  const ByteView rpc_block("\"jsonrpc\"");

  while (position < contents.get_size() && contents[position] != '"') {
    position++;
  }

  if (!contents.block_compare(rpc_block, position)) {
    return;
  }

  // Scan to closing quote.
  position += rpc_block.get_size() + 1;
  while (contents[position] != '"') {
    position++;
  }
  auto start = ++position;

  while (contents[position] != '"') {
    position++;
  }
  auto end = position++;

  json_rpc = contents.slice(start, end - start);
}

auto RpcHeader::parse_id(const Perimortem::Memory::ByteView& contents,
                         uint32_t& position) -> void {
  const ByteView id_block("\"id\"");

  while (position < contents.get_size() && contents[position] != '"') {
    position++;
  }

  // Parsing for Id Block
  if (!contents.block_compare(id_block, position)) {
    return;
  }

  // Scan up to valid numbers.
  auto start = ++position;
  while (position < contents.get_size() &&
         (contents[position] < '0' || contents[position] > '9')) {
    position++;
  }
  auto end = position++;

  while (position < contents.get_size() && contents[position] >= '0' &&
         contents[position] <= '9') {
    position++;
  }

  id = contents.slice(start, end - start);
}

auto RpcHeader::parse_method(const Perimortem::Memory::ByteView& contents,
                             uint32_t& position) -> void {
  const ByteView method_block("\"method\"");

  while (position < contents.get_size() && contents[position] != '"') {
    position++;
  }

  if (!contents.block_compare(method_block, position)) {
    return;
  }

  // Scan to closing quote.
  position += method_block.get_size() + 1;
  while (contents[position] != '"') {
    position++;
  }
  auto start = ++position;

  while (contents[position] != '"') {
    position++;
  }
  auto end = position++;

  method = contents.slice(start, end - start);
}
auto RpcHeader::parse_params(const Perimortem::Memory::ByteView& contents,
                             uint32_t& position) -> void {
  const ByteView params_block("\"params\"");

  while (position < contents.get_size() && contents[position] != '"') {
    position++;
  }

  if (!contents.block_compare(params_block, position)) {
    return;
  }

  // Scan twice to get to the value.
  position += params_block.get_size() + 1;
  while (contents[position] != '{') {
    position++;
  }
  params_offset = position++;
}
