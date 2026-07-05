// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/compiler/shader.hpp"
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
    "  public vertex : stage {\n"
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

static auto load_render(Resolver& resolver, Resolver::Context& context)
    -> const Source::Record* {
  return resolver.load_source(context, "unit/render.ttx"_view, render_source);
}

static auto load_shader(Resolver& resolver, Resolver::Context& context)
    -> const Source::Record* {
  return resolver.load_source(context, "unit/shader.ttx"_view, shader_source);
}

static auto root_type(const Source::Record* record) -> const Ttx::Type* {
  return record == nullptr ? nullptr : record->get_type();
}

static auto function(const Ttx::Type& type, View::Bytes name)
    -> const Ttx::Type::Function* {
  return type.find_function(name);
}

static auto first_error(const Resolver::Context& context) -> View::Bytes {
  return context.get_errors()[0].get_message();
}

PERIMORTEM_UNIT_TEST(TtxShader, contract) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  ASSERT(load_render(resolver, context) != nullptr);
  EXPECT_NOT(context.has_errors());
  context.reset();

  const Source::Record* shader = load_shader(resolver, context);
  ASSERT(shader != nullptr);
  EXPECT_NOT(context.has_errors());

  const Ttx::Type* type = root_type(shader);
  ASSERT(type != nullptr);
  EXPECT_TEXT(type->get_name(), "Default2D"_view);
  ASSERT(function(*type, "vertex"_view) != nullptr);
  ASSERT(function(*type, "pixel"_view) != nullptr);
  EXPECT(function(*type, "vertex"_view)->has_body());
  EXPECT(function(*type, "pixel"_view)->has_body());
}

PERIMORTEM_UNIT_TEST(TtxShader, bad_contract) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  ASSERT(load_render(resolver, context) != nullptr);
  EXPECT_NOT(context.has_errors());
  context.reset();

  EXPECT_NOT(resolver.load_source(
      context, "unit/bad_shader.ttx"_view,
      "dialect : Shader;\n"
      "import Renderer : Render = \"render.ttx\";\n"
      "shader Default2D : Renderer::Render2D {\n"
      "  func vertex[.index : Bits_32] -> [.screen_position : Vec4D] {\n"
      "    return;\n"
      "  }\n"
      "}\n"_view));

  ASSERT(context.has_errors());
  EXPECT_TEXT(
      first_error(context),
      "Shader stage parameters do not match the render contract."_view);
}

PERIMORTEM_UNIT_TEST(TtxShader, named_fit) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  ASSERT(resolver.load_source(
      context, "unit/render.ttx"_view,
      "dialect : Render;\n"
      "public Render2D : Render {\n"
      "  public vertex : stage {\n"
      "    input [.x : Bits_32, .y : Bits_32];\n"
      "    output [.color : Vec4D, .depth : Real_32];\n"
      "  }\n"
      "}\n"_view));
  EXPECT_NOT(context.has_errors());
  context.reset();

  const Source::Record* shader = resolver.load_source(
      context, "unit/shader.ttx"_view,
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
  EXPECT_NOT(context.has_errors());
}

PERIMORTEM_UNIT_TEST(TtxShader, bad_reads) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  ASSERT(resolver.load_source(
      context, "unit/render.ttx"_view,
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
  EXPECT_NOT(context.has_errors());
  context.reset();

  EXPECT_NOT(resolver.load_source(
      context, "unit/shader.ttx"_view,
      "dialect : Shader;\n"
      "import Renderer : Render = \"render.ttx\";\n"
      "shader Default2D : Renderer::Render2D {\n"
      "  func vertex[] -> [] {\n"
      "    state value : Bits_32 = push.tone;\n"
      "    return;\n"
      "  }\n"
      "}\n"_view));

  ASSERT(context.has_errors());
  EXPECT_TEXT(first_error(context), "Shader stage cannot read render fact."_view);
}

PERIMORTEM_UNIT_TEST(TtxShader, stage_symbols) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolver resolver(toolchain);
  Resolver::Context context;

  ASSERT(load_render(resolver, context) != nullptr);
  EXPECT_NOT(context.has_errors());
  context.reset();

  const Source::Record* shader = load_shader(resolver, context);
  ASSERT(shader != nullptr);
  ASSERT(root_type(shader) != nullptr);
  EXPECT_NOT(context.has_errors());

  Allocator::Arena arena;
  Tetrodotoxin::Compiler::Shader compiler(arena);
  ASSERT(compiler.lower("default_2d"_view, *root_type(shader)));

  Tetrodotoxin::Linker::Linker linker;
  compiler.add_to(linker);
  auto archive = linker.build_library("shader.o"_view);
  EXPECT(
      Algorithm::search(
          archive.get_view(),
          "TTX_shader_default_2d_Default2D_vertex_spirv"_view) != Count(-1));
  EXPECT(
      Algorithm::search(
          archive.get_view(),
          "TTX_shader_default_2d_Default2D_pixel_spirv"_view) != Count(-1));
}
