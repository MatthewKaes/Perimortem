// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/spirv/assembler/spir_v.hpp"

#include "validation/process/child.hpp"
#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/package/archive/reader.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Terminal::Spirv;
using namespace Validation;

static Harness TtxSpirV = {
  .name = "TTX::SPIR-V"_view,
};

extern "C" {
extern const U8 TTX_DATA_Validation_2eShader__TestShader__Program[];
extern const U8 TTX_DATA_Validation_2eShader__TestShader__Program_end[];
extern const U8 TTX_DATA_Validation_2eShader__Float64Shader__Program[];
extern const U8 TTX_DATA_Validation_2eShader__Float64Shader__Program_end[];
}

PERIMORTEM_UNIT_TEST(TtxSpirV, word_emitter) {
  Dynamic::Bytes words;
  Assembler::SpirV assembler(words);

  assembler.begin_module(2);
  assembler.capability(Assembler::SpirV::Capability::Shader);
  assembler.memory_model(
      Assembler::SpirV::AddressingModel::Logical,
      Assembler::SpirV::MemoryModel::GLSL450);
  assembler.entry_point(
      Assembler::SpirV::ExecutionModel::Vertex, 1, "main"_view);

  constexpr Static::Bytes<60> expected = {{
    0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
  }};

  EXPECT_HEX(words, expected);
  EXPECT(Assembler::SpirV::is_valid_module(words));
}

PERIMORTEM_UNIT_TEST(TtxSpirV, unsigned_to_real_word_emitter) {
  Dynamic::Bytes words;
  Assembler::SpirV assembler(words);
  assembler.convert_u_to_f(1, 2, 3);

  constexpr Static::Bytes<16> expected = {{
    0x70,
    0x00,
    0x04,
    0x00,
    0x01,
    0x00,
    0x00,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x03,
    0x00,
    0x00,
    0x00,
  }};
  EXPECT_HEX(words, expected);
}

PERIMORTEM_UNIT_TEST(TtxSpirV, independent_validator) {
  View::Bytes modules[] = {
    View::Bytes(
        TTX_DATA_Validation_2eShader__TestShader__Program,
        Count(
            TTX_DATA_Validation_2eShader__TestShader__Program_end -
            TTX_DATA_Validation_2eShader__TestShader__Program)),
    View::Bytes(
        TTX_DATA_Validation_2eShader__Float64Shader__Program,
        Count(
            TTX_DATA_Validation_2eShader__Float64Shader__Program_end -
            TTX_DATA_Validation_2eShader__Float64Shader__Program)),
  };
  static constexpr Static::Vector<View::Bytes, 2> paths = {{
    ".bin/bin/validation/embedded_shader_u32.spv"_view,
    ".bin/bin/validation/embedded_shader_r64.spv"_view,
  }};
  for (Count index = 0; index < 2; index++) {
    ASSERT(Perimortem::System::File::write(modules[index], paths[index]));
    Static::Vector<View::Bytes, 3> arguments = {{
      "--target-env"_view,
      "vulkan1.0"_view,
      paths[index],
    }};
    Process::Request request = {
      .executable = "/usr/bin/spirv-val"_view,
      .arguments = arguments,
    };
    Process::Observation observation = Process::run(request);
    EXPECT(observation.launched);
    EXPECT_NOT(observation.timed_out);
    EXPECT_EQ(observation.exit_status, 0);
    EXPECT(observation.standard_output.is_empty());
    EXPECT(observation.standard_error.is_empty());
    EXPECT(observation.runner_error.is_empty());
    EXPECT(Perimortem::System::File::remove(paths[index]));
  }
}

PERIMORTEM_UNIT_TEST(TtxSpirV, package_locator) {
  auto bytes = Perimortem::System::File::read(
      ".bin/bin/validation/Validation.Shader/1.0/complete.txa"_view);
  ASSERT(bytes);
  Allocator::Arena arena;
  auto decoded =
      Tetrodotoxin::Package::Archive::Reader::read(arena, bytes->get_view());
  Option<Tetrodotoxin::Package::Archive::Archive> archive;
  decoded.visit(
      [&](const Tetrodotoxin::Package::Archive::Archive& selected) {
        archive = selected;
      },
      [](const Tetrodotoxin::Package::Archive::Reader::Error&) {});
  ASSERT(archive);

  Bool found_u32 = False;
  Bool found_r64 = False;
  for (const Tetrodotoxin::Package::Archive::Export& exported :
       archive->get_exports()) {
    found_u32 |= exported.get_semantic_route() == "TestShader::Program"_view &&
                 exported.get_artifact_id() == "x86_64-sysv-linux"_view &&
                 exported.get_symbol_locator() ==
                     "TTX_DATA_Validation_2eShader__TestShader__Program"_view;
    found_r64 |=
        exported.get_semantic_route() == "Float64Shader::Program"_view &&
        exported.get_artifact_id() == "x86_64-sysv-linux"_view &&
        exported.get_symbol_locator() ==
            "TTX_DATA_Validation_2eShader__Float64Shader__Program"_view;
  }
  EXPECT(found_u32);
  EXPECT(found_r64);
}
