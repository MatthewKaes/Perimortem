// Perimortem Engine
// Copyright © Matt Kaes

#include "storage/formats/lazy_json.hpp"

using namespace Perimortem::Storage::Json;

auto LazyNode::get_bool() const -> bool {
  return !sub_section.empty() && sub_section[0] == 't';
}

auto LazyNode::get_int() const -> long {
  if (sub_section.empty()) {
    return -1;
  }

  bool negative = false;
  uint32_t position = 0;
  if (sub_section[position] == '-') {
    negative = true;
    position++;
  }

  long value = 0;
  while (position < sub_section.get_size() && sub_section[position] >= '0' &&
         sub_section[position] <= '9') {
    value *= 10;
    value += sub_section[position] - '0';
    position++;
  }

  return value * (negative ? -1 : 1);
}

auto LazyNode::get_double() const -> double {
  if (sub_section.empty()) {
    return -1;
  }

  bool negative = false;
  uint32_t position = 0;
  if (sub_section[position] == '-') {
    negative = true;
    position++;
  }

  double value = 0;
  while (position < sub_section.get_size() && sub_section[position] >= '0' &&
         sub_section[position] <= '9') {
    value *= 10;
    value += sub_section[position] - '0';
    position++;
  }

  if (position >= sub_section.get_size() || sub_section[position] != '.') {
    return value;
  }

  // Floating point accumulation
  double divisor = 1.0;
  while (position < sub_section.get_size() && sub_section[position] >= '0' &&
         sub_section[position] <= '9') {
    value *= 10;
    divisor *= 10;
    value += sub_section[position] - '0';
    position++;
  }

  return (value / divisor) * (negative ? -1 : 1);
}

auto LazyNode::get_string() const -> const Perimortem::Memory::ManagedString {
  // Convert any non-string values into views, accounting for empty as well.
  if (sub_section.empty() || sub_section[0] != '"') {
    return sub_section;
  }

  // Remove quotes on strings.
  return sub_section.slice(1, sub_section.get_size() - 2);
}

constexpr inline auto ignored_characters(char c) -> bool {
  return c == ':' || c == ',';
}

auto LazyNode::parse_section(uint32_t& position) const -> const LazyNode {
  if (sub_section.empty())
    return LazyNode(std::string_view("", 0));

  while (ignored_characters(sub_section[position])) {
    position++;
  }

  uint32_t start_position = position;
  uint32_t end_position = position;
  switch (sub_section[start_position]) {
    case '"': {
      start_position++;
      end_position++;
      while (sub_section[end_position] != '"') {
        end_position++;
      }

      position += 2;
      break;
    }

    case '[': {
      uint32_t balance = 1;
      end_position++;
      while (balance > 0) {
        if (sub_section[end_position] == ']')
          balance--;
        else if (sub_section[end_position] == '[')
          balance++;

        end_position++;
      }
      break;
    }

    case '{': {
      uint32_t balance = 1;
      end_position++;
      while (balance > 0) {
        if (sub_section[end_position] == '}')
          balance--;
        else if (sub_section[end_position] == '{')
          balance++;

        end_position++;
      }
      break;
    }

    case 't':
    case 'f':
      return LazyNode(sub_section.slice(start_position, 1));

    default:
      while (!ignored_characters(sub_section[end_position])) {
        end_position++;
      }
      break;
  }

  position += end_position - start_position;
  while (ignored_characters(sub_section[position])) {
    position++;
  }

  auto slice = sub_section.slice(start_position, end_position - start_position);
  return LazyNode(slice);
}
