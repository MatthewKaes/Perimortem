// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/dialect.hpp"

#include "validation/unit_test.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/shader/language/contract.hpp"
#include "tetrodotoxin/shader/language/monograph.hpp"
#include "ttx/lexical/errors.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness ShaderDialect = {
  .name = "Tetrodotoxin::Shader::Dialect"_view,
};

PERIMORTEM_UNIT_TEST(ShaderDialect, workspace_contract_and_stage) {
  static constexpr View::Bytes library_source =
      "// CPU and portable values.\n"
      "dialect : Library;"_view;
  static constexpr View::Bytes render_source =
      "// Render interface.\n"
      "dialect : Render;\n"
      "public Simple : struct {\n"
      "  @capability(\"fragment\")\n"
      "  public fragment : stage [@location(0).color : Cpu::R64] -> "
      "[@location(0).color : Cpu::R64];\n"
      "  @set(0) @slot(1) @read\n"
      "  public texture : resource Cpu::U64;\n"
      "}"_view;
  static constexpr View::Bytes shader_source =
      "// GPU implementation.\n"
      "dialect : Shader;\n"
      "public Test : shader Formats::Simple {\n"
      "  public gain : uniform Cpu::R64 = 0.0;\n"
      "  public fragment : func {\n"
      "    state copied : Cpu::R64 = color + color + parameters.gain;\n"
      "    return (.color = copied);\n"
      "  }\n"
      "  @direction(\"upload\") @marshal(\"copy\") @sync(\"submission\")\n"
      "  public count : bridge Cpu::U64 -> U64;\n"
      "}"_view;

  Environment::Toolchain toolchain;
  auto library = toolchain.install<Library::Dialect>("Library"_view);
  auto render = toolchain.install<Render::Dialect>("Render"_view);
  ASSERT(library && render);
  auto shader =
      toolchain.install<Shader::Dialect>("Shader"_view, *library, *render);
  ASSERT(shader);
  Environment::Workspace workspace(toolchain);
  Errors render_errors;
  Errors shader_errors;

  ASSERT(workspace.interpret_source(
      render_errors, "Cpu"_view, "cpu.ttx"_view, library_source));
  ASSERT(workspace.interpret_source(
      render_errors, "Formats"_view, "formats.ttx"_view, render_source));
  auto interpreted = workspace.interpret_source(
      shader_errors, "Programs"_view, "shader.ttx"_view, shader_source);

  ASSERT(interpreted && interpreted->is<Shader::Language::Monograph>());
  const auto& monograph =
      static_cast<const Shader::Language::Monograph&>(*interpreted);
  ASSERT(monograph.get_layer(*shader));
  EXPECT(&*monograph.get_layer(*shader) == &monograph);
  ASSERT(monograph.get_layer(*library));
  EXPECT(&*monograph.get_layer(*library) == &monograph.get_library());
  EXPECT_NOT(monograph.get_layer(*render));
  ASSERT_EQ(monograph.get_programs().get_size(), Count(1));
  ASSERT_EQ(monograph.get_bridges().get_size(), Count(1));
  const auto& program = monograph.get_programs().get_data()[0].get();
  EXPECT(program.get_contract());
  Shader::Language::Contract negotiator;
  EXPECT(negotiator.accepts(*program.get_contract(), program));
  EXPECT(program
             .resolve_type_call(
                 program, "fragment"_view,
                 Library::Language::Model::Type::Access::Static)
             .resolve()
             .is<Library::Language::Function>());
  EXPECT(program
             .resolve_type_access(
                 program, "texture"_view,
                 Library::Language::Model::Type::Access::Static)
             .resolve()
             .is<Library::Language::Field>());
  EXPECT_EQ(program.get_bindings().get_size(), Count(2));
  ASSERT_EQ(program.get_uniforms().get_size(), Count(1));
  EXPECT_EQ(program.get_parameters().get_layout().get_size(), Count(1));
  EXPECT(program.satisfies(*program.get_contract()));
  EXPECT(monograph.get_bridges().get_data()[0].get().get_cpu_type());
  EXPECT(monograph.get_bridges().get_data()[0].get().get_gpu_type());
  EXPECT(render_errors.is_empty());
  EXPECT(shader_errors.is_empty());
}

PERIMORTEM_UNIT_TEST(ShaderDialect, rejects_incomplete_contract) {
  static constexpr View::Bytes library_source =
      "// CPU and portable values.\n"
      "dialect : Library;"_view;
  static constexpr View::Bytes render_source =
      "// Required GPU surface.\n"
      "dialect : Render;\n"
      "public Required : struct {\n"
      "  public fragment : stage [] -> [];\n"
      "  @set(0) @slot(0) @read\n"
      "  public texture : resource Cpu::U64;\n"
      "}"_view;
  static constexpr View::Bytes shader_source =
      "// Missing required Stage body.\n"
      "dialect : Shader;\n"
      "public Broken : shader Formats::Required {\n"
      "  public seed : uniform Cpu::R64 = 0.0;\n"
      "}"_view;

  Environment::Toolchain toolchain;
  auto library = toolchain.install<Library::Dialect>("Library"_view);
  auto render = toolchain.install<Render::Dialect>("Render"_view);
  ASSERT(library && render);
  ASSERT(toolchain.install<Shader::Dialect>("Shader"_view, *library, *render));
  Environment::Workspace workspace(toolchain);
  Errors errors;

  ASSERT(workspace.interpret_source(
      errors, "Cpu"_view, "cpu.ttx"_view, library_source));
  ASSERT(workspace.interpret_source(
      errors, "Formats"_view, "required.ttx"_view, render_source));
  auto interpreted = workspace.interpret_source(
      errors, "Programs"_view, "broken-shader.ttx"_view, shader_source);

  EXPECT_NOT(interpreted);
  EXPECT_NOT(errors.is_empty());
  auto retained = workspace.get_monograph("broken-shader.ttx"_view);
  EXPECT(retained && retained->is<Shader::Language::Monograph>());
}
