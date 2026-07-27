// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/shader/assembler/spir_v.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Shader;
using namespace Validation;

static Harness TtxSpirV = {
  .name = "TTX::SPIR-V"_view,
};

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

  constexpr Static::Bytes<60> expected = {
    0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
  };

  EXPECT_HEX(words, expected);
  EXPECT(Assembler::SpirV::is_valid_module(words));
}
