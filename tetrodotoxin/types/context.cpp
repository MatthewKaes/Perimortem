// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "types/context.hpp"

#include "concepts/singleton.hpp"

using namespace Tetrodotoxin::Language::Parser::Types;
using namespace Perimortem::Concepts;

class ContextAllocator
    : public Perimortem::Concepts::Singleton<ContextAllocator> {
  static constexpr Context::size_type starting_size = 3;
  static constexpr uint32_t radix_range =
      sizeof(Context::size_type) * 8 - (starting_size + 1);

  struct BufferInterface {
    virtual auto request_block() -> int* = 0;
    virtual auto get_size() const -> Context::size_type = 0;
    virtual auto get_capacity() const -> Context::size_type = 0;
  };

  template <Context::size_type block_range>
  struct StaticBuffer : public BufferInterface {
    auto request_block() -> int*;
    auto get_size() const -> size_type;
    auto get_capacity() const -> size_type { return block_range; }
    const Abstract* data;

    // Chain of related available buffers.
    StaticBuffer<block_range>* previous;
    Context::size_type size;
  };

  template <Context::size_type block_value>
  auto pop_or_new() -> BufferInterface* {
    if (previous[block_value] == nullptr)
      return new StaticBuffer<block_value>();

    BufferInterface*
  }

  std::array<BufferInterface*, radix_range> page_buckets;

  auto exchange(BufferInterface* buffer) -> BufferInterface* {
    if (buffer == nullptr) {
      return new StaticBuffer<starting_size>();
    }
  }
  auto release(Buffer* buffer) -> void;
};
