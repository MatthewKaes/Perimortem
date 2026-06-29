// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Tetrodotoxin::Compiler::Assembler {

class spirv {
 public:
  enum class Version : Bits_32 {
    V1_0 = 0x00010000,
  };

  enum class Op : Bits_16 {
    Nop = 0,
    Undef = 1,
    SourceContinued = 2,
    Source = 3,
    SourceExtension = 4,
    Name = 5,
    MemberName = 6,
    String = 7,
    Line = 8,
    Extension = 10,
    ExtInstImport = 11,
    ExtInst = 12,
    MemoryModel = 14,
    EntryPoint = 15,
    ExecutionMode = 16,
    Capability = 17,
    TypeVoid = 19,
    TypeBool = 20,
    TypeInt = 21,
    TypeFloat = 22,
    TypeVector = 23,
    TypeImage = 25,
    TypeSampler = 26,
    TypeSampledImage = 27,
    TypeArray = 28,
    TypeStruct = 30,
    TypePointer = 32,
    TypeFunction = 33,
    Constant = 43,
    ConstantComposite = 44,
    Function = 54,
    FunctionEnd = 56,
    Variable = 59,
    Load = 61,
    Store = 62,
    AccessChain = 65,
    Decorate = 71,
    MemberDecorate = 72,
    VectorShuffle = 79,
    CompositeConstruct = 80,
    CompositeExtract = 81,
    ImageSampleImplicitLod = 87,
    FAdd = 129,
    FMul = 133,
    Label = 248,
    Return = 253,
  };

  enum class Capability : Bits_32 {
    Shader = 1,
  };

  enum class AddressingModel : Bits_32 {
    Logical = 0,
  };

  enum class MemoryModel : Bits_32 {
    GLSL450 = 1,
  };

  enum class ExecutionModel : Bits_32 {
    Vertex = 0,
    Fragment = 4,
  };

  enum class ExecutionMode : Bits_32 {
    OriginUpperLeft = 7,
  };

  enum class Dim : Bits_32 {
    D2 = 1,
  };

  enum class ImageFormat : Bits_32 {
    Unknown = 0,
  };

  enum class StorageClass : Bits_32 {
    UniformConstant = 0,
    Input = 1,
    Uniform = 2,
    Output = 3,
    Function = 7,
    PushConstant = 9,
  };

  enum class Decoration : Bits_32 {
    Block = 2,
    BuiltIn = 11,
    Location = 30,
    Binding = 33,
    DescriptorSet = 34,
    Offset = 35,
  };

  enum class BuiltIn : Bits_32 {
    Position = 0,
    VertexIndex = 42,
  };

  enum class FunctionControl : Bits_32 {
    None = 0,
  };

  static constexpr Bits_32 magic = 0x07230203;

  explicit spirv(Perimortem::Memory::Dynamic::Bytes& words) : words(words) {}

  auto begin_module(
      Bits_32 bound,
      Version version = Version::V1_0,
      Bits_32 generator = 0) -> void;
  auto word(Bits_32 value) -> void;
  auto instruction(Op op, Count word_count) -> void;
  auto literal_string(Perimortem::Core::View::Bytes text) -> Count;

  auto capability(Capability value) -> void;
  auto memory_model(AddressingModel addressing, MemoryModel memory) -> void;
  auto entry_point(
      ExecutionModel model,
      Bits_32 function_id,
      Perimortem::Core::View::Bytes name) -> void;
  auto entry_point(
      ExecutionModel model,
      Bits_32 function_id,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Vector<Bits_32> interface_ids) -> void;
  auto execution_mode(Bits_32 entry_point_id, ExecutionMode mode) -> void;
  auto name(Bits_32 target_id, Perimortem::Core::View::Bytes name) -> void;
  auto member_name(
      Bits_32 target_id,
      Bits_32 member_index,
      Perimortem::Core::View::Bytes name) -> void;
  auto decorate(Bits_32 target_id, Decoration decoration, Bits_32 value)
      -> void;
  auto decorate(Bits_32 target_id, Decoration decoration) -> void;
  auto member_decorate(
      Bits_32 target_id,
      Bits_32 member_index,
      Decoration decoration,
      Bits_32 value) -> void;
  auto type_void(Bits_32 result_id) -> void;
  auto type_bool(Bits_32 result_id) -> void;
  auto type_int(Bits_32 result_id, Bits_32 width, Bool signedness) -> void;
  auto type_float(Bits_32 result_id, Bits_32 width) -> void;
  auto type_vector(
      Bits_32 result_id,
      Bits_32 component_type_id,
      Bits_32 component_count) -> void;
  auto type_image(
      Bits_32 result_id,
      Bits_32 sampled_type_id,
      Dim dim,
      Bits_32 depth,
      Bits_32 arrayed,
      Bits_32 multisampled,
      Bits_32 sampled,
      ImageFormat format) -> void;
  auto type_sampler(Bits_32 result_id) -> void;
  auto type_sampled_image(Bits_32 result_id, Bits_32 image_type_id) -> void;
  auto type_array(Bits_32 result_id, Bits_32 element_type_id, Bits_32 length_id)
      -> void;
  auto type_struct(
      Bits_32 result_id,
      Perimortem::Core::View::Vector<Bits_32> member_type_ids) -> void;
  auto type_pointer(
      Bits_32 result_id,
      StorageClass storage_class,
      Bits_32 type_id) -> void;
  auto type_function(Bits_32 result_id, Bits_32 return_type_id) -> void;
  auto constant(Bits_32 result_type_id, Bits_32 result_id, Bits_32 value)
      -> void;
  auto constant_composite(
      Bits_32 result_type_id,
      Bits_32 result_id,
      Perimortem::Core::View::Vector<Bits_32> constituents) -> void;
  auto variable(
      Bits_32 result_type_id,
      Bits_32 result_id,
      StorageClass storage_class) -> void;
  auto load(Bits_32 result_type_id, Bits_32 result_id, Bits_32 pointer_id)
      -> void;
  auto store(Bits_32 pointer_id, Bits_32 object_id) -> void;
  auto access_chain(
      Bits_32 result_type_id,
      Bits_32 result_id,
      Bits_32 base_id,
      Perimortem::Core::View::Vector<Bits_32> index_ids) -> void;
  auto vector_shuffle(
      Bits_32 result_type_id,
      Bits_32 result_id,
      Bits_32 vector_1_id,
      Bits_32 vector_2_id,
      Perimortem::Core::View::Vector<Bits_32> components) -> void;
  auto composite_construct(
      Bits_32 result_type_id,
      Bits_32 result_id,
      Perimortem::Core::View::Vector<Bits_32> constituents) -> void;
  auto composite_extract(
      Bits_32 result_type_id,
      Bits_32 result_id,
      Bits_32 composite_id,
      Perimortem::Core::View::Vector<Bits_32> indexes) -> void;
  auto image_sample_implicit_lod(
      Bits_32 result_type_id,
      Bits_32 result_id,
      Bits_32 sampled_image_id,
      Bits_32 coordinate_id) -> void;
  auto fadd(
      Bits_32 result_type_id,
      Bits_32 result_id,
      Bits_32 left_id,
      Bits_32 right_id) -> void;
  auto fmul(
      Bits_32 result_type_id,
      Bits_32 result_id,
      Bits_32 left_id,
      Bits_32 right_id) -> void;
  auto function(
      Bits_32 result_type_id,
      Bits_32 result_id,
      FunctionControl control,
      Bits_32 function_type_id) -> void;
  auto label(Bits_32 result_id) -> void;
  auto return_void() -> void;
  auto function_end() -> void;

  static auto literal_string_word_count(Perimortem::Core::View::Bytes text)
      -> Count;
  static auto is_valid_module(Perimortem::Core::View::Bytes words) -> Bool;

 private:
  Perimortem::Memory::Dynamic::Bytes& words;
};

}  // namespace Tetrodotoxin::Compiler::Assembler
