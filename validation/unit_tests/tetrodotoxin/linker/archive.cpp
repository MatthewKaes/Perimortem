// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/compiler/assembler/spirv.hpp"
#include "tetrodotoxin/compiler/compiler.hpp"
#include "tetrodotoxin/compiler/shader/spirv.hpp"
#include "tetrodotoxin/compiler/shader/program.hpp"
#include "tetrodotoxin/linker/linker.hpp"
#include "tetrodotoxin/resolution/resolver.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness TtxLinker = {
  .name = "Tetrodotoxin::Linker"_view,
};

PERIMORTEM_UNIT_TEST(TtxLinker, ro_render_symbols) {
  Resolution::Resolver resolver;
  ASSERT(resolver.resolve("apps/shaders/default_2d.ttx"_view));

  Resolution::Record* record =
      resolver.find("apps/shaders/default_2d.ttx"_view);
  ASSERT(record);

  Compiler::Compiler compiler;
  Perimortem::Memory::Dynamic::Bytes vertex_words;
  Perimortem::Memory::Dynamic::Bytes pixel_words;
  ASSERT(Compiler::Shader::emit_program_modules(
      *record, vertex_words, pixel_words));
  ASSERT(Compiler::Assembler::spirv::is_valid_module(vertex_words));
  ASSERT(Compiler::Assembler::spirv::is_valid_module(pixel_words));

  Compiler::Shader::add_program_symbols(
      compiler, *record,
      {
        "default_2d_shader"_view,
        vertex_words,
        pixel_words,
      });

  const auto archive =
      Linker::emit_archive(compiler, "default_2d_shader_program.o"_view);

  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "!<arch>\n"_view) == Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "default_2d_shader"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), ".rodata"_view) == Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "default_2d_shader_vert_spv"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "default_2d_shader_vert_spv_size"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "default_2d_shader_frag_spv"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "default_2d_shader_frag_spv_size"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "default_2d_shader_host_inputs"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "default_2d_shader_host_inputs_size"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "default_2d_shader_descriptors"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "default_2d_shader_descriptors_size"_view) ==
      Count(-1));
  EXPECT_NOT(Algorithm::search(archive.get_view(), vertex_words) == Count(-1));
  EXPECT_NOT(Algorithm::search(archive.get_view(), pixel_words) == Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          archive.get_view(), "TTX_SHADER_HOST_INPUTS"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "size=24 alignment=8 fields=3"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          archive.get_view(), "field name=position type=Vec2D"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          archive.get_view(), "field name=size_pixels type=Size2D"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          archive.get_view(), "field name=alpha type=Real_32"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(archive.get_view(), "TTX_SHADER_DESCRIPTORS"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          archive.get_view(),
          "descriptor name=image type=Sampler2D set=0 slot=0"_view) ==
      Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxLinker, render_header_meta) {
  Resolution::Resolver resolver;
  ASSERT(resolver.resolve("apps/shaders/default_2d.ttx"_view));

  Resolution::Record* record =
      resolver.find("apps/shaders/default_2d.ttx"_view);
  ASSERT(record);

  Perimortem::Memory::Dynamic::Bytes header;
  Compiler::Shader::write_program_header(
      *record, "default_2d_shader"_view, header);

  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "#include \"perimortem/graphics/render.hpp\""_view) == Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "#include \"perimortem/core/static/vector.hpp\""_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(), "struct default_2d_shader_HostInputs"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "Perimortem::Core::Static::Vector<Perimortem::Graphics::Render::Module, 2> default_2d_shader_modules"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "Perimortem::Graphics::Render::Stage::Vertex, default_2d_shader_vert_spv, &default_2d_shader_vert_spv_size"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "Perimortem::Graphics::Render::Stage::Pixel, default_2d_shader_frag_spv, &default_2d_shader_frag_spv_size"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "Perimortem::Core::Static::Vector<Perimortem::Graphics::Render::HostInputRange, 1> default_2d_shader_host_input_ranges"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "sizeof(default_2d_shader_HostInputs), default_2d_shader_host_input_stages"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "Perimortem::Core::Static::Vector<Perimortem::Graphics::Render::DescriptorBinding, 1> default_2d_shader_descriptor_bindings"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "Perimortem::Graphics::Render::DescriptorBinding(\"image\", 0, 0)"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "Perimortem::Core::Static::Vector<Perimortem::Graphics::Render::HostField, 3> default_2d_shader_host_fields"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "Perimortem::Graphics::Render::HostField(\"position\", 0, 8)"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "Perimortem::Graphics::Render::HostField(\"size_pixels\", 8, 8)"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "Perimortem::Graphics::Render::HostField(\"alpha\", 16, 4)"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "static constexpr Perimortem::Graphics::Render::Program default_2d_shader_render_program"_view) ==
      Count(-1));
  EXPECT_NOT(
      Algorithm::search(
          header.get_view(),
          "static constexpr Perimortem::Graphics::Render default_2d_shader_renderer(Internal::default_2d_shader_render_program)"_view) ==
      Count(-1));
}
