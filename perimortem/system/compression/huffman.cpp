// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/compression/huffman.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/math.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;

auto Compression::HuffmanTable::compute_lengths(
    View::Vector<Bits_32> frequencies,
    Access::Vector<Bits_8> lengths) -> void {
  const Count symbol_count = frequencies.get_size();
  struct Node {
    Bits_32 frequency;
    Bits_16 parent;
    Bits_16 symbol;
  };

  constexpr Bits_16 null_node = 0xFFFF;
  constexpr Count node_max = Compression::HuffmanTable::max_symbol_count * 2;

  Static::Vector<Node, node_max> nodes;
  Count node_count = 0;
  Count leaf_count = 0;

  for (Count s = 0; s < symbol_count; s++) {
    lengths[s] = 0;
    if (frequencies[s] > 0) {
      nodes[node_count++] = {frequencies[s], null_node, Bits_16(s)};
      leaf_count++;
    }
  }

  if (leaf_count == 0) {
    return;
  } else if (leaf_count == 1) {
    lengths[nodes[0].symbol] = 1;
    return;
  }

  for (Count i = 1; i < leaf_count; i++) {
    Node key = nodes[i];
    Count j = i;
    while (j > 0 && nodes[j - 1].frequency > key.frequency) {
      nodes[j] = nodes[j - 1];
      j--;
    }

    nodes[j] = key;
  }

  Count next_leaf = 0;
  Count next_node = leaf_count;
  while ((leaf_count - next_leaf) + (node_count - next_node) > 1) {
    auto take_min = [&]() -> Count {
      if (next_leaf >= leaf_count) {
        return next_node++;
      }

      if (next_node >= node_count) {
        return next_leaf++;
      }

      if (nodes[next_leaf].frequency <= nodes[next_node].frequency) {
        return next_leaf++;
      }
      return next_node++;
    };

    Count left_child = take_min();
    Count right_child = take_min();
    Count new_node = node_count++;
    nodes[new_node] = {
      nodes[left_child].frequency + nodes[right_child].frequency, null_node,
      null_node};
    nodes[left_child].parent = Bits_16(new_node);
    nodes[right_child].parent = Bits_16(new_node);
  }

  // Phase 1: compute depths, clamp to max_code_bits, and build length counts.
  Static::Vector<Count, max_code_bits + 2> codes_per_length;
  for (Count i = 0; i <= max_code_bits + 1; i++) {
    codes_per_length[i] = 0;
  }

  for (Count i = 0; i < leaf_count; i++) {
    Count depth = 0;
    Count current = i;
    while (nodes[current].parent != null_node) {
      current = nodes[current].parent;
      depth++;
    }

    // Cap the depth if it's outside the range of max bits.
    if (depth > max_code_bits) {
      depth = max_code_bits;
    }

    codes_per_length[depth]++;
    lengths[nodes[i].symbol] = Bits_8(depth);
  }

  // Phase 2: restore Kraft equality if clamping over-committed the prefix code.
  // Clamping a symbol from depth d > max_code_bits to max_code_bits can make
  // the Kraft sum exceed 2 ^ max_code_bits which would produce an ambiguous
  // prefix code that stricter decoders reject.
  //
  // We compute the actual integer Kraft excess directly and reduce it by
  // exactly that many iterations. Each iteration removes one code from the
  // deepest available sub-max level, replaces it with two codes at the next
  // level, and removes one code at max_code_bits.
  Count kraft_sum = 0;
  for (Count j = 1; j <= max_code_bits; j++) {
    kraft_sum += codes_per_length[j] * (Count(1) << (max_code_bits - j));
  }

  constexpr auto target_kraft = Count(1) << max_code_bits;
  Count overflow = kraft_sum > target_kraft ? kraft_sum - target_kraft : 0;
  if (overflow > 0) {
    while (overflow > 0) {
      Count bits = max_code_bits;
      while (codes_per_length[bits - 1] == 0) {
        bits--;
      }
      codes_per_length[bits - 1]--;
      codes_per_length[bits] += 2;
      codes_per_length[max_code_bits]--;
      overflow--;
    }

    // Reassign lengths from the adjusted codes_per_length. Nodes are sorted in
    // frequency ascending order, so walking backwards assigns the shortest
    // codes to the most frequent symbols.
    Count assign_idx = leaf_count;
    for (Count bits = 1; bits <= max_code_bits; bits++) {
      for (Count n = 0; n < codes_per_length[bits]; n++) {
        lengths[nodes[--assign_idx].symbol] = Bits_8(bits);
      }
    }
  }
}
