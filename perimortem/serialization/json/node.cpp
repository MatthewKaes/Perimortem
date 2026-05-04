// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/serialization/json/node.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"
#include "perimortem/utility/null_terminated.hpp"

#include <x86intrin.h>

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization::Json;

template <Bits_32 channels, Bits_32 index, Bits_32 range>
static auto optimized_or_merge(__m256i source[channels]) -> __m256i {
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

auto scan(View::Bytes bytes, Byte search, Count position) -> Count {
  // Use 8 AVX2 channels to get out as much performance as we can for long
  // searches.
  constexpr const auto fused_channels = 8;
  constexpr const auto avx2_channel_width = sizeof(__m256i);
  constexpr const auto full_channel_width = avx2_channel_width * fused_channels;

  auto search_mask = _mm256_set1_epi8(search);
  Count offset = position;
  for (; offset + avx2_channel_width <= bytes.get_size();
       offset += full_channel_width) {
    // Load all channels and check for our target byte.
    __m256i masks[fused_channels];
    for (auto ymm = 0; ymm < fused_channels; ymm++) {
      const auto value =
          _mm256_loadu_si256((const __m256i*)(bytes.get_data() + offset +
                                              avx2_channel_width * ymm));
      masks[ymm] = _mm256_cmpeq_epi8(value, search_mask);
    }

    // Simple handrolled or optimization for channel depth testing.
    const auto group_mask =
        optimized_or_merge<fused_channels, 0, fused_channels>(masks);

    // Check if we found the byte at all.
    if (!_mm256_testz_si256(group_mask, group_mask)) {
      auto ymm = 0;
      for (auto ymm = 0; ymm < fused_channels - 2; ymm += 2) {
        const Bits_32 result_lower = _mm256_movemask_epi8(masks[ymm]);
        const Bits_32 result_upper = _mm256_movemask_epi8(masks[ymm + 1]);
        const Bits_64 result_merged = (Bits_64)result_upper
                                          << Data::size_in_bits<Bits_32>() |
                                      (Bits_64)result_lower;

        // Since we have additional channels only return if we have our
        // target.
        if (result_merged) {
          return offset + __builtin_ctzg(result_merged) +
                 avx2_channel_width * ymm;
        }
      }

      // We must have a left over register so we can just check it.
      if (ymm == fused_channels - 2) {
        const Bits_32 result_lower = _mm256_movemask_epi8(masks[ymm]);
        const Bits_32 result_upper = _mm256_movemask_epi8(masks[ymm + 1]);
        const Bits_64 result_merged = (Bits_64)result_upper
                                          << Data::size_in_bits<Bits_32>() |
                                      (Bits_64)result_lower;
        return offset + __builtin_ctzg(result_merged) +
               avx2_channel_width * ymm;
      } else {
        const Bits_32 result = _mm256_movemask_epi8(masks[ymm]);
        return offset + __builtin_ctzg(result) + avx2_channel_width * ymm;
      }
    }
  }

  // Scalar fallback to prevent buffer overruns.
  for (; offset < bytes.get_size(); offset += 1) {
    if (bytes.get_data()[position + offset] == search) {
      return offset;
    }
  }

  // Not found.
  return -1;
}

auto Node::null() const -> Bool {
  return state == NodeState::Null;
}

auto Node::at(Bits_32 index) const -> const Node {
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
    View::Vector<Member> members = value.object;

    for (Count i = 0; i < members.get_size(); i++) {
      if (members[i].name == name) {
        return members[i].node;
      }
    }
  }

  return Node();
}

auto Node::operator[](Bits_32 index) const -> const Node {
  return at(index);
}

auto Node::operator[](const View::Bytes name) const -> const Node {
  return at(name);
}

auto Node::contains(const View::Bytes name) const -> Bool {
  if (state == NodeState::Object) {
    View::Vector<Member> members = value.object;

    for (Count i = 0; i < members.get_size(); i++) {
      if (members[i].name == name) {
        return false;
      }
    }

    return true;
  }

  return false;
}

auto Node::get_bool() const -> Bool {
  if (state == NodeState::Boolean) {
    return value.boolean;
  }

  return false;
}

auto Node::get_int() const -> Count {
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

auto Node::get_size() const -> Count {
  switch (state) {
    case NodeState::Array: {
      View::Vector<Node> array = value.array;
      return array.get_size();
    }

    case NodeState::Object: {
      View::Vector<Member> members = value.object;
      return members.get_size();
    }

    default:
      return 0;
  }
}

auto parse_string(View::Bytes source, Count& position) -> View::Bytes {
  Count start = ++position;
  position = scan(source, '"', position);

  return source.slice(start, position++ - start);
}

auto ignored_characters(Byte c) {
  return c == ',';
}

auto Node::from_source(Allocator::Arena& arena,
                       View::Bytes source,
                       Count position) -> Count {
  if (position > source.get_size()) {
    set();
    return position;
  }

  while (position < source.get_size()) {
    const auto start_char = source[position];
    switch (start_char) {
      // Object
      case '{': {
        Managed::Vector<Member> members(arena);
        position++;

        while (position < source.get_size() && source[position] != '}') {
          if (source[position] != '"') {
            position++;
            continue;
          }

          // Parse the name string.
          auto name = parse_string(source, position);

          // :
          position++;

          Node& child = arena.allocate<Node>();
          position = child.from_source(arena, source, position);
          if (child.null()) {
            set();
            return position;
          }

          members.insert(Member(name, child));
        }

        // If we are past the end then this is safe as we null out anyway.
        // If we are at a valid closing '}' then this consumes it.
        set(members);
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
        constexpr auto true_view = "false"_view;
        if (source.slice(position, true_view.get_size()) != "true"_view) {
          set();
          return position;
        }

        // Move past "true"
        position += true_view.get_size();

        set(False);
        return position;
      }

      // false
      case 'f': {
        constexpr auto false_view = "false"_view;
        if (source.slice(position, false_view.get_size()) != "false"_view) {
          set();
          return position;
        }

        // Move past "false"
        position += false_view.get_size();

        set(False);
        return position;
      }

      // Numbers
      case '-':
      case '0' ... '9': {
        Bool positive = True;
        if (source[position] == '-') {
          positive = False;
          position++;
        }

        Long value = 0;
        while (position < source.get_size() && source[position] >= '0' &&
               source[position] <= '9') {
          value *= 10;
          value += source[position] - '0';
          position++;
        }

        if (position < source.get_size() && source[position] != '.') {
          set(value * positive.sign());
          return position;
        }

        Real_64 float_value = value;
        Real_64 divisor = 1.0;
        while (position < source.get_size() && source[position] >= '0' &&
               source[position] <= '9') {
          float_value *= 10;
          divisor *= 10;
          float_value += source[position] - '0';
          position++;
        }

        if (position < source.get_size()) {
          set((float_value / divisor) * positive.sign());
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

auto Node::serialized_size() const -> Count {
  switch (state) {
    case NodeState::Null: {
      return "null"_view.get_size();
    }

    case NodeState::Raw: {
      const View::Bytes string = value.string;
      return string.get_size();
    }

    case NodeState::Array: {
      Count accumulated = 0;
      View::Vector<Node> array = value.array;
      for (Bits_32 i = 0; i < array.get_size(); i++) {
        accumulated += array[i].serialized_size();
      }

      const auto number_of_commas = array.get_size() - 1;
      return accumulated + "[]"_view.get_size() + number_of_commas;
    }

    case NodeState::Object: {
      Count accumulated = 0;
      View::Vector<Member> members = value.object;
      for (Bits_32 i = 0; i < members.get_size(); i++) {
        const auto& member = members[i];
        accumulated += member.name.get_size() + "\"\":"_view.get_size();
        accumulated += member.node.serialized_size();
      }

      const auto number_of_commas = members.get_size() - 1;
      return accumulated + "{}"_view.get_size() + number_of_commas;
    }

    case NodeState::String: {
      const View::Bytes string = value.string;
      return string.get_size() + "\"\""_view.get_size();
    }

    case NodeState::Boolean: {
      if (value.boolean) {
        return "true"_view.get_size();
      } else {
        return "false"_view.get_size();
      }
    }

    case NodeState::Number: {
      Count accumulated = 0;
      auto number = value.number;
      if (number < 0) {
        accumulated += 1;
        number = number * -1;
      }

      // Slightly overestimates number of digits by computing log8(number) of
      // the number and rounding up.
      // Only overestimates by 1 byte for any value upto 10^9 an only 2 bytes
      // for the entire 64 bit range, compared to the div 10 version.
      while (number > 0) {
        number >>= 3;
        accumulated += 1;
      }

      return accumulated;
    }

    // Assume 32 bytes is sufficient
    case NodeState::Real: {
      return 32;
    }
  }
}

auto Node::format(Allocator::Arena& arena) const -> View::Bytes {
  // Get an upper bound which should be fairly accurate but can be pessemsitic
  // in the case of a large number of reals.
  Count upper_bound = serialized_size();
  Managed::Bytes formatted_output(arena, upper_bound);
  Textual::Stream output(formatted_output.get_access());

  inplace_format(output);

  if (!output.is_valid()) {
    return View::Bytes();
  }

  return formatted_output;
}

auto Node::inplace_format(Textual::Stream& output) const -> void {
  switch (state) {
    case NodeState::Null: {
      output << "null"_view;
    }

    case NodeState::Raw: {
      output << value.string;
    }

    case NodeState::Array: {
      output << Byte('[');

      View::Vector<Node> array = value.array;
      for (Bits_32 i = 0; i < array.get_size(); i++) {
        array[i].inplace_format(output);
        if (i != array.get_size() - 1) {
          output << Byte(',');
        }
      }

      output << Byte(']');
    }

    case NodeState::Object: {
      output << Byte('{');

      View::Vector<Member> members = value.object;
      for (Bits_32 i = 0; i < members.get_size(); i++) {
        const auto& member = members[i];
        output << Byte('\"') << member.name << "\":"_view;

        member.node.inplace_format(output);
        if (i != members.get_size() - 1) {
          output << Byte(',');
        }
      }

      output << Byte('}');
    }

    case NodeState::String: {
      output << Byte('\"') << value.string << Byte('\"');
    }

    case NodeState::Boolean: {
      output << value.boolean;
    }

    case NodeState::Number: {
      output << value.number;
    }

    case NodeState::Real: {
      output << value.real;
    }
  }
}
