// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/assembler/spirv.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Compiler::Assembler;

static auto read_word(View::Bytes words, Count word_index) -> Bits_32 {
  Count byte_index = word_index * 4;
  if (byte_index + 4 > words.get_size()) {
    return 0;
  }

  return Bits_32(words[byte_index]) | (Bits_32(words[byte_index + 1]) << 8) |
         (Bits_32(words[byte_index + 2]) << 16) |
         (Bits_32(words[byte_index + 3]) << 24);
}

auto spirv::begin_module(Bits_32 bound, Version version, Bits_32 generator)
    -> void {
  word(magic);
  word(Bits_32(version));
  word(generator);
  word(bound);
  word(0);
}

auto spirv::word(Bits_32 value) -> void {
  words.append(Bits_8(value & 0xFF));
  words.append(Bits_8((value >> 8) & 0xFF));
  words.append(Bits_8((value >> 16) & 0xFF));
  words.append(Bits_8((value >> 24) & 0xFF));
}

auto spirv::instruction(Op opcode, Count word_count) -> void {
  word((Bits_32(word_count) << 16) | Bits_32(opcode));
}

auto spirv::literal_string(View::Bytes text) -> Count {
  Count byte_index = 0;
  Count written = 0;
  Count words = literal_string_word_count(text);
  for (Count i = 0; i < words; i++) {
    Bits_32 packed = 0;
    for (Count j = 0; j < 4; j++) {
      Bits_8 byte = 0;
      if (byte_index < text.get_size()) {
        byte = text[byte_index];
      }

      packed |= Bits_32(byte) << (j * 8);
      byte_index++;
    }

    word(packed);
    written++;
  }

  return written;
}

auto spirv::capability(Capability value) -> void {
  instruction(Op::Capability, 2);
  word(Bits_32(value));
}

auto spirv::memory_model(AddressingModel addressing, MemoryModel memory)
    -> void {
  instruction(Op::MemoryModel, 3);
  word(Bits_32(addressing));
  word(Bits_32(memory));
}

auto spirv::entry_point(
    ExecutionModel model,
    Bits_32 function_id,
    View::Bytes name) -> void {
  entry_point(model, function_id, name, View::Vector<Bits_32>());
}

auto spirv::entry_point(
    ExecutionModel model,
    Bits_32 function_id,
    View::Bytes name,
    View::Vector<Bits_32> interface_ids) -> void {
  instruction(
      Op::EntryPoint,
      3 + literal_string_word_count(name) + interface_ids.get_size());
  word(Bits_32(model));
  word(function_id);
  literal_string(name);
  for (Count i = 0; i < interface_ids.get_size(); i++) {
    word(interface_ids[i]);
  }
}

auto spirv::execution_mode(Bits_32 entry_point_id, ExecutionMode mode) -> void {
  instruction(Op::ExecutionMode, 3);
  word(entry_point_id);
  word(Bits_32(mode));
}

auto spirv::name(Bits_32 target_id, View::Bytes name) -> void {
  instruction(Op::Name, 2 + literal_string_word_count(name));
  word(target_id);
  literal_string(name);
}

auto spirv::member_name(
    Bits_32 target_id,
    Bits_32 member_index,
    View::Bytes name) -> void {
  instruction(Op::MemberName, 3 + literal_string_word_count(name));
  word(target_id);
  word(member_index);
  literal_string(name);
}

auto spirv::decorate(Bits_32 target_id, Decoration decoration, Bits_32 value)
    -> void {
  instruction(Op::Decorate, 4);
  word(target_id);
  word(Bits_32(decoration));
  word(value);
}

auto spirv::decorate(Bits_32 target_id, Decoration decoration) -> void {
  instruction(Op::Decorate, 3);
  word(target_id);
  word(Bits_32(decoration));
}

auto spirv::member_decorate(
    Bits_32 target_id,
    Bits_32 member_index,
    Decoration decoration,
    Bits_32 value) -> void {
  instruction(Op::MemberDecorate, 5);
  word(target_id);
  word(member_index);
  word(Bits_32(decoration));
  word(value);
}

auto spirv::type_void(Bits_32 result_id) -> void {
  instruction(Op::TypeVoid, 2);
  word(result_id);
}

auto spirv::type_bool(Bits_32 result_id) -> void {
  instruction(Op::TypeBool, 2);
  word(result_id);
}

auto spirv::type_int(Bits_32 result_id, Bits_32 width, Bool signedness)
    -> void {
  instruction(Op::TypeInt, 4);
  word(result_id);
  word(width);
  word(signedness ? 1 : 0);
}

auto spirv::type_float(Bits_32 result_id, Bits_32 width) -> void {
  instruction(Op::TypeFloat, 3);
  word(result_id);
  word(width);
}

auto spirv::type_vector(
    Bits_32 result_id,
    Bits_32 component_type_id,
    Bits_32 component_count) -> void {
  instruction(Op::TypeVector, 4);
  word(result_id);
  word(component_type_id);
  word(component_count);
}

auto spirv::type_image(
    Bits_32 result_id,
    Bits_32 sampled_type_id,
    Dim dim,
    Bits_32 depth,
    Bits_32 arrayed,
    Bits_32 multisampled,
    Bits_32 sampled,
    ImageFormat format) -> void {
  instruction(Op::TypeImage, 9);
  word(result_id);
  word(sampled_type_id);
  word(Bits_32(dim));
  word(depth);
  word(arrayed);
  word(multisampled);
  word(sampled);
  word(Bits_32(format));
}

auto spirv::type_sampler(Bits_32 result_id) -> void {
  instruction(Op::TypeSampler, 2);
  word(result_id);
}

auto spirv::type_sampled_image(Bits_32 result_id, Bits_32 image_type_id)
    -> void {
  instruction(Op::TypeSampledImage, 3);
  word(result_id);
  word(image_type_id);
}

auto spirv::type_array(
    Bits_32 result_id,
    Bits_32 element_type_id,
    Bits_32 length_id) -> void {
  instruction(Op::TypeArray, 4);
  word(result_id);
  word(element_type_id);
  word(length_id);
}

auto spirv::type_struct(
    Bits_32 result_id,
    View::Vector<Bits_32> member_type_ids) -> void {
  instruction(Op::TypeStruct, 2 + member_type_ids.get_size());
  word(result_id);
  for (Count i = 0; i < member_type_ids.get_size(); i++) {
    word(member_type_ids[i]);
  }
}

auto spirv::type_pointer(
    Bits_32 result_id,
    StorageClass storage_class,
    Bits_32 type_id) -> void {
  instruction(Op::TypePointer, 4);
  word(result_id);
  word(Bits_32(storage_class));
  word(type_id);
}

auto spirv::type_function(Bits_32 result_id, Bits_32 return_type_id) -> void {
  instruction(Op::TypeFunction, 3);
  word(result_id);
  word(return_type_id);
}

auto spirv::constant(Bits_32 result_type_id, Bits_32 result_id, Bits_32 value)
    -> void {
  instruction(Op::Constant, 4);
  word(result_type_id);
  word(result_id);
  word(value);
}

auto spirv::constant_composite(
    Bits_32 result_type_id,
    Bits_32 result_id,
    View::Vector<Bits_32> constituents) -> void {
  instruction(Op::ConstantComposite, 3 + constituents.get_size());
  word(result_type_id);
  word(result_id);
  for (Count i = 0; i < constituents.get_size(); i++) {
    word(constituents[i]);
  }
}

auto spirv::variable(
    Bits_32 result_type_id,
    Bits_32 result_id,
    StorageClass storage_class) -> void {
  instruction(Op::Variable, 4);
  word(result_type_id);
  word(result_id);
  word(Bits_32(storage_class));
}

auto spirv::load(Bits_32 result_type_id, Bits_32 result_id, Bits_32 pointer_id)
    -> void {
  instruction(Op::Load, 4);
  word(result_type_id);
  word(result_id);
  word(pointer_id);
}

auto spirv::store(Bits_32 pointer_id, Bits_32 object_id) -> void {
  instruction(Op::Store, 3);
  word(pointer_id);
  word(object_id);
}

auto spirv::access_chain(
    Bits_32 result_type_id,
    Bits_32 result_id,
    Bits_32 base_id,
    View::Vector<Bits_32> index_ids) -> void {
  instruction(Op::AccessChain, 4 + index_ids.get_size());
  word(result_type_id);
  word(result_id);
  word(base_id);
  for (Count i = 0; i < index_ids.get_size(); i++) {
    word(index_ids[i]);
  }
}

auto spirv::vector_shuffle(
    Bits_32 result_type_id,
    Bits_32 result_id,
    Bits_32 vector_1_id,
    Bits_32 vector_2_id,
    View::Vector<Bits_32> components) -> void {
  instruction(Op::VectorShuffle, 5 + components.get_size());
  word(result_type_id);
  word(result_id);
  word(vector_1_id);
  word(vector_2_id);
  for (Count i = 0; i < components.get_size(); i++) {
    word(components[i]);
  }
}

auto spirv::composite_construct(
    Bits_32 result_type_id,
    Bits_32 result_id,
    View::Vector<Bits_32> constituents) -> void {
  instruction(Op::CompositeConstruct, 3 + constituents.get_size());
  word(result_type_id);
  word(result_id);
  for (Count i = 0; i < constituents.get_size(); i++) {
    word(constituents[i]);
  }
}

auto spirv::composite_extract(
    Bits_32 result_type_id,
    Bits_32 result_id,
    Bits_32 composite_id,
    View::Vector<Bits_32> indexes) -> void {
  instruction(Op::CompositeExtract, 4 + indexes.get_size());
  word(result_type_id);
  word(result_id);
  word(composite_id);
  for (Count i = 0; i < indexes.get_size(); i++) {
    word(indexes[i]);
  }
}

auto spirv::image_sample_implicit_lod(
    Bits_32 result_type_id,
    Bits_32 result_id,
    Bits_32 sampled_image_id,
    Bits_32 coordinate_id) -> void {
  instruction(Op::ImageSampleImplicitLod, 5);
  word(result_type_id);
  word(result_id);
  word(sampled_image_id);
  word(coordinate_id);
}

auto spirv::fadd(
    Bits_32 result_type_id,
    Bits_32 result_id,
    Bits_32 left_id,
    Bits_32 right_id) -> void {
  instruction(Op::FAdd, 5);
  word(result_type_id);
  word(result_id);
  word(left_id);
  word(right_id);
}

auto spirv::fsub(
    Bits_32 result_type_id,
    Bits_32 result_id,
    Bits_32 left_id,
    Bits_32 right_id) -> void {
  instruction(Op::FSub, 5);
  word(result_type_id);
  word(result_id);
  word(left_id);
  word(right_id);
}

auto spirv::fmul(
    Bits_32 result_type_id,
    Bits_32 result_id,
    Bits_32 left_id,
    Bits_32 right_id) -> void {
  instruction(Op::FMul, 5);
  word(result_type_id);
  word(result_id);
  word(left_id);
  word(right_id);
}

auto spirv::function(
    Bits_32 result_type_id,
    Bits_32 result_id,
    FunctionControl control,
    Bits_32 function_type_id) -> void {
  instruction(Op::Function, 5);
  word(result_type_id);
  word(result_id);
  word(Bits_32(control));
  word(function_type_id);
}

auto spirv::label(Bits_32 result_id) -> void {
  instruction(Op::Label, 2);
  word(result_id);
}

auto spirv::return_void() -> void {
  instruction(Op::Return, 1);
}

auto spirv::function_end() -> void {
  instruction(Op::FunctionEnd, 1);
}

auto spirv::literal_string_word_count(View::Bytes text) -> Count {
  return (text.get_size() + 4) / 4;
}

auto spirv::is_valid_module(View::Bytes words) -> Bool {
  if (words.get_size() < 20 || words.get_size() % 4 != 0) {
    return False;
  }

  if (read_word(words, 0) != magic) {
    return False;
  }

  Bits_32 version = read_word(words, 1);
  if ((version & 0x00FF0000) == 0) {
    return False;
  }

  if (read_word(words, 3) == 0 || read_word(words, 4) != 0) {
    return False;
  }

  Count word_count = words.get_size() / 4;
  for (Count offset = 5; offset < word_count;) {
    Bits_32 header = read_word(words, offset);
    Count instruction_words = Count(header >> 16);
    if (instruction_words == 0 || offset + instruction_words > word_count) {
      return False;
    }

    offset += instruction_words;
  }

  return True;
}
