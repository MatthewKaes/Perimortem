// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/compression/huffman.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/math.hpp"

using namespace Perimortem::Core;

namespace Perimortem::System::Compression {

auto HuffmanTable::compute_lengths(
    View::Vector<Bits_32> frequencies,
    Access::Vector<Bits_8> lengths) -> void {
  const Count symbol_count = frequencies.get_size();
  struct Node {
    Bits_32 freq;
    Bits_16 parent;
    Bits_16 symbol;
  };

  constexpr Bits_16 huff_null = 0xFFFF;
  constexpr Count node_max = HuffmanTable::max_symbol_count * 2;

  Static::Vector<Node, node_max> nodes;
  Count node_count = 0;
  Count leaf_count = 0;

  for (Count s = 0; s < symbol_count; s++) {
    lengths[s] = 0;
    if (frequencies[s] > 0) {
      nodes[node_count++] = {frequencies[s], huff_null, Bits_16(s)};
      leaf_count++;
    }
  }

  if (leaf_count == 0) {
    return;
  }
  if (leaf_count == 1) {
    lengths[nodes[0].symbol] = 1;
    return;
  }

  for (Count i = 1; i < leaf_count; i++) {
    Node key = nodes[i];
    Count j = i;
    while (j > 0 && nodes[j - 1].freq > key.freq) {
      nodes[j] = nodes[j - 1];
      j--;
    }
    nodes[j] = key;
  }

  Count q1 = 0;
  Count q2 = leaf_count;
  while ((leaf_count - q1) + (node_count - q2) > 1) {
    auto take_min = [&]() -> Count {
      Bool q1_empty = (q1 >= leaf_count);
      Bool q2_empty = (q2 >= node_count);
      if (q1_empty.value) {
        return q2++;
      }
      if (q2_empty.value) {
        return q1++;
      }
      if (nodes[q1].freq <= nodes[q2].freq) {
        return q1++;
      }
      return q2++;
    };
    Count a = take_min();
    Count b = take_min();
    Count new_node = node_count++;
    nodes[new_node] = {nodes[a].freq + nodes[b].freq, huff_null, huff_null};
    nodes[a].parent = Bits_16(new_node);
    nodes[b].parent = Bits_16(new_node);
  }

  for (Count i = 0; i < leaf_count; i++) {
    Count depth = 0;
    Count cur = i;
    while (nodes[cur].parent != huff_null) {
      cur = nodes[cur].parent;
      depth++;
    }
    lengths[nodes[i].symbol] =
        Bits_8(Math::min(depth, Count(HuffmanTable::max_code_bits)));
  }
}

}  // namespace Perimortem::System::Compression
