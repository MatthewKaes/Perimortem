// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/shader/assembler/spir_v.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Shader;

// Validation reads the same little endian word stream the writer produces. This
// stays local because it is only a structural sanity check, not a public
// reader.
static auto read_word(View::Bytes words, Count word_index) -> Unsigned_32 {
  Count byte_index = word_index * 4;
  if (byte_index + 4 > words.get_size()) {
    return 0;
  }

  return Unsigned_32(words[byte_index]) |
         (Unsigned_32(words[byte_index + 1]) << 8) |
         (Unsigned_32(words[byte_index + 2]) << 16) |
         (Unsigned_32(words[byte_index + 3]) << 24);
}

auto Assembler::SpirV::begin_module(
    Unsigned_32 bound,
    Version version,
    Unsigned_32 generator) -> void {
  // The fifth header word is the reserved schema field. It must be zero for the
  // current SPIR V versions.
  word(magic);
  word(Unsigned_32(version));
  word(generator);
  word(bound);
  word(0);
}

auto Assembler::SpirV::word(Unsigned_32 value) -> void {
  // SPIR V binary modules are little endian 32 bit words. Keeping this
  // primitive explicit makes every higher level helper a direct spelling of the
  // wire form.
  words.append(Unsigned_8(value & 0xFF));
  words.append(Unsigned_8((value >> 8) & 0xFF));
  words.append(Unsigned_8((value >> 16) & 0xFF));
  words.append(Unsigned_8((value >> 24) & 0xFF));
}

auto Assembler::SpirV::instruction(Op opcode, Count word_count) -> void {
  // Every instruction begins with one word. The opcode occupies the low 16
  // bits. The total instruction word count occupies the high 16 bits and
  // includes this header word.
  word((Unsigned_32(word_count) << 16) | Unsigned_32(opcode));
}

auto Assembler::SpirV::literal_string(View::Bytes text) -> Count {
  // Literal strings are stored inline as words. The first zero byte terminates
  // the string, and any remaining bytes in that final word are also zero.
  Count byte_index = 0;
  Count written = 0;
  Count words = literal_string_word_count(text);
  for (Count i = 0; i < words; i++) {
    Unsigned_32 packed = 0;
    for (Count j = 0; j < 4; j++) {
      Unsigned_8 byte = 0;
      if (byte_index < text.get_size()) {
        byte = text[byte_index];
      }

      packed |= Unsigned_32(byte) << (j * 8);
      byte_index++;
    }

    word(packed);
    written++;
  }

  return written;
}

auto Assembler::SpirV::capability(Capability value) -> void {
  // OpCapability opts the module into a feature family. The Shader capability
  // is the baseline required before declaring shader stages.
  instruction(Op::Capability, 2);
  word(Unsigned_32(value));
}

auto Assembler::SpirV::memory_model(
    AddressingModel addressing,
    MemoryModel memory) -> void {
  // OpMemoryModel is mandatory and fixes the pointer/addressing rules the rest
  // of the module is interpreted under.
  instruction(Op::MemoryModel, 3);
  word(Unsigned_32(addressing));
  word(Unsigned_32(memory));
}

auto Assembler::SpirV::entry_point(
    ExecutionModel model,
    Unsigned_32 function_id,
    View::Bytes name) -> void {
  entry_point(model, function_id, name, View::Vector<Unsigned_32>());
}

auto Assembler::SpirV::entry_point(
    ExecutionModel model,
    Unsigned_32 function_id,
    View::Bytes name,
    View::Vector<Unsigned_32> interface_ids) -> void {
  // OpEntryPoint binds an execution model to a function id and lists the global
  // input/output variables visible at that boundary.
  instruction(
      Op::EntryPoint,
      3 + literal_string_word_count(name) + interface_ids.get_size());
  word(Unsigned_32(model));
  word(function_id);
  literal_string(name);
  for (Count i = 0; i < interface_ids.get_size(); i++) {
    word(interface_ids.get_data()[i]);
  }
}

auto Assembler::SpirV::execution_mode(
    Unsigned_32 entry_point_id,
    ExecutionMode mode) -> void {
  // OpExecutionMode adds stage specific facts. Fragment shaders commonly need
  // OriginUpperLeft so coordinates match the Vulkan framebuffer convention.
  instruction(Op::ExecutionMode, 3);
  word(entry_point_id);
  word(Unsigned_32(mode));
}

auto Assembler::SpirV::name(Unsigned_32 target_id, View::Bytes name) -> void {
  // OpName is debug metadata. It can legally reference an id before the
  // instruction that defines that id appears later in the module.
  instruction(Op::Name, 2 + literal_string_word_count(name));
  word(target_id);
  literal_string(name);
}

auto Assembler::SpirV::member_name(
    Unsigned_32 target_id,
    Unsigned_32 member_index,
    View::Bytes name) -> void {
  // OpMemberName is the struct member version of OpName. The member is
  // addressed by index because struct fields are positional in SPIR V.
  instruction(Op::MemberName, 3 + literal_string_word_count(name));
  word(target_id);
  word(member_index);
  literal_string(name);
}

auto Assembler::SpirV::decorate(
    Unsigned_32 target_id,
    Decoration decoration,
    Unsigned_32 value) -> void {
  // OpDecorate attaches semantic metadata to an id. Decorations are how Vulkan
  // sees locations, descriptor sets, bindings, and builtin IO roles.
  instruction(Op::Decorate, 4);
  word(target_id);
  word(Unsigned_32(decoration));
  word(value);
}

auto Assembler::SpirV::decorate(Unsigned_32 target_id, Decoration decoration)
    -> void {
  instruction(Op::Decorate, 3);
  word(target_id);
  word(Unsigned_32(decoration));
}

auto Assembler::SpirV::member_decorate(
    Unsigned_32 target_id,
    Unsigned_32 member_index,
    Decoration decoration,
    Unsigned_32 value) -> void {
  // OpMemberDecorate is used for layout metadata on a single struct member,
  // such as the byte offset inside a push constant block.
  instruction(Op::MemberDecorate, 5);
  word(target_id);
  word(member_index);
  word(Unsigned_32(decoration));
  word(value);
}

auto Assembler::SpirV::type_void(Unsigned_32 result_id) -> void {
  // Type instructions define ids in the type namespace. Later instructions
  // refer to these ids instead of restating the structural type.
  instruction(Op::TypeVoid, 2);
  word(result_id);
}

auto Assembler::SpirV::type_bool(Unsigned_32 result_id) -> void {
  instruction(Op::TypeBool, 2);
  word(result_id);
}

auto Assembler::SpirV::type_int(
    Unsigned_32 result_id,
    Unsigned_32 width,
    Bool signedness) -> void {
  instruction(Op::TypeInt, 4);
  word(result_id);
  word(width);
  word(signedness ? 1 : 0);
}

auto Assembler::SpirV::type_float(Unsigned_32 result_id, Unsigned_32 width)
    -> void {
  instruction(Op::TypeFloat, 3);
  word(result_id);
  word(width);
}

auto Assembler::SpirV::type_vector(
    Unsigned_32 result_id,
    Unsigned_32 component_type_id,
    Unsigned_32 component_count) -> void {
  instruction(Op::TypeVector, 4);
  word(result_id);
  word(component_type_id);
  word(component_count);
}

auto Assembler::SpirV::type_image(
    Unsigned_32 result_id,
    Unsigned_32 sampled_type_id,
    Dim dim,
    Unsigned_32 depth,
    Unsigned_32 arrayed,
    Unsigned_32 multisampled,
    Unsigned_32 sampled,
    ImageFormat format) -> void {
  instruction(Op::TypeImage, 9);
  word(result_id);
  word(sampled_type_id);
  word(Unsigned_32(dim));
  word(depth);
  word(arrayed);
  word(multisampled);
  word(sampled);
  word(Unsigned_32(format));
}

auto Assembler::SpirV::type_sampler(Unsigned_32 result_id) -> void {
  instruction(Op::TypeSampler, 2);
  word(result_id);
}

auto Assembler::SpirV::type_sampled_image(
    Unsigned_32 result_id,
    Unsigned_32 image_type_id) -> void {
  instruction(Op::TypeSampledImage, 3);
  word(result_id);
  word(image_type_id);
}

auto Assembler::SpirV::type_array(
    Unsigned_32 result_id,
    Unsigned_32 element_type_id,
    Unsigned_32 length_id) -> void {
  instruction(Op::TypeArray, 4);
  word(result_id);
  word(element_type_id);
  word(length_id);
}

auto Assembler::SpirV::type_struct(
    Unsigned_32 result_id,
    View::Vector<Unsigned_32> member_type_ids) -> void {
  instruction(Op::TypeStruct, 2 + member_type_ids.get_size());
  word(result_id);
  for (Count i = 0; i < member_type_ids.get_size(); i++) {
    word(member_type_ids.get_data()[i]);
  }
}

auto Assembler::SpirV::type_pointer(
    Unsigned_32 result_id,
    StorageClass storage_class,
    Unsigned_32 type_id) -> void {
  // Pointer types include their storage class, so "pointer to Vec2 input" and
  // "pointer to Vec2 output" are distinct SPIR V types.
  instruction(Op::TypePointer, 4);
  word(result_id);
  word(Unsigned_32(storage_class));
  word(type_id);
}

auto Assembler::SpirV::type_function(
    Unsigned_32 result_id,
    Unsigned_32 return_type_id) -> void {
  instruction(Op::TypeFunction, 3);
  word(result_id);
  word(return_type_id);
}

auto Assembler::SpirV::constant(
    Unsigned_32 result_type_id,
    Unsigned_32 result_id,
    Unsigned_32 value) -> void {
  instruction(Op::Constant, 4);
  word(result_type_id);
  word(result_id);
  word(value);
}

auto Assembler::SpirV::constant_composite(
    Unsigned_32 result_type_id,
    Unsigned_32 result_id,
    View::Vector<Unsigned_32> constituents) -> void {
  instruction(Op::ConstantComposite, 3 + constituents.get_size());
  word(result_type_id);
  word(result_id);
  for (Count i = 0; i < constituents.get_size(); i++) {
    word(constituents.get_data()[i]);
  }
}

auto Assembler::SpirV::variable(
    Unsigned_32 result_type_id,
    Unsigned_32 result_id,
    StorageClass storage_class) -> void {
  // Variables are storage declarations. For shader inputs/outputs/resources
  // they are module scope globals. Function storage variables will use Function
  // storage.
  instruction(Op::Variable, 4);
  word(result_type_id);
  word(result_id);
  word(Unsigned_32(storage_class));
}

auto Assembler::SpirV::load(
    Unsigned_32 result_type_id,
    Unsigned_32 result_id,
    Unsigned_32 pointer_id) -> void {
  // OpLoad turns a pointer id into an SSA value id. The result type is the
  // pointee value type, not the pointer type.
  instruction(Op::Load, 4);
  word(result_type_id);
  word(result_id);
  word(pointer_id);
}

auto Assembler::SpirV::store(Unsigned_32 pointer_id, Unsigned_32 object_id)
    -> void {
  // OpStore writes an SSA value into a pointer. It has no result id.
  instruction(Op::Store, 3);
  word(pointer_id);
  word(object_id);
}

auto Assembler::SpirV::access_chain(
    Unsigned_32 result_type_id,
    Unsigned_32 result_id,
    Unsigned_32 base_id,
    View::Vector<Unsigned_32> index_ids) -> void {
  // OpAccessChain computes a pointer into a composite object. Index ids are SSA
  // integer values, which lets later lowering address dynamic array elements.
  instruction(Op::AccessChain, 4 + index_ids.get_size());
  word(result_type_id);
  word(result_id);
  word(base_id);
  for (Count i = 0; i < index_ids.get_size(); i++) {
    word(index_ids.get_data()[i]);
  }
}

auto Assembler::SpirV::vector_shuffle(
    Unsigned_32 result_type_id,
    Unsigned_32 result_id,
    Unsigned_32 vector_1_id,
    Unsigned_32 vector_2_id,
    View::Vector<Unsigned_32> components) -> void {
  // OpVectorShuffle builds a new vector by selecting lanes from one or two
  // input vectors. It is the natural target for swizzles.
  instruction(Op::VectorShuffle, 5 + components.get_size());
  word(result_type_id);
  word(result_id);
  word(vector_1_id);
  word(vector_2_id);
  for (Count i = 0; i < components.get_size(); i++) {
    word(components.get_data()[i]);
  }
}

auto Assembler::SpirV::composite_construct(
    Unsigned_32 result_type_id,
    Unsigned_32 result_id,
    View::Vector<Unsigned_32> constituents) -> void {
  // OpCompositeConstruct creates vectors, arrays, and structs from already
  // computed constituent value ids.
  instruction(Op::CompositeConstruct, 3 + constituents.get_size());
  word(result_type_id);
  word(result_id);
  for (Count i = 0; i < constituents.get_size(); i++) {
    word(constituents.get_data()[i]);
  }
}

auto Assembler::SpirV::composite_extract(
    Unsigned_32 result_type_id,
    Unsigned_32 result_id,
    Unsigned_32 composite_id,
    View::Vector<Unsigned_32> indexes) -> void {
  // OpCompositeExtract reads a value out of a composite without producing a
  // pointer. For a pointer result, use OpAccessChain instead.
  instruction(Op::CompositeExtract, 4 + indexes.get_size());
  word(result_type_id);
  word(result_id);
  word(composite_id);
  for (Count i = 0; i < indexes.get_size(); i++) {
    word(indexes.get_data()[i]);
  }
}

auto Assembler::SpirV::image_sample_implicit_lod(
    Unsigned_32 result_type_id,
    Unsigned_32 result_id,
    Unsigned_32 sampled_image_id,
    Unsigned_32 coordinate_id) -> void {
  // OpImageSampleImplicitLod samples an image using implicit derivatives rather
  // than an explicit LOD operand, which is enough for the first 2D fragment
  // path.
  instruction(Op::ImageSampleImplicitLod, 5);
  word(result_type_id);
  word(result_id);
  word(sampled_image_id);
  word(coordinate_id);
}

auto Assembler::SpirV::fadd(
    Unsigned_32 result_type_id,
    Unsigned_32 result_id,
    Unsigned_32 left_id,
    Unsigned_32 right_id) -> void {
  instruction(Op::FAdd, 5);
  word(result_type_id);
  word(result_id);
  word(left_id);
  word(right_id);
}

auto Assembler::SpirV::fsub(
    Unsigned_32 result_type_id,
    Unsigned_32 result_id,
    Unsigned_32 left_id,
    Unsigned_32 right_id) -> void {
  instruction(Op::FSub, 5);
  word(result_type_id);
  word(result_id);
  word(left_id);
  word(right_id);
}

auto Assembler::SpirV::fmul(
    Unsigned_32 result_type_id,
    Unsigned_32 result_id,
    Unsigned_32 left_id,
    Unsigned_32 right_id) -> void {
  instruction(Op::FMul, 5);
  word(result_type_id);
  word(result_id);
  word(left_id);
  word(right_id);
}

auto Assembler::SpirV::function(
    Unsigned_32 result_type_id,
    Unsigned_32 result_id,
    FunctionControl control,
    Unsigned_32 function_type_id) -> void {
  // OpFunction opens a function body. The function type id names the signature.
  // The result type id repeats the return type for quick validation.
  instruction(Op::Function, 5);
  word(result_type_id);
  word(result_id);
  word(Unsigned_32(control));
  word(function_type_id);
}

auto Assembler::SpirV::label(Unsigned_32 result_id) -> void {
  // A label begins a basic block. Even a function with no real body needs one
  // block before it can return.
  instruction(Op::Label, 2);
  word(result_id);
}

auto Assembler::SpirV::return_void() -> void {
  instruction(Op::Return, 1);
}

auto Assembler::SpirV::function_end() -> void {
  instruction(Op::FunctionEnd, 1);
}

auto Assembler::SpirV::literal_string_word_count(View::Bytes text) -> Count {
  // Add one byte for the NUL terminator, then round up to a whole word.
  return (text.get_size() + 4) / 4;
}

auto Assembler::SpirV::is_valid_module(View::Bytes words) -> Bool {
  // This is intentionally shallow. It catches broken writers and truncated
  // modules without pretending to be a SPIR V semantic validator.
  if (words.get_size() < 20 || words.get_size() % 4 != 0) {
    return False;
  }

  if (read_word(words, 0) != magic) {
    return False;
  }

  Unsigned_32 version = read_word(words, 1);
  if ((version & 0x00FF0000) == 0) {
    return False;
  }

  if (read_word(words, 3) == 0 || read_word(words, 4) != 0) {
    return False;
  }

  Count word_count = words.get_size() / 4;
  for (Count offset = 5; offset < word_count;) {
    Unsigned_32 header = read_word(words, offset);
    Count instruction_words = Count(header >> 16);
    if (instruction_words == 0 || offset + instruction_words > word_count) {
      return False;
    }

    offset += instruction_words;
  }

  return True;
}
