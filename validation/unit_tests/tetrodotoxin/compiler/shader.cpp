// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/compiler/assembler/spirv.hpp"
#include "tetrodotoxin/compiler/shader/spirv.hpp"
#include "tetrodotoxin/resolution/resolver.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness TtxShaderCompiler = {
  .name = "Tetrodotoxin::Compiler::Shader"_view,
};

static auto read_spirv_word(View::Bytes source, Count word_index) -> Bits_32 {
  const Count byte_index = word_index * 4;
  return Bits_32(source[byte_index]) |
         (Bits_32(source[byte_index + 1]) << 8) |
         (Bits_32(source[byte_index + 2]) << 16) |
         (Bits_32(source[byte_index + 3]) << 24);
}

static auto has_vector_shuffle_components(
    View::Bytes module,
    Bits_32 first_component,
    Bits_32 second_component,
    Bits_32 third_component) -> Bool {
  const Count word_count = module.get_size() / 4;
  Count word_index = 5;

  while (word_index < word_count) {
    const Bits_32 instruction = read_spirv_word(module, word_index);
    const Bits_32 opcode = instruction & 0xFFFF;
    const Count instruction_word_count = Count(instruction >> 16);
    if (instruction_word_count == 0 ||
        word_index + instruction_word_count > word_count) {
      return False;
    }

    if (opcode == Bits_32(Compiler::Assembler::spirv::Op::VectorShuffle) &&
        instruction_word_count == 8 &&
        read_spirv_word(module, word_index + 5) == first_component &&
        read_spirv_word(module, word_index + 6) == second_component &&
        read_spirv_word(module, word_index + 7) == third_component) {
      return True;
    }

    word_index += instruction_word_count;
  }

  return False;
}

static auto has_decoration(
    View::Bytes module,
    Bits_32 target_id,
    Compiler::Assembler::spirv::Decoration decoration,
    Bits_32 value) -> Bool {
  const Count word_count = module.get_size() / 4;
  Count word_index = 5;

  while (word_index < word_count) {
    const Bits_32 instruction = read_spirv_word(module, word_index);
    const Bits_32 opcode = instruction & 0xFFFF;
    const Count instruction_word_count = Count(instruction >> 16);
    if (instruction_word_count == 0 ||
        word_index + instruction_word_count > word_count) {
      return False;
    }

    if (opcode == Bits_32(Compiler::Assembler::spirv::Op::Decorate) &&
        instruction_word_count == 4 &&
        read_spirv_word(module, word_index + 1) == target_id &&
        read_spirv_word(module, word_index + 2) == Bits_32(decoration) &&
        read_spirv_word(module, word_index + 3) == value) {
      return True;
    }

    word_index += instruction_word_count;
  }

  return False;
}

static auto has_variable_storage(
    View::Bytes module,
    Bits_32 target_id,
    Compiler::Assembler::spirv::StorageClass storage) -> Bool {
  const Count word_count = module.get_size() / 4;
  Count word_index = 5;

  while (word_index < word_count) {
    const Bits_32 instruction = read_spirv_word(module, word_index);
    const Bits_32 opcode = instruction & 0xFFFF;
    const Count instruction_word_count = Count(instruction >> 16);
    if (instruction_word_count == 0 ||
        word_index + instruction_word_count > word_count) {
      return False;
    }

    if (opcode == Bits_32(Compiler::Assembler::spirv::Op::Variable) &&
        instruction_word_count == 4 &&
        read_spirv_word(module, word_index + 2) == target_id &&
        read_spirv_word(module, word_index + 3) == Bits_32(storage)) {
      return True;
    }

    word_index += instruction_word_count;
  }

  return False;
}

static auto has_descriptor_binding(
    View::Bytes module,
    Bits_32 descriptor_set,
    Bits_32 binding_slot) -> Bool {
  const Count word_count = module.get_size() / 4;
  Count word_index = 5;

  while (word_index < word_count) {
    const Bits_32 instruction = read_spirv_word(module, word_index);
    const Bits_32 opcode = instruction & 0xFFFF;
    const Count instruction_word_count = Count(instruction >> 16);
    if (instruction_word_count == 0 ||
        word_index + instruction_word_count > word_count) {
      return False;
    }

    if (opcode == Bits_32(Compiler::Assembler::spirv::Op::Decorate) &&
        instruction_word_count == 4 &&
        read_spirv_word(module, word_index + 2) ==
            Bits_32(Compiler::Assembler::spirv::Decoration::DescriptorSet) &&
        read_spirv_word(module, word_index + 3) == descriptor_set) {
      const Bits_32 target_id = read_spirv_word(module, word_index + 1);
      if (has_decoration(
              module, target_id,
              Compiler::Assembler::spirv::Decoration::Binding,
              binding_slot) &&
          has_variable_storage(
              module, target_id,
              Compiler::Assembler::spirv::StorageClass::UniformConstant)) {
        return True;
      }
    }

    word_index += instruction_word_count;
  }

  return False;
}

PERIMORTEM_UNIT_TEST(TtxShaderCompiler, default_2d_spirv_modules) {
  Resolution::Resolver resolver;
  ASSERT(resolver.resolve("apps/shaders/default_2d.ttx"_view));

  Resolution::Record* record =
      resolver.find("apps/shaders/default_2d.ttx"_view);
  ASSERT(record);

  Perimortem::Memory::Dynamic::Bytes vertex;
  Perimortem::Memory::Dynamic::Bytes pixel;
  ASSERT(Compiler::Shader::emit_program_modules(*record, vertex, pixel));

  EXPECT(Compiler::Assembler::spirv::is_valid_module(vertex));
  EXPECT(Compiler::Assembler::spirv::is_valid_module(pixel));
  EXPECT_NOT(Algorithm::search(vertex, "main"_view) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, "main"_view) == Count(-1));

  constexpr Static::Bytes<12> origin_upper_left = {
    0x10, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
  };
  constexpr Static::Bytes<4> vertex_entry_interface = {
    0x0f,
    0x00,
    0x08,
    0x00,
  };
  constexpr Static::Bytes<4> pixel_entry_interface = {
    0x0f,
    0x00,
    0x07,
    0x00,
  };
  EXPECT(Algorithm::search(vertex, origin_upper_left) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, origin_upper_left) == Count(-1));
  EXPECT_NOT(Algorithm::search(vertex, vertex_entry_interface) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, pixel_entry_interface) == Count(-1));

  constexpr Static::Bytes<4> op_function = {
    0x36,
    0x00,
    0x05,
    0x00,
  };
  constexpr Static::Bytes<4> op_return = {
    0xfd,
    0x00,
    0x01,
    0x00,
  };
  constexpr Static::Bytes<4> op_function_end = {
    0x38,
    0x00,
    0x01,
    0x00,
  };
  constexpr Static::Bytes<4> op_type_int = {
    0x15,
    0x00,
    0x04,
    0x00,
  };
  constexpr Static::Bytes<4> op_type_float = {
    0x16,
    0x00,
    0x03,
    0x00,
  };
  constexpr Static::Bytes<4> op_type_vector = {
    0x17,
    0x00,
    0x04,
    0x00,
  };
  constexpr Static::Bytes<4> op_type_image = {
    0x19,
    0x00,
    0x09,
    0x00,
  };
  constexpr Static::Bytes<4> op_type_sampled_image = {
    0x1b,
    0x00,
    0x03,
    0x00,
  };
  constexpr Static::Bytes<4> op_load = {
    0x3d,
    0x00,
    0x04,
    0x00,
  };
  constexpr Static::Bytes<4> op_store = {
    0x3e,
    0x00,
    0x03,
    0x00,
  };
  constexpr Static::Bytes<4> op_access_chain = {
    0x41,
    0x00,
    0x05,
    0x00,
  };
  constexpr Static::Bytes<4> op_vector_shuffle = {
    0x4f,
    0x00,
    0x08,
    0x00,
  };
  constexpr Static::Bytes<4> op_composite_construct_vec4 = {
    0x50,
    0x00,
    0x07,
    0x00,
  };
  constexpr Static::Bytes<4> op_composite_extract_scalar = {
    0x51,
    0x00,
    0x05,
    0x00,
  };
  constexpr Static::Bytes<4> op_image_sample_implicit_lod = {
    0x57,
    0x00,
    0x05,
    0x00,
  };
  constexpr Static::Bytes<4> op_fadd = {
    0x81,
    0x00,
    0x05,
    0x00,
  };
  constexpr Static::Bytes<4> op_fmul = {
    0x85,
    0x00,
    0x05,
    0x00,
  };
  EXPECT_NOT(Algorithm::search(vertex, op_function) == Count(-1));
  EXPECT_NOT(Algorithm::search(vertex, op_return) == Count(-1));
  EXPECT_NOT(Algorithm::search(vertex, op_function_end) == Count(-1));
  EXPECT_NOT(Algorithm::search(vertex, op_type_int) == Count(-1));
  EXPECT_NOT(Algorithm::search(vertex, op_type_float) == Count(-1));
  EXPECT_NOT(Algorithm::search(vertex, op_type_vector) == Count(-1));
  EXPECT(Algorithm::search(vertex, op_type_image) == Count(-1));
  EXPECT(Algorithm::search(vertex, op_type_sampled_image) == Count(-1));
  EXPECT_NOT(Algorithm::search(vertex, op_load) == Count(-1));
  EXPECT_NOT(Algorithm::search(vertex, op_store) == Count(-1));
  EXPECT_NOT(Algorithm::search(vertex, op_access_chain) == Count(-1));
  EXPECT_NOT(
      Algorithm::search(vertex, op_composite_construct_vec4) == Count(-1));
  EXPECT_NOT(
      Algorithm::search(vertex, op_composite_extract_scalar) == Count(-1));
  EXPECT_NOT(Algorithm::search(vertex, op_fadd) == Count(-1));
  EXPECT_NOT(Algorithm::search(vertex, op_fmul) == Count(-1));
  EXPECT_NOT(Algorithm::search(vertex, "quad_positions"_view) == Count(-1));
  EXPECT_NOT(Algorithm::search(vertex, "quad_uvs"_view) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, op_function) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, op_return) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, op_function_end) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, op_type_int) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, op_type_float) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, op_type_vector) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, op_type_image) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, op_type_sampled_image) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, op_load) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, op_store) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, op_access_chain) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, op_vector_shuffle) == Count(-1));
  EXPECT_NOT(
      Algorithm::search(pixel, op_composite_construct_vec4) == Count(-1));
  EXPECT_NOT(
      Algorithm::search(pixel, op_composite_extract_scalar) == Count(-1));
  EXPECT_NOT(
      Algorithm::search(pixel, op_image_sample_implicit_lod) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, op_fmul) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, "image"_view) == Count(-1));

  EXPECT_NOT(has_descriptor_binding(vertex, 0, 0));
  EXPECT(has_descriptor_binding(pixel, 0, 0));
}

PERIMORTEM_UNIT_TEST(TtxShaderCompiler, pixel_swizzle_output) {
  Resolution::Resolver resolver;
  ASSERT(resolver.resolve("validation/data/ttx/shaders/channel_swap.ttx"_view));

  Resolution::Record* record =
      resolver.find("validation/data/ttx/shaders/channel_swap.ttx"_view);
  ASSERT(record);

  Perimortem::Memory::Dynamic::Bytes vertex;
  Perimortem::Memory::Dynamic::Bytes pixel;
  ASSERT(Compiler::Shader::emit_program_modules(*record, vertex, pixel));

  EXPECT(has_vector_shuffle_components(pixel, 1, 0, 2));
  EXPECT_NOT(has_vector_shuffle_components(pixel, 0, 1, 2));
}

PERIMORTEM_UNIT_TEST(TtxShaderCompiler, renamed_host_input) {
  Resolution::Resolver resolver;
  ASSERT(resolver.resolve("validation/data/ttx/shaders/renamed_values.ttx"_view));

  Resolution::Record* record =
      resolver.find("validation/data/ttx/shaders/renamed_values.ttx"_view);
  ASSERT(record);

  Perimortem::Memory::Dynamic::Bytes vertex;
  Perimortem::Memory::Dynamic::Bytes pixel;
  ASSERT(Compiler::Shader::emit_program_modules(*record, vertex, pixel));

  EXPECT_NOT(Algorithm::search(vertex, "center"_view) == Count(-1));
  EXPECT_NOT(Algorithm::search(vertex, "extent"_view) == Count(-1));
  EXPECT_NOT(Algorithm::search(vertex, "visibility"_view) == Count(-1));
  EXPECT_NOT(Algorithm::search(vertex, "width"_view) == Count(-1));
  EXPECT_NOT(Algorithm::search(vertex, "height"_view) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, "center"_view) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, "extent"_view) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, "visibility"_view) == Count(-1));
  EXPECT(has_vector_shuffle_components(pixel, 1, 0, 2));
  EXPECT(Algorithm::search(pixel, "position"_view) == Count(-1));
  EXPECT(Algorithm::search(pixel, "size_pixels"_view) == Count(-1));
  EXPECT(Algorithm::search(pixel, "alpha"_view) == Count(-1));
}
