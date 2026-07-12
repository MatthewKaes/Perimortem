// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/compiler/engine.hpp"
#include "tetrodotoxin/compiler/target/system_v.hpp"
#include "tetrodotoxin/isa/shader/block.hpp"
#include "tetrodotoxin/isa/shader/compiler.hpp"
#include "tetrodotoxin/puffer/resolution/resolver.hpp"
#include "tetrodotoxin/toolchain.hpp"
#include "ttx/type.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Puffer;
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

static auto load_render(
    Resolution::Resolver& resolver,
    Resolution::Resolver::Context& source_context)
    -> const Resolution::Source::Record* {
  return resolver.load_source(
      source_context, "unit/render.ttx"_view, render_source);
}

static auto load_shader(
    Resolution::Resolver& resolver,
    Resolution::Resolver::Context& source_context)
    -> const Resolution::Source::Record* {
  return resolver.load_source(
      source_context, "unit/shader.ttx"_view, shader_source);
}

static auto root_type(const Resolution::Source::Record* record)
    -> const Ttx::Type* {
  return record == nullptr ? nullptr : &record->get_type();
}

static auto function(const Ttx::Type& type, View::Bytes name)
    -> const Ttx::Function* {
  return type.find_function(name);
}

static auto first_error(const Resolution::Resolver::Context& source_context)
    -> View::Bytes {
  return source_context.get_errors()[0].get_message();
}

static auto first_error(const Ttx::Lexical::Errors& errors) -> View::Bytes {
  return errors.get_view()[0].get_message();
}

PERIMORTEM_UNIT_TEST(TtxShader, contract) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolution::Resolver resolver(toolchain);
  Resolution::Resolver::Context render_source_context;

  const Resolution::Source::Record* render =
      load_render(resolver, render_source_context);
  ASSERT(render != nullptr);
  EXPECT_NOT(render_source_context.has_errors());
  Resolution::Resolver::Context shader_source_context;

  const Resolution::Source::Record* shader =
      load_shader(resolver, shader_source_context);
  ASSERT(shader != nullptr);
  EXPECT_NOT(shader_source_context.has_errors());

  const Ttx::Type* type = root_type(shader);
  ASSERT(type != nullptr);
  EXPECT_TEXT(type->get_name(), "Default2D"_view);
  const Ttx::Type* render_type = root_type(render)->find_type("Render2D"_view);
  const Ttx::Type* contract = type->find_type("Contract"_view);
  ASSERT(render_type != nullptr);
  ASSERT(contract != nullptr);
  EXPECT(contract->is_alias());
  EXPECT(contract->equivalent_to(*render_type));
  const Ttx::Function* vertex = function(*type, "vertex"_view);
  const Ttx::Function* pixel = function(*type, "pixel"_view);
  ASSERT(vertex != nullptr);
  ASSERT(pixel != nullptr);
  EXPECT(shader->get_implementation().has(*vertex));
  EXPECT(shader->get_implementation().has(*pixel));
  const auto* vertex_body =
      shader->get_implementation().find<Tetrodotoxin::Isa::Shader::Block>(
          *vertex);
  ASSERT(vertex_body != nullptr);
  EXPECT_NOT(vertex_body->get_statements().is_empty());
}

PERIMORTEM_UNIT_TEST(TtxShader, bad_contract) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolution::Resolver resolver(toolchain);
  Resolution::Resolver::Context render_source_context;

  ASSERT(load_render(resolver, render_source_context) != nullptr);
  EXPECT_NOT(render_source_context.has_errors());
  Resolution::Resolver::Context bad_shader_source_context;

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
  Resolution::Resolver resolver(toolchain);
  Resolution::Resolver::Context render_source_context;

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
  Resolution::Resolver::Context shader_source_context;

  const Resolution::Source::Record* shader = resolver.load_source(
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
  Resolution::Resolver resolver(toolchain);
  Resolution::Resolver::Context render_source_context;

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
  Resolution::Resolver::Context bad_reads_source_context;

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
  Resolution::Resolver resolver(toolchain);
  Resolution::Resolver::Context render_source_context;

  const Resolution::Source::Record* render =
      load_render(resolver, render_source_context);
  ASSERT(render != nullptr);
  ASSERT(root_type(render) != nullptr);
  EXPECT_NOT(render_source_context.has_errors());
  Resolution::Resolver::Context shader_source_context;

  const Resolution::Source::Record* shader =
      load_shader(resolver, shader_source_context);
  ASSERT(shader != nullptr);
  ASSERT(root_type(shader) != nullptr);
  EXPECT_NOT(shader_source_context.has_errors());

  Allocator::Arena arena;
  Ttx::Lexical::Errors compiler_errors(arena);
  Tetrodotoxin::Isa::Shader::Compiler compiler;
  ASSERT(compiler.lower(
      arena, compiler_errors,
      Ttx::Lexical::Source(shader->get_source_path(), shader->get_content()),
      "default_2d"_view, *root_type(shader), shader->get_implementation()));

  Tetrodotoxin::Compiler::Engine engine(
      compiler_errors, Tetrodotoxin::Compiler::Target::SystemV::backend());
  ASSERT(engine.publish_read_only(
      compiler.get_read_only(), compiler.get_stages()));
  Dynamic::Bytes archive = engine.build_archive("shader.o"_view);
  ASSERT_NOT(archive.is_empty());
  EXPECT(
      Algorithm::search(
          archive, "TTX_shader_default_2d_Default2D_vertex_spirv"_view) !=
      Count(-1));
  EXPECT(
      Algorithm::search(
          archive, "TTX_shader_default_2d_Default2D_pixel_spirv"_view) !=
      Count(-1));
  EXPECT(Algorithm::search(archive, "vertex_index"_view) != Count(-1));
  EXPECT(Algorithm::search(archive, "screen_position"_view) != Count(-1));
  EXPECT(Algorithm::search(archive, "push"_view) != Count(-1));
  EXPECT(Algorithm::search(archive, "position"_view) != Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxShader, pass_through) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolution::Resolver resolver(toolchain);
  Resolution::Resolver::Context render_source_context;

  const Resolution::Source::Record* render = resolver.load_source(
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
  Resolution::Resolver::Context shader_source_context;

  const Resolution::Source::Record* shader = resolver.load_source(
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
  Tetrodotoxin::Isa::Shader::Compiler compiler;
  ASSERT(compiler.lower(
      arena, compiler_errors,
      Ttx::Lexical::Source(shader->get_source_path(), shader->get_content()),
      "copy"_view, *root_type(shader), shader->get_implementation()));

  static constexpr Static::Vector<Bits_8, 4> store_instruction = {
    0x3E, 0x00, 0x03, 0x00};
  constexpr View::Bytes store_pattern(
      store_instruction.get_data(), store_instruction.get_size());
  EXPECT(
      Algorithm::search(compiler.get_read_only(), store_pattern) != Count(-1));
}

PERIMORTEM_UNIT_TEST(TtxShader, annotated_type) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolution::Resolver resolver(toolchain);
  Resolution::Resolver::Context types_source_context;

  ASSERT(resolver.load_source(
      types_source_context, "unit/types.ttx"_view,
      "dialect : Library;\n"
      "@shader_type(Vec4D)\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "  public g : Real_32;\n"
      "  public b : Real_32;\n"
      "  public a : Real_32;\n"
      "}\n"_view));
  EXPECT_NOT(types_source_context.has_errors());
  Resolution::Resolver::Context render_source_context;

  const Resolution::Source::Record* render = resolver.load_source(
      render_source_context, "unit/render.ttx"_view,
      "dialect : Render;\n"
      "import Types : Library = \"types.ttx\";\n"
      "public Copy : Render {\n"
      "  public pixel : stage {\n"
      "    input [];\n"
      "    output [.color : Types::Color];\n"
      "  }\n"
      "}\n"_view);
  ASSERT(render != nullptr);
  ASSERT(root_type(render) != nullptr);
  EXPECT_NOT(render_source_context.has_errors());
  Resolution::Resolver::Context shader_source_context;

  const Resolution::Source::Record* shader = resolver.load_source(
      shader_source_context, "unit/shader.ttx"_view,
      "dialect : Shader;\n"
      "import Renderer : Render = \"render.ttx\";\n"
      "import Types : Library = \"types.ttx\";\n"
      "shader CopyShader : Renderer::Copy {\n"
      "  func pixel[] -> [.color : Types::Color] {\n"
      "    return;\n"
      "  }\n"
      "}\n"_view);
  ASSERT(shader != nullptr);
  ASSERT(root_type(shader) != nullptr);
  EXPECT_NOT(shader_source_context.has_errors());

  Allocator::Arena arena;
  Ttx::Lexical::Errors compiler_errors(arena);
  Tetrodotoxin::Isa::Shader::Compiler compiler;
  EXPECT(compiler.lower(
      arena, compiler_errors,
      Ttx::Lexical::Source(shader->get_source_path(), shader->get_content()),
      "copy"_view, *root_type(shader), shader->get_implementation()));
  EXPECT_NOT(compiler_errors.has_errors());
}

PERIMORTEM_UNIT_TEST(TtxShader, unmarked_type) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolution::Resolver resolver(toolchain);
  Resolution::Resolver::Context types_source_context;

  ASSERT(resolver.load_source(
      types_source_context, "unit/types.ttx"_view,
      "dialect : Library;\n"
      "public Color : struct {\n"
      "  public r : Real_32;\n"
      "  public g : Real_32;\n"
      "  public b : Real_32;\n"
      "  public a : Real_32;\n"
      "}\n"_view));
  EXPECT_NOT(types_source_context.has_errors());
  Resolution::Resolver::Context render_source_context;

  const Resolution::Source::Record* render = resolver.load_source(
      render_source_context, "unit/render.ttx"_view,
      "dialect : Render;\n"
      "import Types : Library = \"types.ttx\";\n"
      "public Copy : Render {\n"
      "  public pixel : stage {\n"
      "    input [];\n"
      "    output [.color : Types::Color];\n"
      "  }\n"
      "}\n"_view);
  ASSERT(render != nullptr);
  ASSERT(root_type(render) != nullptr);
  EXPECT_NOT(render_source_context.has_errors());
  Resolution::Resolver::Context shader_source_context;

  const Resolution::Source::Record* shader = resolver.load_source(
      shader_source_context, "unit/shader.ttx"_view,
      "dialect : Shader;\n"
      "import Renderer : Render = \"render.ttx\";\n"
      "import Types : Library = \"types.ttx\";\n"
      "shader CopyShader : Renderer::Copy {\n"
      "  func pixel[] -> [.color : Types::Color] {\n"
      "    return;\n"
      "  }\n"
      "}\n"_view);
  ASSERT(shader != nullptr);
  ASSERT(root_type(shader) != nullptr);
  EXPECT_NOT(shader_source_context.has_errors());

  Allocator::Arena arena;
  Ttx::Lexical::Errors compiler_errors(arena);
  Tetrodotoxin::Isa::Shader::Compiler compiler;
  EXPECT_NOT(compiler.lower(
      arena, compiler_errors,
      Ttx::Lexical::Source(shader->get_source_path(), shader->get_content()),
      "copy"_view, *root_type(shader), shader->get_implementation()));
  ASSERT(compiler_errors.has_errors());
  EXPECT_TEXT(
      first_error(compiler_errors),
      "Shader result type cannot lower today."_view);
}

PERIMORTEM_UNIT_TEST(TtxShader, state_diag) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolution::Resolver resolver(toolchain);
  Resolution::Resolver::Context render_source_context;

  const Resolution::Source::Record* render = resolver.load_source(
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
  Resolution::Resolver::Context shader_source_context;

  const Resolution::Source::Record* shader = resolver.load_source(
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
  Tetrodotoxin::Isa::Shader::Compiler compiler;
  EXPECT_NOT(compiler.lower(
      arena, compiler_errors,
      Ttx::Lexical::Source(shader->get_source_path(), shader->get_content()),
      "copy"_view, *root_type(shader), shader->get_implementation()));
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

PERIMORTEM_UNIT_TEST(TtxShader, no_render_pass) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolution::Resolver resolver(toolchain);
  Resolution::Resolver::Context render_source_context;

  ASSERT(load_render(resolver, render_source_context) != nullptr);
  EXPECT_NOT(render_source_context.has_errors());
  Resolution::Resolver::Context shader_source_context;

  const Resolution::Source::Record* shader =
      load_shader(resolver, shader_source_context);
  ASSERT(shader != nullptr);
  ASSERT(root_type(shader) != nullptr);
  EXPECT_NOT(shader_source_context.has_errors());

  Allocator::Arena arena;
  Ttx::Lexical::Errors compiler_errors(arena);
  Tetrodotoxin::Isa::Shader::Compiler compiler;
  EXPECT(compiler.lower(
      arena, compiler_errors,
      Ttx::Lexical::Source(shader->get_source_path(), shader->get_content()),
      "default_2d"_view, *root_type(shader), shader->get_implementation()));
  EXPECT_NOT(compiler_errors.has_errors());
}

PERIMORTEM_UNIT_TEST(TtxShader, bad_result) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolution::Resolver resolver(toolchain);
  Resolution::Resolver::Context render_source_context;

  ASSERT(load_render(resolver, render_source_context) != nullptr);
  EXPECT_NOT(render_source_context.has_errors());
  Resolution::Resolver::Context shader_source_context;

  EXPECT_NOT(resolver.load_source(
      shader_source_context, "unit/bad_result.ttx"_view,
      "dialect : Shader;\n"
      "import Renderer : Render = \"render.ttx\";\n"
      "shader Default2D : Renderer::Render2D {\n"
      "  func vertex[.vertex_index : Bits_32] -> [.color : Vec4D] {\n"
      "    return;\n"
      "  }\n"
      "}\n"_view));

  ASSERT(shader_source_context.has_errors());
  EXPECT_TEXT(
      first_error(shader_source_context),
      "Shader stage result does not match the render contract."_view);
}

PERIMORTEM_UNIT_TEST(TtxShader, missing_stage) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolution::Resolver resolver(toolchain);
  Resolution::Resolver::Context render_source_context;

  ASSERT(load_render(resolver, render_source_context) != nullptr);
  EXPECT_NOT(render_source_context.has_errors());
  Resolution::Resolver::Context shader_source_context;

  EXPECT_NOT(resolver.load_source(
      shader_source_context, "unit/missing_stage.ttx"_view,
      "dialect : Shader;\n"
      "import Renderer : Render = \"render.ttx\";\n"
      "shader Default2D : Renderer::Render2D {\n"
      "  func compute[] -> [] {\n"
      "    return;\n"
      "  }\n"
      "}\n"_view));

  ASSERT(shader_source_context.has_errors());
  EXPECT_TEXT(
      first_error(shader_source_context),
      "Shader stage is not declared by the render contract."_view);
}

PERIMORTEM_UNIT_TEST(TtxShader, dupe_stage) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolution::Resolver resolver(toolchain);
  Resolution::Resolver::Context render_source_context;

  ASSERT(load_render(resolver, render_source_context) != nullptr);
  EXPECT_NOT(render_source_context.has_errors());
  Resolution::Resolver::Context shader_source_context;

  EXPECT_NOT(resolver.load_source(
      shader_source_context, "unit/dupe_stage.ttx"_view,
      "dialect : Shader;\n"
      "import Renderer : Render = \"render.ttx\";\n"
      "shader Default2D : Renderer::Render2D {\n"
      "  func pixel[] -> [.color : Vec4D] { return; }\n"
      "  func pixel[] -> [.color : Vec4D] { return; }\n"
      "}\n"_view));

  ASSERT(shader_source_context.has_errors());
  EXPECT_TEXT(
      first_error(shader_source_context),
      "Shader stage name is already defined."_view);
}

PERIMORTEM_UNIT_TEST(TtxShader, bad_contract_ref) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolution::Resolver resolver(toolchain);
  Resolution::Resolver::Context render_source_context;

  ASSERT(load_render(resolver, render_source_context) != nullptr);
  EXPECT_NOT(render_source_context.has_errors());
  Resolution::Resolver::Context shader_source_context;

  EXPECT_NOT(resolver.load_source(
      shader_source_context, "unit/bad_contract.ttx"_view,
      "dialect : Shader;\n"
      "import Renderer : Render = \"render.ttx\";\n"
      "shader Default2D : Renderer::Missing {\n"
      "}\n"_view));

  ASSERT(shader_source_context.has_errors());
  EXPECT_TEXT(
      first_error(shader_source_context),
      "Shader render contract could not be resolved."_view);
}

PERIMORTEM_UNIT_TEST(TtxShader, read_needs_member) {
  Tetrodotoxin::Toolchain toolchain = Tetrodotoxin::Toolchain::standard();
  Resolution::Resolver resolver(toolchain);
  Resolution::Resolver::Context render_source_context;

  ASSERT(resolver.load_source(
      render_source_context, "unit/render.ttx"_view,
      "dialect : Render;\n"
      "public Copy : Render {\n"
      "  push_constants { const position : Vec2D; }\n"
      "  public vertex : stage {\n"
      "    reads push[position];\n"
      "    input [];\n"
      "    output [];\n"
      "  }\n"
      "}\n"_view));
  EXPECT_NOT(render_source_context.has_errors());
  Resolution::Resolver::Context shader_source_context;

  EXPECT_NOT(resolver.load_source(
      shader_source_context, "unit/read_root.ttx"_view,
      "dialect : Shader;\n"
      "import Renderer : Render = \"render.ttx\";\n"
      "shader CopyShader : Renderer::Copy {\n"
      "  func vertex[] -> [] {\n"
      "    state value : Vec2D = push;\n"
      "    return;\n"
      "  }\n"
      "}\n"_view));

  ASSERT(shader_source_context.has_errors());
  EXPECT_TEXT(
      first_error(shader_source_context),
      "Shader render fact access needs a member name."_view);
}
