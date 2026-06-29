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

PERIMORTEM_UNIT_TEST(TtxShaderCompiler, emits_icon_v1_spirv_modules) {
  Resolution::Resolver resolver;
  ASSERT(resolver.resolve("apps/shaders/icon.ttx"_view));

  Resolution::Record* record = resolver.find("apps/shaders/icon.ttx"_view);
  ASSERT(record);

  Perimortem::Memory::Dynamic::Bytes vertex;
  Perimortem::Memory::Dynamic::Bytes pixel;
  ASSERT(Compiler::Shader::emit_v1_modules(*record, vertex, pixel));

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
  EXPECT_NOT(Algorithm::search(pixel, "icon_texture"_view) == Count(-1));

  constexpr Static::Bytes<16> descriptor_set_zero = {
    0x47, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00,
    0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  constexpr Static::Bytes<16> descriptor_binding_zero = {
    0x47, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00,
    0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  constexpr Static::Bytes<16> sampled_image_variable = {
    0x3b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00,
    0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  EXPECT(Algorithm::search(vertex, descriptor_set_zero) == Count(-1));
  EXPECT(Algorithm::search(vertex, descriptor_binding_zero) == Count(-1));
  EXPECT(Algorithm::search(vertex, sampled_image_variable) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, descriptor_set_zero) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, descriptor_binding_zero) == Count(-1));
  EXPECT_NOT(Algorithm::search(pixel, sampled_image_variable) == Count(-1));
}
