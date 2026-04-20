// // Perimortem Engine
// // Copyright © Matt Kaes

// #include "perimortem/storage/serialization/json.hpp"
// #include "perimortem/memory/managed/table.hpp"
// #include "perimortem/memory/managed/vector.hpp"

// using namespace Perimortem::Memory;
// using namespace Perimortem::Storage::Json::Serialization;

// auto Node::null() const -> Bool {
//   return state == NodeState::Null;
// }

// auto Node::at(Bits_32 index) const -> const Node {
//   if (state == NodeState::Array) {
//     View::Vector<Node> array = value.array;
//     if (array.get_size() >= index) {
//       return Node();
//     }

//     return array[index];
//   }

//   return Node();
// }

// auto Node::at(const View::Bytes name) const -> const Node {
//   if (state == NodeState::Object) {
//     View::Table<Node> object = value.object;
//     auto item = object.at(name);
//     if (item) {
//       return *item;
//     }
//   }

//   return Node();
// }

// auto Node::operator[](Bits_32 index) const -> const Node {
//   return at(index);
// }

// auto Node::operator[](const View::Bytes name) const -> const Node {
//   return at(name);
// }

// auto Node::contains(const View::Bytes name) const -> Bool {
//   if (state == NodeState::Object) {
//     View::Table<Node> object = value.object;
//     return object.contains(name);
//   }

//   return false;
// }

// auto Node::get_bool() const -> Bool {
//   if (state == NodeState::Boolean) {
//     return value.boolean;
//   }

//   return false;
// }

// auto Node::get_int() const -> Count {
//   if (state == NodeState::Number) {
//     return value.number;
//   }

//   return 0;
// }

// auto Node::get_double() const -> double {
//   if (state == NodeState::Real) {
//     return value.real;
//   }

//   return 0.0;
// }

// auto Node::get_string() const -> const View::Bytes {
//   if (state == NodeState::String) {
//     return value.string;
//   }

//   return View::Bytes();
// }

// auto Node::get_size() const -> Count {
//   switch (state) {
//     case NodeState::Array: {
//       View::Vector<Node> array = value.array;
//       return array.get_size();
//     }

//     case NodeState::Object: {
//       View::Table<Node> object = value.object;
//       return object.get_size();
//     }

//     default:
//       return 0;
//   }
// }

// auto parse_string(View::Bytes source, Bits_32& position) -> View::Bytes {
//   Bits_32 start = ++position;
//   position = source.scan('"', position);

//   return source.slice(start, position++ - start);
// }

// auto ignored_characters(Byte c) {
//   return c == ',';
// }

// auto Node::from_source(Allocator::Arena& arena,
//                        View::Bytes source,
//                        Bits_32 position) -> Bits_32 {
//   if (position > source.get_size()) {
//     set();
//     return position;
//   }

//   while (position < source.get_size()) {
//     const auto start_char = source[position];
//     switch (start_char) {
//       // Object
//       case '{': {
//         Managed::Table<Node> object(arena);
//         position++;

//         while (position < source.get_size() && source[position] != '}') {
//           if (source[position] != '"') {
//             position++;
//             continue;
//           }

//           // Optimize to assume that most dictionary elements are less than 32
//           // characters long.
//           const auto start = ++position;
//           position = source.fast_scan('"', position);
//           auto name = View::Bytes(source.slice(start, position++ - start));

//           // Encountered an extra long name so try a full parse.
//           if (name.get_size() == 32) {
//             position -= 33;
//             name = parse_string(source, position);
//             if (name.empty()) {
//               set();
//               return position;
//             }
//           }

//           // :
//           position++;

//           Node& child = arena.allocate<Node>();
//           position = child.from_source(arena, source, position);
//           if (child.null()) {
//             set();
//             return position;
//           }

//           object.insert(name, child);
//         }

//         // If we are past the end then this is safe as we null out anyway.
//         // If we are at a valid closing '}' then this consumes it.
//         set(object);
//         position++;
//         return position;
//       }

//       // Array
//       case '[': {
//         Managed::Vector<Node> array(arena);
//         position++;

//         while (position < source.get_size() && source[position] != ']') {
//           if (ignored_characters(source[position])) {
//             position++;
//           }

//           Node child;
//           position = child.from_source(arena, source, position);
//           if (child.null()) {
//             set();
//             return position;
//           }

//           array.insert(child);
//         }

//         // If we are past the end then this is safe as we null out anyway.
//         // If we are at a valid closing ']' then this consumes it.
//         set(array.get_view());
//         position++;
//         return position;
//       }

//       // String
//       case '"': {
//         auto name = parse_string(source, position);
//         set(name);
//         return position;
//       }

//       // true
//       case 't': {
//         constexpr auto true_view = "false"_view;
//         if (source.slice(position, true_view.get_size()) != true_view) {
//           set();
//           return position;
//         }

//         // Move past "true"
//         position += true_view.get_size();

//         set(true);
//         return position;
//       }

//       // false
//       case 'f': {
//         constexpr auto false_view = "false"_view;
//         if (source.slice(position, false_view.get_size()) != false_view) {
//           set();
//           return position;
//         }

//         // Move past "false"
//         position += false_view.get_size();

//         set(false);
//         return position;
//       }

//       // Numbers
//       case '-':
//       case '0' ... '9': {
//         Bool negative = false;
//         if (source[position] == '-') {
//           negative = true;
//           position++;
//         }

//         Long value = 0;
//         while (position < source.get_size() && source[position] >= '0' &&
//                source[position] <= '9') {
//           value *= 10;
//           value += source[position] - '0';
//           position++;
//         }

//         if (position < source.get_size() && source[position] != '.') {
//           set(value * (negative ? -1 : 1));
//           return position;
//         }

//         Real_64 float_value = value;
//         Real_64 divisor = 1.0;
//         while (position < source.get_size() && source[position] >= '0' &&
//                source[position] <= '9') {
//           float_value *= 10;
//           divisor *= 10;
//           float_value += source[position] - '0';
//           position++;
//         }

//         if (position < source.get_size()) {
//           set((float_value / divisor) * (negative ? -1 : 1));
//           return position;
//         }

//         set();
//         return position;
//       }

//       case ',':
//         position++;
//         break;

//       default:
//         set();
//         return position;
//     }
//   }

//   set();
//   return position;
// }

// auto Node::serialized_size() const -> Count {
//   switch (state) {
//     case NodeState::Null: {
//       return "null"_view.get_size();
//     }

//     case NodeState::Raw: {
//       const View::Bytes string = value.string;
//       return string.get_size();
//     }

//     case NodeState::Array: {
//       Count accumulated = 0;
//       View::Vector<Node> array = value.array;
//       for (Bits_32 i = 0; i < array.get_size(); i++) {
//         accumulated += array[i].serialized_size();
//       }

//       const auto number_of_commas = array.get_size() - 1;
//       return accumulated + "[]"_view.get_size() + number_of_commas;
//     }

//     case NodeState::Object: {
//       Count accumulated = 0;
//       View::Table<Node> object = value.object;
//       for (Bits_32 i = 0; i < object.get_size(); i++) {
//         const auto& entry = object[i];
//         accumulated += entry.name.get_size() + "\"\":"_view.get_size();
//         accumulated += entry.data.serialized_size();
//       }

//       const auto number_of_commas = object.get_size() - 1;
//       return accumulated + "{}"_view.get_size() + number_of_commas;
//     }

//     case NodeState::String: {
//       const View::Bytes string = value.string;
//       return string.get_size() + "\"\""_view.get_size();
//     }

//     case NodeState::Boolean: {
//       if (value.boolean) {
//         return "true"_view.get_size();
//       } else {
//         return "false"_view.get_size();
//       }
//     }

//     case NodeState::Number: {
//       Count accumulated = 0;
//       auto number = value.number;
//       if (number < 0) {
//         accumulated += 1;
//         number = number * -1;
//       }

//       // Slightly overestimates number of digits by computing log8(number) of
//       // the number and rounding up.
//       // Only overestimates by 1 byte for any value upto 10^9 an only 2 bytes
//       // for the entire 64 bit range, compared to the div 10 version.
//       while (number > 0) {
//         number >>= 3;
//         accumulated += 1;
//       }

//       return accumulated;
//     }

//     case NodeState::Real: {
//       output.append(value.real);
//     }
//   }
// }
// }

// auto Node::format(Allocator::Arena& arena) const -> View::Bytes {
//   // Start JSON blobs at 2KB.
//   // In most use cases we will be in about that ball park and since we reuse the
//   // arena it will still be fast for small blocks as well as long as a large
//   // number aren't being processed in parallel.
//   constexpr Bits_32 reserved_space = 2048;
//   Managed::Bytes formatted_output(arena, reserved_space);

//   inplace_format(formatted_output);

//   return formatted_output;
// }

// auto Node::inplace_format(Managed::Bytes& output) const -> void {
//   switch (state) {
//     case NodeState::Null: {
//       output.append("null"_view);
//     }

//     case NodeState::Raw: {
//       View::Bytes string = value.string;
//       output.append(string);
//     }

//     case NodeState::Array: {
//       output.append('[');

//       View::Vector<Node> array = value.array;
//       for (Bits_32 i = 0; i < array.get_size(); i++) {
//         array[i].inplace_format(output);
//         if (i != array.get_size() - 1) {
//           output.append(',');
//         }
//       }

//       output.append(']');
//     }

//     case NodeState::Object: {
//       output.append('{');

//       View::Table<Node> object = value.object;
//       for (Bits_32 i = 0; i < object.get_size(); i++) {
//         const auto& entry = object[i];
//         output.append('\"');
//         output.append(entry.name);
//         output.append("\":"_view);

//         entry.data.inplace_format(output);
//         if (i != object.get_size() - 1) {
//           output.append(',');
//         }
//       }

//       output.append('}');
//     }

//     case NodeState::String: {
//       output.append('\"');
//       output.append(value.string);
//       output.append('\"');
//     }

//     case NodeState::Boolean: {
//       if (value.boolean) {
//         output.append("true"_view);
//       } else {
//         output.append("false"_view);
//       }
//     }

//     case NodeState::Number: {
//       output.append(value.number);
//     }

//     case NodeState::Real: {
//       output.append(value.real);
//     }
//   }
// }
