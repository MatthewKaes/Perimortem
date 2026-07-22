// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/compiler/assembler/spir_v.hpp"
#include "tetrodotoxin/compiler/assembler/x86_64.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Compiler;
using namespace Validation;

static Harness TtxSpirV = {
  .name = "TTX::SPIR-V"_view,
};

static Harness Ttxx86_64 = {
  .name = "TTX::x86_64"_view,
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

PERIMORTEM_UNIT_TEST(Ttxx86_64, unary_encodings) {
  Dynamic::Bytes machine_code;
  Assembler::x86_64 assembler(machine_code);

  assembler.inc(Assembler::x86_64::Reg::EAX);
  assembler.inc(Assembler::x86_64::Reg::R8);
  assembler.dec(Assembler::x86_64::Reg::EAX);
  assembler.dec(Assembler::x86_64::Reg::R8);
  assembler.zero(Assembler::x86_64::Reg::EAX);
  assembler.zero(Assembler::x86_64::Reg::R8);
  assembler.one(Assembler::x86_64::Reg::EAX);
  assembler.one(Assembler::x86_64::Reg::R8);
  assembler.neg_one(Assembler::x86_64::Reg::RAX);
  assembler.neg_one(Assembler::x86_64::Reg::R8);

  EXPECT_HEX(
      machine_code,
      "\xFF\xC0\x49\xFF\xC0"
      "\xFF\xC8\x49\xFF\xC8"
      "\x31\xC0\x4D\x31\xC0"
      "\x31\xC0\xFF\xC0\x4D\x31\xC0\x49\xFF\xC0"
      "\x31\xC0\x48\xFF\xC8\x4D\x31\xC0\x49\xFF\xC8"_view);
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, register_moves) {
  Dynamic::Bytes machine_code;
  Assembler::x86_64 assembler(machine_code);

  assembler.mov(Assembler::x86_64::Reg::AL, Assembler::x86_64::Reg::SPL);
  assembler.mov(Assembler::x86_64::Reg::R8B, Assembler::x86_64::Reg::AL);
  assembler.mov(Assembler::x86_64::Reg::AX, Assembler::x86_64::Reg::R8W);
  assembler.mov(Assembler::x86_64::Reg::EAX, Assembler::x86_64::Reg::R8D);
  assembler.mov(Assembler::x86_64::Reg::RAX, Assembler::x86_64::Reg::R8);
  assembler.mov(Assembler::x86_64::Reg::R9, Assembler::x86_64::Reg::R8);

  EXPECT_HEX(
      machine_code,
      "\x40\x88\xC4"
      "\x44\x88\xC0"
      "\x66\x41\x89\xC0"
      "\x41\x89\xC0"
      "\x49\x89\xC0"
      "\x4D\x89\xC8"_view);
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, immediate_moves) {
  Dynamic::Bytes machine_code;
  Assembler::x86_64 assembler(machine_code);

  assembler.mov(Unsigned_64(0), Assembler::x86_64::Reg::RAX);
  assembler.mov(Unsigned_64(1), Assembler::x86_64::Reg::R8);
  assembler.mov(Unsigned_64(-1), Assembler::x86_64::Reg::RAX);
  assembler.mov(Unsigned_64(2), Assembler::x86_64::Reg::RAX);
  assembler.mov(Unsigned_64(0xFFFFFFFF), Assembler::x86_64::Reg::R8);
  assembler.mov(Unsigned_64(0xFFFFFFFF80000000), Assembler::x86_64::Reg::RAX);
  assembler.mov(Unsigned_64(0x100000000), Assembler::x86_64::Reg::R8);

  EXPECT_HEX(
      machine_code,
      "\x31\xC0"
      "\x4D\x31\xC0\x49\xFF\xC0"
      "\x31\xC0\x48\xFF\xC8"
      "\xB8\x02\x00\x00\x00"
      "\x41\xB8\xFF\xFF\xFF\xFF"
      "\x48\xC7\xC0\x00\x00\x00\x80"
      "\x49\xB8\x00\x00\x00\x00\x01\x00\x00\x00"_view);
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, memory_moves) {
  Dynamic::Bytes machine_code;
  Assembler::x86_64 assembler(machine_code);

  assembler.mov(
      Assembler::x86_64::Reg::RAX, Assembler::x86_64::Reg::RSP, Signed_32(8));
  assembler.mov(
      Assembler::x86_64::Reg::AL, Assembler::x86_64::Reg::RBP, Signed_32(0));
  assembler.mov(
      Assembler::x86_64::Reg::R8, Signed_32(8), Assembler::x86_64::Reg::RAX);
  assembler.mov(
      Assembler::x86_64::Reg::RBP, Signed_32(0), Assembler::x86_64::Reg::AX);

  EXPECT_HEX(
      machine_code,
      "\x48\x89\x44\x24\x08"
      "\x40\x88\x45\x00"
      "\x49\x8B\x40\x08"
      "\x66\x8B\x45\x00"_view);
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, call_relocation_slot) {
  Dynamic::Bytes machine_code;
  Assembler::x86_64 assembler(machine_code);

  assembler.call();

  EXPECT_HEX(machine_code, "\xE8\0\0\0\0"_view);
}
