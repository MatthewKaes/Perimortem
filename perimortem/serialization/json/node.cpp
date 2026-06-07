// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/serialization/json/node.hpp"

#include <x86intrin.h>

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

enum class NodeState : Bits_32 {
  Null,
  String,
  Number,
  Real,
  Object,
  Array,
  Flag
};

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;

auto construct_from_blueprint(
    Allocator::Arena& arena,
    const Json::Blueprint& bp) -> Json::Node;

auto construct_object(
    Allocator::Arena& arena,
    const Json::Blueprint* entries,
    Count count) -> Json::Node {
  Managed::Vector<Json::Member> members(arena);

  for (Count i = 0; i < count; i++) {
    members.insert(
        Json::Member{
          entries[i].name, construct_from_blueprint(arena, entries[i])});
  }

  Json::Node result;
  result.set(members);
  return result;
}

auto construct_array(
    Allocator::Arena& arena,
    const Json::Blueprint* items,
    Count count) -> Json::Node {
  Managed::Vector<Json::Node> nodes(arena);

  for (Count i = 0; i < count; i++) {
    nodes.insert(construct_from_blueprint(arena, items[i]));
  }

  Json::Node result;
  result.set(nodes.get_view());
  return result;
}

auto construct_from_blueprint(
    Allocator::Arena& arena,
    const Json::Blueprint& bp) -> Json::Node {
  switch (bp.tag) {
  case Json::Blueprint::Tag::Null:
    return Json::Node();
  case Json::Blueprint::Tag::String:
    return Json::Node(bp.string_val);
  case Json::Blueprint::Tag::Number:
    return Json::Node(bp.number_val);
  case Json::Blueprint::Tag::Real:
    return Json::Node(bp.real_val);
  case Json::Blueprint::Tag::Flag:
    return Json::Node(bp.flag_val);
  case Json::Blueprint::Tag::Compound: {
    // Named children → object; unnamed children → array.
    const Bool is_object =
        bp.compound.size > 0 && bp.compound.ptr[0].name.get_size() > 0;
    if (is_object) {
      return construct_object(arena, bp.compound.ptr, bp.compound.size);
    }
    return construct_array(arena, bp.compound.ptr, bp.compound.size);
  }
  }
}

template <Bits_32 channels, Bits_32 index, Bits_32 range>
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

auto scan(View::Bytes bytes, Byte search, Count position) -> Count {
  // Use 8 AVX2 channels to get out as much performance as we can for long
  // searches.
  constexpr const auto fused_channels = 8;
  constexpr const auto avx2_channel_width = sizeof(__m256i);
  constexpr const auto full_channel_width = avx2_channel_width * fused_channels;

  auto search_mask = _mm256_set1_epi8(search);
  for (; position + full_channel_width <= bytes.get_size();
       position += full_channel_width) {
    // Load all channels and check for our target byte.
    __m256i masks[fused_channels];
    for (auto ymm = 0; ymm < fused_channels; ymm++) {
      const auto value =
          _mm256_loadu_si256((const __m256i*)(bytes.get_data() + position +
                                              avx2_channel_width * ymm));
      masks[ymm] = _mm256_cmpeq_epi8(value, search_mask);
    }

    // Simple handrolled or optimization for channel depth testing.
    const auto group_mask =
        optimized_or_merge<fused_channels, 0, fused_channels>(masks);

    // Check if we found the byte at all.
    if (!_mm256_testz_si256(group_mask, group_mask)) {
      Count ymm;
      for (ymm = 0; ymm < fused_channels - 2; ymm += 2) {
        const Bits_32 result_lower = _mm256_movemask_epi8(masks[ymm]);
        const Bits_32 result_upper = _mm256_movemask_epi8(masks[ymm + 1]);
        const Bits_64 result_merged = (Bits_64)result_upper
                                          << Data::size_in_bits<Bits_32>() |
                                      (Bits_64)result_lower;

        // Since we have additional channels only return if we have our
        // target.
        if (result_merged) {
          return position + __builtin_ctzg(result_merged) +
                 avx2_channel_width * ymm;
        }
      }

      // Not found in channels 0..fused_channels-2, so it must be in the
      // final pair.
      const Bits_32 result_lower = _mm256_movemask_epi8(masks[ymm]);
      const Bits_32 result_upper = _mm256_movemask_epi8(masks[ymm + 1]);
      const Bits_64 result_merged = (Bits_64)result_upper
                                        << Data::size_in_bits<Bits_32>() |
                                    (Bits_64)result_lower;
      return position + __builtin_ctzg(result_merged) +
             avx2_channel_width * ymm;
    }
  }

  // Scalar fallback to prevent buffer overruns.
  for (; position < bytes.get_size(); position += 1) {
    if (bytes.get_data()[position] == search) {
      return position;
    }
  }

  // String was never closed so just treat the entire range as a string.
  return Count(-1);
}

auto parse_string(View::Bytes source, Count& position) -> View::Bytes {
  Count start = ++position;
  position = scan(source, '"', position);

  return source.slice(start, position++ - start);
}

auto ignored_characters(Byte c) {
  return c == ',' || c == '\n' || c == ' ';
}

auto Json::Node::set(const Core::View::Bytes value) -> void {
  data.ptr = value.get_data();
  data.size = value.get_size();
  data.state = (Bits_32)NodeState::String;
}

auto Json::Node::set(const Core::View::Vector<Node> value) -> void {
  data.ptr = value.get_data();
  data.size = value.get_size();
  data.state = (Bits_32)NodeState::Array;
}

auto Json::Node::set(const Core::View::Vector<Member> value) -> void {
  data.ptr = value.get_data();
  data.size = value.get_size();
  data.state = (Bits_32)NodeState::Object;
}

auto Json::Node::set(Long value) -> void {
  data.number = value;
  data.state = (Bits_32)NodeState::Number;
}

auto Json::Node::set(Real_64 value) -> void {
  data.real = value;
  data.state = (Bits_32)NodeState::Real;
}

auto Json::Node::set(Bool value) -> void {
  data.flag = value;
  data.state = (Bits_32)NodeState::Flag;
}

auto Json::Node::set() -> void {
  data.state = (Bits_32)NodeState::Null;
}

auto Json::Node::at(Bits_32 index) const -> const Json::Node {
  if (data.state == (Bits_32)NodeState::Array) {
    View::Vector<Json::Node> array((const Json::Node*)data.ptr, data.size);
    if (array.get_size() <= index) {
      return Json::Node();
    }

    return array[index];
  }

  return Json::Node();
}

auto Json::Node::at(const View::Bytes name) const -> const Json::Node {
  if (data.state == (Bits_32)NodeState::Object) {
    View::Vector<Member> members((const Member*)data.ptr, data.size);

    for (Count i = 0; i < members.get_size(); i++) {
      if (members[i].name == name) {
        return members[i].node;
      }
    }
  }

  return Json::Node();
}

auto Json::Node::operator[](Bits_32 index) const -> const Json::Node {
  return at(index);
}

auto Json::Node::operator[](const View::Bytes name) const -> const Json::Node {
  return at(name);
}

auto Json::Node::contains(const View::Bytes name) const -> Bool {
  if (data.state == (Bits_32)NodeState::Object) {
    View::Vector<Member> members((const Member*)data.ptr, data.size);

    for (Count i = 0; i < members.get_size(); i++) {
      if (members[i].name == name) {
        return true;
      }
    }

    return false;
  }

  return false;
}

auto Json::Node::get_flag() const -> Bool {
  if (data.state == (Bits_32)NodeState::Flag) {
    return data.flag;
  }

  return false;
}

auto Json::Node::get_number() const -> Count {
  if (data.state == (Bits_32)NodeState::Number) {
    return data.number;
  }

  return 0;
}

auto Json::Node::get_real() const -> double {
  if (data.state == (Bits_32)NodeState::Real) {
    return data.real;
  }

  return 0.0;
}

auto Json::Node::get_string() const -> const View::Bytes {
  if (data.state == (Bits_32)NodeState::String) {
    return View::Bytes((const Byte*)data.ptr, data.size);
  }

  return View::Bytes();
}

auto Json::Node::get_array() const -> const View::Vector<Node> {
  if (data.state == (Bits_32)NodeState::Array) {
    return View::Vector<Node>((const Node*)data.ptr, data.size);
  }

  return View::Vector<Node>();
}

auto Json::Node::get_object() const -> const View::Vector<Member> {
  if (data.state == (Bits_32)NodeState::Object) {
    return View::Vector<Member>((const Member*)data.ptr, data.size);
  }

  return View::Vector<Member>();
}

auto Json::Node::get_size() const -> Count {
  switch ((NodeState)data.state) {
  case NodeState::String:
  case NodeState::Array:
  case NodeState::Object:
    return data.size;

  default:
    return 0;
  }
}

auto Json::Node::is_null() const -> Bool {
  return (NodeState)data.state == NodeState::Null;
}

auto Json::Node::is_flag() const -> Bool {
  return (NodeState)data.state == NodeState::Flag;
}

auto Json::Node::is_number() const -> Bool {
  return (NodeState)data.state == NodeState::Number;
}

auto Json::Node::is_real() const -> Bool {
  return (NodeState)data.state == NodeState::Real;
}

auto Json::Node::is_string() const -> Bool {
  return (NodeState)data.state == NodeState::String;
}

auto Json::Node::is_array() const -> Bool {
  return (NodeState)data.state == NodeState::Array;
}

auto Json::Node::is_object() const -> Bool {
  return (NodeState)data.state == NodeState::Object;
}

auto Json::Node::construct(
    Allocator::Arena& arena,
    const Json::Blueprint* entries,
    Count count) -> Node {
  return construct_object(arena, entries, count);
}

auto Json::Node::construct(Allocator::Arena& arena, const Json::Blueprint& root)
    -> Node {
  return construct_from_blueprint(arena, root);
}

auto Json::Node::parse(
    Allocator::Arena& arena,
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

      while (position < source.get_size()) {
        if (source[position] != '"') {
          if (source[position++] == '}') {
            break;
          }

          continue;
        }

        // Parse the name string.
        auto name = parse_string(source, position);

        // :
        position++;

        Json::Node& child = arena.allocate<Json::Node>();
        position = child.parse(arena, source, position);
        if (child.is_null()) {
          set();
          return position;
        }

        members.insert(Member(name, child));
      }

      set(members);
      return position;
    }

    // Array
    case '[': {
      Managed::Vector<Json::Node> array(arena);
      position++;

      while (position < source.get_size()) {
        if (ignored_characters(source[position])) {
          position++;
        }

        if (source[position] == ']') {
          break;
        }

        Json::Node child;
        position = child.parse(arena, source, position);
        if (child.is_null()) {
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
      constexpr auto true_view = "true"_view;
      if (source.slice(position, true_view.get_size()) != "true"_view) {
        set();
        return position;
      }

      // Move past "true"
      position += true_view.get_size();

      set(True);
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

      if (position >= source.get_size() || source[position] != '.') {
        set(value * positive.sign());
        return position;
      }

      position++;  // consume '.'
      Real_64 float_value = value;
      Bits_64 divisor = 1;

      // Try to perserve precision by using fixed point and only convert into
      // floating point once.
      while (position < source.get_size() && source[position] >= '0' &&
             source[position] <= '9') {
        float_value *= 10;
        divisor *= 10;
        float_value += source[position] - '0';
        position++;
      }

      set((float_value / Real_64(divisor)) * positive.sign());
      return position;
    }

    default:
      position++;
      break;
    }
  }

  set();
  return position;
}

auto Json::Node::serialized_size() const -> Count {
  switch ((NodeState)data.state) {
  case NodeState::Null: {
    return "null"_view.get_size();
  }

  case NodeState::Array: {
    Count accumulated = 0;
    View::Vector<Json::Node> array = get_array();
    for (Bits_32 i = 0; i < array.get_size(); i++) {
      accumulated += array[i].serialized_size();
    }

    const auto number_of_commas = array.get_size() - 1;
    return accumulated + "[]"_view.get_size() + number_of_commas;
  }

  case NodeState::Object: {
    Count accumulated = 0;
    View::Vector<Member> members = get_object();
    for (Bits_32 i = 0; i < members.get_size(); i++) {
      const auto& member = members[i];
      accumulated += member.name.get_size() + "\"\":"_view.get_size();
      accumulated += member.node.serialized_size();
    }

    const auto number_of_commas = members.get_size() - 1;
    return accumulated + "{}"_view.get_size() + number_of_commas;
  }

  case NodeState::String: {
    auto length = get_size();
    return length + "\"\""_view.get_size();
  }

  case NodeState::Flag: {
    if (data.flag) {
      return "true"_view.get_size();
    } else {
      return "false"_view.get_size();
    }
  }

  case NodeState::Number: {
    Count accumulated = 0;
    auto number = data.number;
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

auto Json::Node::format(Allocator::Arena& arena) const -> View::Bytes {
  // Get an upper bound which should be fairly accurate but can be pessemsitic
  // in the case of a large number of reals.
  Count upper_bound = serialized_size();
  Managed::Bytes formatted_output(arena);
  formatted_output.resize(upper_bound);
  Writer::Textual output(formatted_output.get_access());

  auto inplace_format = [](this auto&& self, Writer::Textual& output,
                           const Json::Node& node) -> void {
    switch ((NodeState)node.data.state) {
    case NodeState::Null: {
      output << "null"_view;
      return;
    }

    case NodeState::Array: {
      output << Byte('[');

      View::Vector<Json::Node> array = node.get_array();
      for (Bits_32 i = 0; i < array.get_size(); i++) {
        self(output, array[i]);
        if (i != array.get_size() - 1) {
          output << Byte(',');
        }
      }

      output << Byte(']');
      return;
    }

    case NodeState::Object: {
      output << Byte('{');

      View::Vector<Member> members = node.get_object();
      for (Bits_32 i = 0; i < members.get_size(); i++) {
        const auto& member = members[i];
        output << Byte('\"') << member.name << "\":"_view;

        self(output, member.node);
        if (i != members.get_size() - 1) {
          output << Byte(',');
        }
      }

      output << Byte('}');
      return;
    }

    case NodeState::String: {
      output << Byte('\"') << node.get_string() << Byte('\"');
      return;
    }

    case NodeState::Flag: {
      output << node.data.flag;
      return;
    }

    case NodeState::Number: {
      output << node.data.number;
      return;
    }

    case NodeState::Real: {
      output << node.data.real;
      return;
    }
    }
  };

  // Start the format chain.
  inplace_format(output, *this);

  // If we went over the buffer or if we failed to format the data return an
  // empty view.
  if (!output.is_valid()) {
    return View::Bytes();
  }

  // Shrink to actual size.
  formatted_output.resize(output.get_location());
  return formatted_output;
}
