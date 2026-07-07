// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/shader.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/isa/shader/block.hpp"
#include "tetrodotoxin/linker/linker.hpp"
#include "tetrodotoxin/puffer/resolution/resolver.hpp"
#include "tetrodotoxin/toolchain.hpp"
#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Puffer::Resolution;
using namespace Validation;

static Harness TtxShader = {
  .name = "TTX::Shader"_view,
};

static constexpr View::Bytes render_source =
    "dialect : Render;\n"
    "public Render2D : Render {\n"
    "  public position : Vec2D;\n"
    "  push_constants {\n"
    "    const position : Vec2D = self.position;\n"
    "  }\n"
    "  public vertex : stage {\n"
    "    reads push[position];\n"
    "    input [.vertex_index : Bits_32];\n"
    "    output [.screen_position : Vec4D];\n"
    "  }\n"
    "  public pixel : stage {\n"
    "    input [];\n"
    "    output [.color : Vec4D];\n"
    "  }\n"
    "}\n"_view;

static constexpr View::Bytes shader_source =
    "dialect : Shader;\n"
    "import Renderer : Render = \"render.ttx\";\n"
    "shader Default2D : Renderer::Render2D {\n"
    "  func vertex[.vertex_index : Bits_32] -> [.screen_position : Vec4D] {\n"
    "    return;\n"
    "  }\n"
    "  func pixel[] -> [.color : Vec4D] {\n"
    "    return;\n"
    "  }\n"
    "}\n"_view;

static auto load_render(Resolver& resolver, Resolver::Context& source_context)
    -> const Source::Record* {
  return resolver.load_source(
      source_context, "unit/render.ttx"_view, render_source);
}

static auto load_shader(Resolver& resolver, Resolver::Context& source_context)
    -> const Source::Record* {
  return resolver.load_source(
      source_context, "unit/shader.ttx"_view, shader_source);
}

static auto root_type(const Source::Record* record) -> const Ttx::Type* {
  return record == nullptr ? nullptr : record->get_type();
}

static auto function(const Ttx::Type& type, View::Bytes name)
    -> const Ttx::Type::Function* {
  return type.find_function(name);
}

static auto first_error(const Resolver::Context& source_context)
    -> View::Bytes {
  return source_context.get_errors()[0].get_message();
}

static auto first_error(const Ttx::Lexical::Errors& errors) -> View::Bytes {
  return errors.get_view()[0].get_message();
}

PERIMORTEM_UNIT_TEST(TtxShader, contract) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context render_source_context;

  ASSERT(load_render(resolver, render_source_context) != nullptr);
  EXPECT_NOT(render_source_context.has_errors());
  Resolver::Context shader_source_context;

  const Source::Record* shader = load_shader(resolver, shader_source_context);
  ASSERT(shader != nullptr);
  EXPECT_NOT(shader_source_context.has_errors());

  const Ttx::Type* type = root_type(shader);
  ASSERT(type != nullptr);
  EXPECT_TEXT(type->get_name(), "Default2D"_view);
  const Ttx::Type::Function* vertex = function(*type, "vertex"_view);
  const Ttx::Type::Function* pixel = function(*type, "pixel"_view);
  ASSERT(vertex != nullptr);
  ASSERT(pixel != nullptr);
  EXPECT(vertex->has_body());
  EXPECT(pixel->has_body());
  ASSERT(vertex->get_blocks().get_size() == 1);
  ASSERT(vertex->get_blocks()[0].get_block() != nullptr);
  EXPECT(
      vertex->get_blocks()[0].get_block()->get_representation() ==
      &Tetrodotoxin::Isa::Shader::Block::get_representation_type());
}

PERIMORTEM_UNIT_TEST(TtxShader, bad_contract) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context render_source_context;

  ASSERT(load_render(resolver, render_source_context) != nullptr);
  EXPECT_NOT(render_source_context.has_errors());
  Resolver::Context bad_shader_source_context;

  EXPECT_NOT(resolver.load_source(
      bad_shader_source_context, "unit/bad_shader.ttx"_view,
      "dialect : Shader;\n"
      "import Renderer : Render = \"render.ttx\";\n"
      "shader Default2D : Renderer::Render2D {\n"
      "  func vertex[.index : Bits_32] -> [.screen_position : Vec4D] {\n"
      "    return;\n"
      "  }\n"
      "}\n"_view));

  ASSERT(bad_shader_source_context.has_errors());
  EXPECT_TEXT(
      first_error(bad_shader_source_context),
      "Shader stage parameters do not match the render contract."_view);
}

PERIMORTEM_UNIT_TEST(TtxShader, named_fit) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context render_source_context;

  ASSERT(resolver.load_source(
      render_source_context, "unit/render.ttx"_view,
      "dialect : Render;\n"
      "public Render2D : Render {\n"
      "  public vertex : stage {\n"
      "    input [.x : Bits_32, .y : Bits_32];\n"
      "    output [.color : Vec4D, .depth : Real_32];\n"
      "  }\n"
      "}\n"_view));
  EXPECT_NOT(render_source_context.has_errors());
  Resolver::Context shader_source_context;

  const Source::Record* shader = resolver.load_source(
      shader_source_context, "unit/shader.ttx"_view,
      "dialect : Shader;\n"
      "import Renderer : Render = \"render.ttx\";\n"
      "shader Default2D : Renderer::Render2D {\n"
      "  func vertex[.y : Bits_32, .x : Bits_32] -> [\n"
      "    .depth : Real_32,\n"
      "    .color : Vec4D,\n"
      "  ] {\n"
      "    return;\n"
      "  }\n"
      "}\n"_view);

  ASSERT(shader != nullptr);
  EXPECT_NOT(shader_source_context.has_errors());
}

PERIMORTEM_UNIT_TEST(TtxShader, bad_reads) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context render_source_context;

  ASSERT(resolver.load_source(
      render_source_context, "unit/render.ttx"_view,
      "dialect : Render;\n"
      "public Render2D : Render {\n"
      "  push_constants {\n"
      "    const position : Bits_32 = 0;\n"
      "    const tone : Bits_32 = 0;\n"
      "  }\n"
      "  public vertex : stage {\n"
      "    reads push[position];\n"
      "    input [];\n"
      "    output [];\n"
      "  }\n"
      "}\n"_view));
  EXPECT_NOT(render_source_context.has_errors());
  Resolver::Context bad_reads_source_context;

  EXPECT_NOT(resolver.load_source(
      bad_reads_source_context, "unit/shader.ttx"_view,
      "dialect : Shader;\n"
      "import Renderer : Render = \"render.ttx\";\n"
      "shader Default2D : Renderer::Render2D {\n"
      "  func vertex[] -> [] {\n"
      "    state value : Bits_32 = push.tone;\n"
      "    return;\n"
      "  }\n"
      "}\n"_view));

  ASSERT(bad_reads_source_context.has_errors());
  EXPECT_TEXT(
      first_error(bad_reads_source_context),
      "Shader stage cannot read render fact."_view);
}

PERIMORTEM_UNIT_TEST(TtxShader, stage_symbols) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context render_source_context;

  const Source::Record* render = load_render(resolver, render_source_context);
  ASSERT(render != nullptr);
  ASSERT(root_type(render) != nullptr);
  EXPECT_NOT(render_source_context.has_errors());
  Resolver::Context shader_source_context;

  const Source::Record* shader = load_shader(resolver, shader_source_context);
  ASSERT(shader != nullptr);
  ASSERT(root_type(shader) != nullptr);
  EXPECT_NOT(shader_source_context.has_errors());

  Allocator::Arena arena;
  Ttx::Lexical::Errors compiler_errors(arena);
  Tetrodotoxin::Compiler::Shader compiler;
  ASSERT(compiler.lower(
      arena, compiler_errors,
      Ttx::Lexical::Source(render->get_source_path(), render->get_content()),
      "render"_view, *root_type(render)));
  ASSERT(compiler.lower(
      arena, compiler_errors,
      Ttx::Lexical::Source(shader->get_source_path(), shader->get_content()),
      "default_2d"_view, *root_type(shader)));

  Tetrodotoxin::Linker::Linker linker;
  linker.add(compiler);
  auto archive = linker.build_library("shader.o"_view);
  EXPECT(
      Algorithm::search(
          archive.get_view(),
          "TTX_shader_default_2d_Default2D_vertex_spirv"_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          archive.get_view(),
          "TTX_shader_default_2d_Default2D_pixel_spirv"_view) != Count(-1));
  EXPECT(
      Algorithm::search(archive.get_view(), "vertex_index"_view) != Count(-1));
  EXPECT(
      Algorithm::search(archive.get_view(), "screen_position"_view) !=
      Count(-1));
  EXPECT(Algorithm::search(archive.get_view(), "push"_view) != Count(-1));
  EXPECT(Algorithm::search(archive.get_view(), "position"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxShader, pass_through) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context render_source_context;

  const Source::Record* render = resolver.load_source(
      render_source_context, "unit/render.ttx"_view,
      "dialect : Render;\n"
      "public Copy : Render {\n"
      "  public pixel : stage {\n"
      "    input [.color : Vec4D];\n"
      "    output [.color : Vec4D];\n"
      "  }\n"
      "}\n"_view);
  ASSERT(render != nullptr);
  ASSERT(root_type(render) != nullptr);
  EXPECT_NOT(render_source_context.has_errors());
  Resolver::Context shader_source_context;

  const Source::Record* shader = resolver.load_source(
      shader_source_context, "unit/shader.ttx"_view,
      "dialect : Shader;\n"
      "import Renderer : Render = \"render.ttx\";\n"
      "shader CopyShader : Renderer::Copy {\n"
      "  func pixel[.color : Vec4D] -> [.color : Vec4D] {\n"
      "    return (.color = color);\n"
      "  }\n"
      "}\n"_view);
  ASSERT(shader != nullptr);
  ASSERT(root_type(shader) != nullptr);
  EXPECT_NOT(shader_source_context.has_errors());

  Allocator::Arena arena;
  Ttx::Lexical::Errors compiler_errors(arena);
  Tetrodotoxin::Compiler::Shader compiler;
  ASSERT(compiler.lower(
      arena, compiler_errors,
      Ttx::Lexical::Source(render->get_source_path(), render->get_content()),
      "render"_view, *root_type(render)));
  ASSERT(compiler.lower(
      arena, compiler_errors,
      Ttx::Lexical::Source(shader->get_source_path(), shader->get_content()),
      "copy"_view, *root_type(shader)));

  static constexpr Static::Vector<Bits_8, 4> store_instruction = {
    0x3E, 0x00, 0x03, 0x00};
  constexpr View::Bytes store_pattern(
      store_instruction.get_data(), store_instruction.get_size());
  EXPECT(
      Algorithm::search(compiler.get_read_only(), store_pattern) !=
      Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxShader, state_diag) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context render_source_context;

  const Source::Record* render = resolver.load_source(
      render_source_context, "unit/render.ttx"_view,
      "dialect : Render;\n"
      "public Copy : Render {\n"
      "  public pixel : stage {\n"
      "    input [];\n"
      "    output [];\n"
      "  }\n"
      "}\n"_view);
  ASSERT(render != nullptr);
  ASSERT(root_type(render) != nullptr);
  EXPECT_NOT(render_source_context.has_errors());
  Resolver::Context shader_source_context;

  const Source::Record* shader = resolver.load_source(
      shader_source_context, "unit/shader.ttx"_view,
      "dialect : Shader;\n"
      "import Renderer : Render = \"render.ttx\";\n"
      "shader CopyShader : Renderer::Copy {\n"
      "  func pixel[] -> [] {\n"
      "    state value : Bits_32 = 0;\n"
      "    state other : Bits_32 = 1;\n"
      "    return;\n"
      "  }\n"
      "}\n"_view);
  ASSERT(shader != nullptr);
  ASSERT(root_type(shader) != nullptr);
  EXPECT_NOT(shader_source_context.has_errors());

  Allocator::Arena arena;
  Ttx::Lexical::Errors compiler_errors(arena);
  Tetrodotoxin::Compiler::Shader compiler;
  ASSERT(compiler.lower(
      arena, compiler_errors,
      Ttx::Lexical::Source(render->get_source_path(), render->get_content()),
      "render"_view, *root_type(render)));
  EXPECT_NOT(compiler.lower(
      arena, compiler_errors,
      Ttx::Lexical::Source(shader->get_source_path(), shader->get_content()),
      "copy"_view, *root_type(shader)));
  ASSERT(compiler_errors.has_errors());
  EXPECT_EQ(compiler_errors.get_view().get_size(), Count(2));
  const auto error = compiler_errors.get_view()[0];
  EXPECT_TEXT(error.get_source_path(), "unit/shader.ttx"_view);
  EXPECT_TEXT(
      error.get_message(), "Shader state statements cannot lower today."_view);

  const Ttx::Lexical::Token* start = error.get_start_token();
  const Ttx::Lexical::Token* end = error.get_end_token();
  ASSERT(start != nullptr);
  ASSERT(end != nullptr);
  EXPECT_EQ(start->get_line(), Bits_32(5));
  EXPECT_EQ(start->get_column(), Bits_32(5));
  EXPECT_EQ(end->get_column(), Bits_32(30));
}

PERIMORTEM_UNIT_TEST(TtxShader, needs_render) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context render_source_context;

  ASSERT(load_render(resolver, render_source_context) != nullptr);
  EXPECT_NOT(render_source_context.has_errors());
  Resolver::Context shader_source_context;

  const Source::Record* shader = load_shader(resolver, shader_source_context);
  ASSERT(shader != nullptr);
  ASSERT(root_type(shader) != nullptr);
  EXPECT_NOT(shader_source_context.has_errors());

  Allocator::Arena arena;
  Ttx::Lexical::Errors compiler_errors(arena);
  Tetrodotoxin::Compiler::Shader compiler;
  EXPECT_NOT(compiler.lower(
      arena, compiler_errors,
      Ttx::Lexical::Source(shader->get_source_path(), shader->get_content()),
      "default_2d"_view, *root_type(shader)));
  ASSERT(compiler_errors.has_errors());
  EXPECT_TEXT(
      first_error(compiler_errors),
      "Shader compiler could not find render contract."_view);
}
