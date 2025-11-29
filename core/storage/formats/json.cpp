// Perimortem Engine
// Copyright © Matt Kaes

#include "storage/formats/json.hpp"

#include <x86intrin.h>
#include <bit>

using namespace Perimortem::Memory;
using namespace Perimortem::Storage::Json;

auto Node::operator[](uint32_t index) const -> const Node* {
  if (std::holds_alternative<ManagedVector<Node*>>(value)) {
    const auto& vec = get<ManagedVector<Node*>>(value);
    if (vec.get_size() >= index) {
      return nullptr;
    }

    return vec.at(index);
  }

  return nullptr;
}

auto Node::operator[](const std::string_view& name) const -> const Node* {
  if (std::holds_alternative<ManagedLookup<Node>>(value)) {
    return get<ManagedLookup<Node>>(value).at(name);
  }

  return nullptr;
}

auto Node::contains(const std::string_view& name) const -> bool {
  if (std::holds_alternative<ManagedLookup<Node>>(value)) {
    return get<ManagedLookup<Node>>(value).contains(name);
  }

  return false;
}

auto Node::get_bool() const -> const bool* {
  if (std::holds_alternative<bool>(value)) {
    return &get<bool>(value);
  }

  return nullptr;
}

auto Node::get_int() const -> const long* {
  if (std::holds_alternative<long>(value)) {
    return &get<long>(value);
  }

  return nullptr;
}

auto Node::get_double() const -> const double* {
  if (std::holds_alternative<double>(value)) {
    return &get<double>(value);
  }

  return nullptr;
}

auto Node::get_string() const -> const Memory::ManagedString* {
  if (std::holds_alternative<ManagedString>(value)) {
    return &get<ManagedString>(value);
  }

  return nullptr;
}

namespace Perimortem::Storage::Json {
template <uint32_t channels, uint32_t index, uint32_t range>
auto optimized_or_merge(__m256i source[channels]) -> __m256i {
  if constexpr (range == 1) {
    return source[index];
  } else if constexpr (range == 2) {
    return _mm256_or_si256(source[index], source[index + 1]);
  } else {
    return _mm256_or_si256(
        optimized_or_merge<channels, index, (range / 2)>(source),
        optimized_or_merge<channels, index + (range / 2), range - (range / 2)>(
            source));
  }
}

auto parse_string(ManagedString source, uint32_t& position)
    -> ManagedString {
  uint32_t start = ++position;
  position = source.scan('"', position);

  return source.slice(start, position++ - start);
}

auto ignored_characters(char c) {
  return c == ',';
}

auto parse(Memory::Arena& arena, ManagedString source, uint32_t& position)
    -> Node* {
  if (position > source.get_size()) {
    return nullptr;
  }

  // Allocations are cheap and we throw away the entire stack if the parse is
  // bad so it profiled slightly faster to just pull out the construct to the
  // top level.
  Node* node = arena.construct<Node>();

  while (position < source.get_size()) {
    const auto start_char = source[position];
    switch (start_char) {
      // Object
      case '{': {
        ManagedLookup<Node> members(arena);
        position++;

        while (position < source.get_size() && source[position] != '}') {
          if (source[position] != '"') {
            position++;
            continue;
          }
          // position = fast_scan(source, position, '"');

          // Optimize to assume that most dictionary elements are less than 32
          // characters long.
          const auto start = ++position;
          position = source.fast_scan('"', position);
          auto name = ManagedString(source.slice(start, position++ - start));

          // Encountered an extra long name so try a full parse.
          if (name.get_size() == 32) {
            position -= 33;
            name = parse_string(source, position);
            if (name.empty()) {
              return nullptr;
            }
          }

          // auto name = parse_string(source, position);
          // if (name.empty()) {
          //   return nullptr;
          // }

          // :
          position++;

          auto child = parse(arena, source, position);
          if (!child) {
            return nullptr;
          }

          members.insert(name, child);
        }

        // If we are past the end then this is safe as we null out anyway.
        // If we are at a valid closing '}' then this consumes it.
        position++;
        node->set(members);
        return node;
      }

      // Array
      case '[': {
        ManagedVector<Node*> items(arena);
        position++;

        while (position < source.get_size() && source[position] != ']') {
          if (ignored_characters(source[position])) {
            position++;
          }

          auto child = parse(arena, source, position);
          if (!child) {
            return nullptr;
          }

          items.insert(child);
          node->set(items);
        }

        // If we are past the end then this is safe as we null out anyway.
        // If we are at a valid closing ']' then this consumes it.
        position++;
        return node;
      }

      // String
      case '"': {
        auto name = parse_string(source, position);

        node->set(name);
        return node;
      }

      // true
      case 't': {
        if (position + 3 >= source.get_size() ||
            std::memcmp(source.get_view().data() + position, "true", 4)) {
          return nullptr;
        }

        // Move past "true"
        position += 4;

        node->set(true);
        return node;
      }

      // false
      case 'f': {
        if (position + 4 >= source.get_size() ||
            std::memcmp(source.get_view().data() + position, "false", 5)) {
          return nullptr;
        }

        // Move past "false"
        position += 5;

        node->set(false);
        return node;
      }

      // Numbers
      case '-':
      case '0' ... '9': {
        bool negative = false;
        if (source[position] == '-') {
          negative = true;
          position++;
        }

        long value = 0;
        while (position < source.get_size() && source[position] >= '0' &&
               source[position] <= '9') {
          value *= 10;
          value += source[position] - '0';
          position++;
        }

        if (position < source.get_size() && source[position] != '.') {
          node->set(value * (negative ? -1 : 1));
          return node;
        }

        double float_value = value;
        double divisor = 1.0;
        while (position < source.get_size() && source[position] >= '0' &&
               source[position] <= '9') {
          float_value *= 10;
          divisor *= 10;
          float_value += source[position] - '0';
          position++;
        }

        if (position < source.get_size()) {
          node->set((float_value / divisor) * (negative ? -1 : 1));
          return node;
        }

        return nullptr;
      }

      case ',':
        position++;
        break;

      default:
        return nullptr;
    }
  }
  return nullptr;
}

}  // namespace Perimortem::Storage::Json
