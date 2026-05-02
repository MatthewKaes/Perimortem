// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/access/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/math/basic.hpp"

namespace Perimortem::Math {

template <typename type>
constexpr auto sort(Core::Access::Vector<type> access)
    -> Core::Access::Vector<type> {
  // TODO: Implement grail sort if it ever comes up, but for now just use a
  // heapsort or eventually a simple introsort.

  constexpr auto max_heapify = [](this auto&& self,
                                  Core::Access::Vector<type> access,
                                  Count index) constexpr -> void {
    auto data = access.get_data();
    auto left = 2 * index + 1;
    auto right = 2 * index + 2;

    // Leaf
    if (left >= access.get_size())
      return;

    if (right < access.get_size() && data[right] > data[left] &&
        data[right] > data[index]) {
      Core::Data::swap(data[right], data[index]);
      self(access, right);
      return;
    }

    // If left child is larger than root
    if (data[left] > data[index]) {
      Core::Data::swap(data[left], data[index]);
      self(access, left);
    }
  };

  // Heapify
  for (Count i = access.get_size() / 2 - 1; i > 0; i--)
    max_heapify(access, i);

  max_heapify(access, 0);

  // Pull items from front of heap and swap them to the back.
  auto data = access.get_data();
  for (Count i = 1; i < access.get_size(); i++) {
    Core::Data::swap(data[0], data[access.get_size() - i]);
    max_heapify(Core::Access::Vector<type>(data, access.get_size() - i), 0);
  }

  return access;
}

template <typename type, Count item_count>
consteval auto sort(const type (&data)[item_count])
    -> Core::Static::Vector<type, item_count> {
  // Heapify
  Core::Static::Vector<type, item_count> output;
  for (Count i = 0; i < item_count; i++) {
    output[i] = data[i];
  }

  sort<type>(output);

  return output;
}

}  // namespace Perimortem::Math