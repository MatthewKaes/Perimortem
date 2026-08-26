// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/package/archive/reader.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/render/dialect.hpp"
#include "tetrodotoxin/render/language/monograph.hpp"
#include "tetrodotoxin/render/language/stage.hpp"
#include "tetrodotoxin/render/language/structure.hpp"
#include "tetrodotoxin/shader/dialect.hpp"
#include "tetrodotoxin/shader/language/monograph.hpp"
#include "tetrodotoxin/shader/language/program.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness ShaderArchive = {
  .name = "Tetrodotoxin::Shader::Archive"_view,
  .setup = []() { Diagnostics::Log::set_sink(Diagnostics::Log::plain_sink); },
  .teardown =
      []() { Diagnostics::Log::set_sink(Diagnostics::Log::default_sink); },
};

static auto decode(Allocator::Arena& arena, View::Bytes bytes)
    -> Option<Package::Archive::Archive> {
  return Package::Archive::Reader::read(arena, bytes)
      .visit(
          [](const Package::Archive::Archive& selected)
              -> Option<Package::Archive::Archive> { return selected; },
          [](const Package::Archive::Reader::Error&)
              -> Option<Package::Archive::Archive> { return {}; });
}

static auto prove_profile(
    const Package::Archive::Archive& math,
    const Package::Archive::Archive& shader,
    Test::TestResult& result) -> void {
  Environment::Toolchain toolchain;
  auto library = toolchain.install<Library::Dialect>("Library"_view);
  auto render = toolchain.install<Render::Dialect>("Render"_view);
  ASSERT(toolchain.install<Package::Dialect>("Package"_view));
  ASSERT(library && render);
  ASSERT(toolchain.install<Shader::Dialect>("Shader"_view, *library, *render));
  Environment::Workspace workspace(toolchain);
  ASSERT(workspace.restore_package(math, "Math"_view));
  ASSERT(workspace.restore_package(shader, "ShaderProduct"_view));

  auto package = workspace.resolve_context("ShaderProduct"_view)
                     .resolve()
                     .select<Package::Language::Monograph>();
  auto format = package ? package->resolve_context("Formats"_view)
                              .resolve()
                              .select<Render::Language::Monograph>()
                        : Option<const Render::Language::Monograph&>();
  auto shader_member = package ? package->resolve_context("Shader"_view)
                                     .resolve()
                                     .select<Shader::Language::Monograph>()
                               : Option<const Shader::Language::Monograph&>();
  auto program = shader_member
                     ? shader_member->resolve_context("TestShader"_view)
                           .resolve()
                           .select<Shader::Language::Program>()
                     : Option<const Shader::Language::Program&>();
  ASSERT(package && format && shader_member && program);
  Count format_type_count =
      shader.get_profile() == Language::Persistence::Profile::Complete ? 2 : 1;
  EXPECT_EQ(format->get_types().get_size(), format_type_count);

  auto contract = format->resolve_context("Simple"_view)
                      .resolve()
                      .select<Render::Language::Structure>();
  auto selected_contract = program->get_contract();
  auto function = program
                      ->resolve_type_call(
                          *program, "fragment"_view,
                          Library::Language::Model::Type::Access::Static)
                      .resolve()
                      .select<Library::Language::Function>();
  auto stage = contract ? contract->resolve_call(*contract, "fragment"_view)
                              .resolve()
                              .select<Render::Language::Stage>()
                        : Option<const Render::Language::Stage&>();
  ASSERT(contract && selected_contract);
  EXPECT(&*contract == &*selected_contract);
  ASSERT(function && stage);
  ASSERT_EQ(program->get_uniforms().get_size(), Count(1));
  EXPECT_EQ(program->get_parameters().get_layout().get_size(), Count(1));
  EXPECT(
      &program->get_instance_parameters_field().get_type() ==
      &program->get_parameters());
  EXPECT(program->satisfies(*contract));
  EXPECT_NOT(function->get_body());
  EXPECT_EQ(function->get_signature().get_parameters().get_size(), Count(1));
  EXPECT_EQ(function->get_signature().get_results().get_size(), Count(1));
  EXPECT_EQ(stage->get_parameter_layout().get_size(), Count(1));
  EXPECT_EQ(stage->get_result_layout().get_size(), Count(1));
}

PERIMORTEM_UNIT_TEST(ShaderArchive, source_free_profiles) {
  auto math_bytes =
      File::read(".bin/bin/packages/ttx/Perimortem.Math/1.0/contract.txa"_view);
  auto complete_bytes =
      File::read(".bin/bin/validation/Validation.Shader/1.0/complete.txa"_view);
  auto contract_bytes =
      File::read(".bin/bin/validation/Validation.Shader/1.0/contract.txa"_view);
  ASSERT(math_bytes && complete_bytes && contract_bytes);

  Allocator::Arena arena;
  auto math = decode(arena, *math_bytes);
  auto complete = decode(arena, *complete_bytes);
  auto contract = decode(arena, *contract_bytes);
  ASSERT(math && complete && contract);
  prove_profile(*math, *complete, result);
  prove_profile(*math, *contract, result);
}

PERIMORTEM_UNIT_TEST(ShaderArchive, rejects_corrupt_payloads) {
  auto archive_bytes =
      File::read(".bin/bin/validation/Validation.Shader/1.0/complete.txa"_view);
  ASSERT(archive_bytes);
  Allocator::Arena archive_arena;
  auto archive = decode(archive_arena, *archive_bytes);
  ASSERT(archive);

  Library::Dialect library;
  Render::Dialect render;
  Shader::Dialect shader(library, render);
  Package::Dialect package;
  Count checked = 0;
  for (const Package::Archive::Member& member : archive->get_members()) {
    if (member.get_dialect_name() != "Render"_view &&
        member.get_dialect_name() != "Shader"_view) {
      continue;
    }
    Dynamic::Bytes corrupted(member.get_payload());
    ASSERT_NOT(corrupted.is_empty());
    corrupted.get_access().get_data()[0] ^= U8(0xFF);
    Allocator::Arena graph_arena;
    auto restored =
        member.get_dialect_name() == "Render"_view
            ? render.restore(
                  graph_arena, corrupted,
                  Language::Persistence::Profile::Complete,
                  Ttx::Concept::Documentation::get_empty(), package)
            : shader.restore(
                  graph_arena, corrupted,
                  Language::Persistence::Profile::Complete,
                  Ttx::Concept::Documentation::get_empty(), package);
    EXPECT_NOT(restored);
    checked++;
  }
  EXPECT_EQ(checked, Count(2));
}
