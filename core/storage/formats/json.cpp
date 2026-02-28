// Perimortem Engine
// Copyright © Matt Kaes

#include "storage/formats/json.hpp"

#include "memory/managed/table.hpp"
#include "memory/managed/vector.hpp"

#include <x86intrin.h>
#include <bit>
#include <charconv>

using namespace Perimortem::Memory;
using namespace Perimortem::Storage::Json;

auto Node::null() const -> bool {
  return state == NodeState::Null;
}

auto Node::at(uint32_t index) const -> const Node {
  if (state == NodeState::Array) {
    View::Vector<Node> array = value.array;
    if (array.get_size() >= index) {
      return Node();
    }

    return array[index];
  }

  return Node();
}

auto Node::at(const View::Bytes name) const -> const Node {
  if (state == NodeState::Object) {
    View::Table<Node> object = value.object;
    auto item = object.at(name);
    if (item) {
      return *item;
    }
  }

  return Node();
}

auto Node::operator[](uint32_t index) const -> const Node {
  return at(index);
}

auto Node::operator[](const View::Bytes name) const -> const Node {
  return at(name);
}

auto Node::contains(const View::Bytes name) const -> bool {
  if (state == NodeState::Object) {
    View::Table<Node> object = value.object;
    return object.contains(name);
  }

  return false;
}

auto Node::get_bool() const -> bool {
  if (state == NodeState::Boolean) {
    return value.boolean;
  }

  return false;
}

auto Node::get_int() const -> int64_t {
  if (state == NodeState::Number) {
    return value.number;
  }

  return 0;
}

auto Node::get_double() const -> double {
  if (state == NodeState::Real) {
    return value.real;
  }

  return 0.0;
}

auto Node::get_string() const -> const View::Bytes {
  if (state == NodeState::String) {
    return value.string;
  }

  return View::Bytes();
}

auto Node::get_size() const -> uint64_t {
  switch (state) {
    case NodeState::Array: {
      View::Vector<Node> array = value.array;
      return array.get_size();
    }

    case NodeState::Object: {
      View::Table<Node> object = value.object;
      return object.get_size();
    }

    default:
      return 0;
  }
}

auto parse_string(View::Bytes source, uint32_t& position) -> View::Bytes {
  uint32_t start = ++position;
  position = source.scan('"', position);

  return source.slice(start, position++ - start);
}

auto ignored_characters(char c) {
  return c == ',';
}

auto Node::from_source(Arena& arena, View::Bytes source, uint32_t position)
    -> uint32_t {
  if (position > source.get_size()) {
    set();
    return position;
  }

  while (position < source.get_size()) {
    const auto start_char = source[position];
    switch (start_char) {
      // Object
      case '{': {
        Managed::Table<Node> object(arena);
        position++;

        while (position < source.get_size() && source[position] != '}') {
          if (source[position] != '"') {
            position++;
            continue;
          }

          // Optimize to assume that most dictionary elements are less than 32
          // characters long.
          const auto start = ++position;
          position = source.fast_scan('"', position);
          auto name = View::Bytes(source.slice(start, position++ - start));

          // Encountered an extra long name so try a full parse.
          if (name.get_size() == 32) {
            position -= 33;
            name = parse_string(source, position);
            if (name.empty()) {
              set();
              return position;
            }
          }

          // :
          position++;

          Node& child = arena.allocate<Node>();
          position = child.from_source(arena, source, position);
          if (child.null()) {
            set();
            return position;
          }

          object.insert(name, child);
        }

        // If we are past the end then this is safe as we null out anyway.
        // If we are at a valid closing '}' then this consumes it.
        set(object);
        position++;
        return position;
      }

      // Array
      case '[': {
        Managed::Vector<Node> array(arena);
        position++;

        while (position < source.get_size() && source[position] != ']') {
          if (ignored_characters(source[position])) {
            position++;
          }

          Node child;
          position = child.from_source(arena, source, position);
          if (child.null()) {
            set();
            return position;
          }

          array.insert(child);
        }

        // If we are past the end then this is safe as we null out anyway.
        // If we are at a valid closing ']' then this consumes it.
        set(array.get_view());
        position++;
        return position;
      }

      // String
      case '"': {
        auto name = parse_string(source, position);
        set(name);
        return position;
      }

      // true
      case 't': {
        if (position + 3 >= source.get_size() ||
            std::memcmp(source.get_data() + position, "true", 4)) {
          set();
          return position;
        }

        // Move past "true"
        position += 4;

        set(true);
        return position;
      }

      // false
      case 'f': {
        if (position + 4 >= source.get_size() ||
            std::memcmp(source.get_data() + position, "false", 5)) {
          set();
          return position;
        }

        // Move past "false"
        position += 5;

        set(false);
        return position;
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
          set(value * (negative ? -1 : 1));
          return position;
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
          set((float_value / divisor) * (negative ? -1 : 1));
          return position;
        }

        set();
        return position;
      }

      case ',':
        position++;
        break;

      default:
        set();
        return position;
    }
  }

  set();
  return position;
}

auto Node::format(Arena& arena) const -> View::Bytes {
  // Start JSON blobs at 2KB.
  // In most use cases we will be in about that ball park and since we reuse the
  // arena it will still be fast for small blocks as well as long as a large
  // number aren't being processed in parallel.
  constexpr uint32_t reserved_space = 2048;
  Managed::Bytes formatted_output(arena, reserved_space);

  inplace_format(formatted_output);

  return formatted_output;
}

auto Node::inplace_format(Managed::Bytes& output) const -> void {
  switch (state) {
    case NodeState::Null: {
      output.append("null");
    }

    case NodeState::Raw: {
      View::Bytes string = value.string;
      output.append(string);
    }

    case NodeState::Array: {
      output.append("[");

      View::Vector<Node> array = value.array;
      for (uint32_t i = 0; i < array.get_size(); i++) {
        array[i].inplace_format(output);
        if (i != array.get_size() - 1) {
          output.append(",");
        }
      }

      output.append("]");
    }

    case NodeState::Object: {
      output.append("{");

      View::Table<Node> object = value.object;
      for (uint32_t i = 0; i < object.get_size(); i++) {
        const auto& entry = object[i];
        output.append("\"");
        output.append(entry.name);
        output.append("\":");

        entry.data.inplace_format(output);
        if (i != object.get_size() - 1) {
          output.append(",");
        }
      }

      output.append("}");
    }

    case NodeState::String: {
      output.append("\"");
      output.append(value.string);
      output.append("\"");
    }

    case NodeState::Boolean: {
      if (value.boolean) {
        output.append("true");
      } else {
        output.append("false");
      }
    }

    case NodeState::Number: {
      output.append(value.number);
    }

    case NodeState::Real: {
      output.append(value.real);
    }
  }
}
