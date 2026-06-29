// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/compiler/assembler/spirv.hpp"
#include "tetrodotoxin/compiler/compiler.hpp"
#include "tetrodotoxin/compiler/shader/spirv.hpp"
#include "tetrodotoxin/compiler/shader/terminal.hpp"
#include "tetrodotoxin/linker/linker.hpp"
#include "tetrodotoxin/resolution/resolver.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness TtxLinker = {
  .name = "Tetrodotoxin::Linker"_view,
};

PERIMORTEM_UNIT_TEST(TtxLinker, emits_read_only_shader_archive_symbols) {
  Resolution::Resolver resolver;
  ASSERT(resolver.resolve("apps/shaders/icon.ttx"_view));

  Resolution::Record* record = resolver.find("apps/shaders/icon.ttx"_view);
  ASSERT(record);

  Compiler::Compiler compiler;
  Perimortem::Memory::Dynamic::Bytes vertex_words;
  Perimortem::Memory::Dynamic::Bytes pixel_words;
  ASSERT(Compiler::Shader::emit_v1_modules(*record, vertex_words, pixel_words));
  ASSERT(Compiler::Assembler::spirv::is_valid_module(vertex_words));
  ASSERT(Compiler::Assembler::spirv::is_valid_module(pixel_words));

  Compiler::Shader::add_v1_terminal_symbols(
      compiler, *record,
      {
        "icon"_view,
        vertex_words,
        pixel_words,
      });

  const auto archive =
      Linker::emit_archive(compiler, "icon_shader_terminal.o"_view);

  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "!<arch>\n"_view) == Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "icon_shader_ter"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), ".rodata"_view) == Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "icon_vert_spv"_view) == Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "icon_vert_spv_size"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "icon_frag_spv"_view) == Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "icon_frag_spv_size"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "icon_push_constants"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "icon_push_constants_size"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "icon_descriptors"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "icon_descriptors_size"_view) ==
      Count(-1));
  EXPECT_NOT(Algorithm::search(archive.get_view(), vertex_words) == Count(-1));
  EXPECT_NOT(Algorithm::search(archive.get_view(), pixel_words) == Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          archive.get_view(), "TTX_SHADER_PUSH_CONSTANTS_V1"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "block name=Push size=24"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          archive.get_view(), "field name=position type=Vec2D"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          archive.get_view(), "field name=alpha type=Real_32"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "TTX_SHADER_DESCRIPTORS_V1"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          archive.get_view(),
          "descriptor name=icon_texture type=Sampler_2D set=0 slot=0"_view) ==
      Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLinker, emits_shader_program_header_metadata) {
  Resolution::Resolver resolver;
  ASSERT(resolver.resolve("apps/shaders/icon.ttx"_view));

  Resolution::Record* record = resolver.find("apps/shaders/icon.ttx"_view);
  ASSERT(record);

  Perimortem::Memory::Dynamic::Bytes header;
  Compiler::Shader::write_v1_terminal_header(*record, "icon"_view, header);

  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "#include \"perimortem/graphics/ttx_shader.hpp\""_view) == Count(-1));
  EXPECT_NOT(
      Algorithm::search(header.get_view(), "struct icon_Push"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "static constexpr ShaderModule icon_modules[]"_view) == Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "ShaderStage::Vertex, icon_vert_spv, &icon_vert_spv_size"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "ShaderStage::Pixel, icon_frag_spv, &icon_frag_spv_size"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "static constexpr ShaderPushConstantRange icon_push_constant_ranges[]"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "sizeof(icon_Push), shader_stage_vertex | shader_stage_pixel"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "static constexpr ShaderDescriptorBinding icon_descriptor_bindings[]"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(header.get_view(), "{\"icon_texture\", 0, 0}"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "static constexpr ShaderObjectPropertyBinding icon_object_properties[]"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(), "{ShaderObjectProperty::Position, 0, 8}"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(), "{ShaderObjectProperty::HalfSize, 8, 8}"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(), "{ShaderObjectProperty::Alpha, 16, 4}"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "static constexpr ShaderProgramInfo icon_shader_info"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "static constexpr Shader icon_shader = {&Internal::icon_shader_info}"_view) ==
      Count(-1));
}
