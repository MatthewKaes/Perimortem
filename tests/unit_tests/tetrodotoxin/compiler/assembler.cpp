// Perimortem Engine
// Copyright © Matt Kaes

// End-to-end smoke test: call a function from a ttx_library.
// Build with:  bazel run //tetrodotoxin:hello

#include "validation/unit_test.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/utility/null_terminated.hpp"

#include "tetrodotoxin/compiler/assembler/x86_64.hpp"
#include "ttx/ttx_tests.hpp"

using namespace Validation;

using namespace Tetrodotoxin::Compiler::Assembler;

Test::Harness Ttxx86_64 = {.name = "TTX::x86_64"};

PERIMORTEM_UNIT_TEST(Ttxx86_64, inc) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  // Perform all the 32 bit operations.
  for (x86_64::Reg reg = x86_64::Reg::EAX; reg <= x86_64::Reg::R15D;
       reg = x86_64::Reg(Byte(reg) + 1)) {
    assembler.inc(reg);
  }

  EXPECT_HEX(
      machine_code.get_view(),
      "\xFF\xC0\xFF\xC1\xFF\xC2\xFF\xC3\xFF\xC4\xFF\xC5\xFF\xC6\xFF\xC7\x41\xFF"
      "\xC0\x41\xFF\xC1\x41\xFF\xC2\x41\xFF\xC3\x41\xFF\xC4\x41\xFF\xC5\x41\xFF"
      "\xC6\x41\xFF\xC7"_view);

  // Perform all 64 bit operations.
  machine_code.clear();
  for (x86_64::Reg reg = x86_64::Reg::RAX; reg <= x86_64::Reg::R15;
       reg = x86_64::Reg(Byte(reg) + 1)) {
    assembler.inc(reg);
  }
  EXPECT_HEX(
      machine_code.get_view(),
      "\x48\xFF\xC0\x48\xFF\xC1\x48\xFF\xC2\x48\xFF\xC3\x48\xFF\xC4\x48\xFF\xC5"
      "\x48\xFF\xC6\x48\xFF\xC7\x49\xFF\xC0\x49\xFF\xC1\x49\xFF\xC2\x49\xFF\xC3"
      "\x49\xFF\xC4\x49\xFF\xC5\x49\xFF\xC6\x49\xFF\xC7"_view);
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, dec) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  // Perform all the 32 bit operations.
  for (x86_64::Reg reg = x86_64::Reg::EAX; reg <= x86_64::Reg::R15D;
       reg = x86_64::Reg(Byte(reg) + 1)) {
    assembler.dec(reg);
  }

  EXPECT_HEX(
      machine_code.get_view(),
      "\xFF\xC8\xFF\xC9\xFF\xCA\xFF\xCB\xFF\xCC\xFF\xCD\xFF\xCE\xFF\xCF\x41\xFF"
      "\xC8\x41\xFF\xC9\x41\xFF\xCA\x41\xFF\xCB\x41\xFF\xCC\x41\xFF\xCD\x41\xFF"
      "\xCE\x41\xFF\xCF"_view);

  // Perform all 64 bit operations.
  machine_code.clear();
  for (x86_64::Reg reg = x86_64::Reg::RAX; reg <= x86_64::Reg::R15;
       reg = x86_64::Reg(Byte(reg) + 1)) {
    assembler.dec(reg);
  }
  EXPECT_HEX(
      machine_code.get_view(),
      "\x48\xFF\xC8\x48\xFF\xC9\x48\xFF\xCA\x48\xFF\xCB\x48\xFF\xCC\x48\xFF\xCD"
      "\x48\xFF\xCE\x48\xFF\xCF\x49\xFF\xC8\x49\xFF\xC9\x49\xFF\xCA\x49\xFF\xCB"
      "\x49\xFF\xCC\x49\xFF\xCD\x49\xFF\xCE\x49\xFF\xCF"_view);
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, zero) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  // Perform all the 32 bit operations.
  for (x86_64::Reg reg = x86_64::Reg::EAX; reg <= x86_64::Reg::R15D;
       reg = x86_64::Reg(Byte(reg) + 1)) {
    assembler.zero(reg);
  }

  EXPECT_HEX(
      machine_code.get_view(),
      "\x31\xC0\x31\xC9\x31\xD2\x31\xDB\x31\xE4\x31\xED\x31\xF6\x31\xFF\x45\x31"
      "\xC0\x45\x31\xC9\x45\x31\xD2\x45\x31\xDB\x45\x31\xE4\x45\x31\xED\x45\x31"
      "\xF6\x45\x31\xFF"_view);

  // Perform all 64 bit operations.
  machine_code.clear();
  for (x86_64::Reg reg = x86_64::Reg::RAX; reg <= x86_64::Reg::R15;
       reg = x86_64::Reg(Byte(reg) + 1)) {
    assembler.zero(reg);
  }
  // Should be the same as the 64 bit operations.
  EXPECT_HEX(
      machine_code.get_view(),
      "\x31\xC0\x31\xC9\x31\xD2\x31\xDB\x31\xE4\x31\xED\x31\xF6\x31\xFF\x4D\x31"
      "\xC0\x4D\x31\xC9\x4D\x31\xD2\x4D\x31\xDB\x4D\x31\xE4\x4D\x31\xED\x4D\x31"
      "\xF6\x4D\x31\xFF"_view);
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, one) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  // Perform all the 32 bit operations.
  for (x86_64::Reg reg = x86_64::Reg::EAX; reg <= x86_64::Reg::R15D;
       reg = x86_64::Reg(Byte(reg) + 1)) {
    assembler.one(reg);
  }

  EXPECT_HEX(
      machine_code.get_view(),
      "\x31\xC0\xFF\xC0\x31\xC9\xFF\xC1\x31\xD2\xFF\xC2\x31\xDB\xFF\xC3\x31\xE4"
      "\xFF\xC4\x31\xED\xFF\xC5\x31\xF6\xFF\xC6\x31\xFF\xFF\xC7\x45\x31\xC0\x41"
      "\xFF\xC0\x45\x31\xC9\x41\xFF\xC1\x45\x31\xD2\x41\xFF\xC2\x45\x31\xDB\x41"
      "\xFF\xC3\x45\x31\xE4\x41\xFF\xC4\x45\x31\xED\x41\xFF\xC5\x45\x31\xF6\x41"
      "\xFF\xC6\x45\x31\xFF\x41\xFF\xC7"_view);

  // Perform all 64 bit operations.
  machine_code.clear();
  for (x86_64::Reg reg = x86_64::Reg::RAX; reg <= x86_64::Reg::R15;
       reg = x86_64::Reg(Byte(reg) + 1)) {
    assembler.one(reg);
  }
  EXPECT_HEX(
      machine_code.get_view(),
      "\x31\xC0\xFF\xC0\x31\xC9\xFF\xC1\x31\xD2\xFF\xC2\x31\xDB\xFF\xC3\x31\xE4"
      "\xFF\xC4\x31\xED\xFF\xC5\x31\xF6\xFF\xC6\x31\xFF\xFF\xC7\x4D\x31\xC0\x49"
      "\xFF\xC0\x4D\x31\xC9\x49\xFF\xC1\x4D\x31\xD2\x49\xFF\xC2\x4D\x31\xDB\x49"
      "\xFF\xC3\x4D\x31\xE4\x49\xFF\xC4\x4D\x31\xED\x49\xFF\xC5\x4D\x31\xF6\x49"
      "\xFF\xC6\x4D\x31\xFF\x49\xFF\xC7"_view);
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, neg_one) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  // Perform all the 32 bit operations.
  for (x86_64::Reg reg = x86_64::Reg::EAX; reg <= x86_64::Reg::R15D;
       reg = x86_64::Reg(Byte(reg) + 1)) {
    assembler.neg_one(reg);
  }

  EXPECT_HEX(
      machine_code.get_view(),
      "\x31\xC0\xFF\xC8\x31\xC9\xFF\xC9\x31\xD2\xFF\xCA\x31\xDB\xFF\xCB\x31\xE4"
      "\xFF\xCC\x31\xED\xFF\xCD\x31\xF6\xFF\xCE\x31\xFF\xFF\xCF\x45\x31\xC0\x41"
      "\xFF\xC8\x45\x31\xC9\x41\xFF\xC9\x45\x31\xD2\x41\xFF\xCA\x45\x31\xDB\x41"
      "\xFF\xCB\x45\x31\xE4\x41\xFF\xCC\x45\x31\xED\x41\xFF\xCD\x45\x31\xF6\x41"
      "\xFF\xCE\x45\x31\xFF\x41\xFF\xCF"_view);

  // Perform all 64 bit operations.
  machine_code.clear();
  for (x86_64::Reg reg = x86_64::Reg::RAX; reg <= x86_64::Reg::R15;
       reg = x86_64::Reg(Byte(reg) + 1)) {
    assembler.neg_one(reg);
  }
  // Neg one can't optimize for the lower 3 bit registers since dec needs to
  // wrap around.
  EXPECT_HEX(
      machine_code.get_view(),
      "\x31\xC0\x48\xFF\xC8\x31\xC9\x48\xFF\xC9\x31\xD2\x48\xFF\xCA\x31\xDB\x48"
      "\xFF\xCB\x31\xE4\x48\xFF\xCC\x31\xED\x48\xFF\xCD\x31\xF6\x48\xFF\xCE\x31"
      "\xFF\x48\xFF\xCF\x4D\x31\xC0\x49\xFF\xC8\x4D\x31\xC9\x49\xFF\xC9\x4D\x31"
      "\xD2\x49\xFF\xCA\x4D\x31\xDB\x49\xFF\xCB\x4D\x31\xE4\x49\xFF\xCC\x4D\x31"
      "\xED\x49\xFF\xCD\x4D\x31\xF6\x49\xFF\xCE\x4D\x31\xFF\x49\xFF\xCF"_view);
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_reg_to_reg_8bit) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  for (x86_64::Reg reg = x86_64::Reg::AL; reg <= x86_64::Reg::R15B;
       reg = x86_64::Reg(Byte(reg) + 1)) {
    assembler.mov(reg, reg);
  }
  // AL-BL: no REX. SPL-DIL: bare REX 0x40. R8B-R15B: REX.R|REX.B = 0x45.
  EXPECT_HEX(
      machine_code.get_view(),
      "\x88\xC0\x88\xC9\x88\xD2\x88\xDB"                  // AL CL DL BL
      "\x40\x88\xE4\x40\x88\xED\x40\x88\xF6\x40\x88\xFF"  // SPL BPL SIL DIL
      "\x45\x88\xC0\x45\x88\xC9\x45\x88\xD2\x45\x88\xDB"  // R8B-R11B
      "\x45\x88\xE4\x45\x88\xED\x45\x88\xF6\x45\x88\xFF"_view);  // R12B-R15B
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_reg_to_reg_16bit) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  for (x86_64::Reg reg = x86_64::Reg::AX; reg <= x86_64::Reg::R15W;
       reg = x86_64::Reg(Byte(reg) + 1)) {
    assembler.mov(reg, reg);
  }
  // AX-DI: 0x66 prefix, no REX. R8W-R15W: 0x66 + REX.R|REX.B = 0x45.
  EXPECT_HEX(
      machine_code.get_view(),
      "\x66\x89\xC0\x66\x89\xC9\x66\x89\xD2\x66\x89\xDB"  // AX CX DX BX
      "\x66\x89\xE4\x66\x89\xED\x66\x89\xF6\x66\x89\xFF"  // SP BP SI DI
      "\x66\x45\x89\xC0\x66\x45\x89\xC9\x66\x45\x89\xD2\x66\x45\x89\xDB"  // R8W-R11W
      "\x66\x45\x89\xE4\x66\x45\x89\xED\x66\x45\x89\xF6\x66\x45\x89\xFF"_view);  // R12W-R15W
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_reg_to_reg_32bit) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  for (x86_64::Reg reg = x86_64::Reg::EAX; reg <= x86_64::Reg::R15D;
       reg = x86_64::Reg(Byte(reg) + 1)) {
    assembler.mov(reg, reg);
  }
  // EAX-EDI: no REX prefix. R8D-R15D: REX.R|REX.B = 0x45.
  EXPECT_HEX(
      machine_code.get_view(),
      "\x89\xC0\x89\xC9\x89\xD2\x89\xDB\x89\xE4\x89\xED\x89\xF6\x89\xFF"  // EAX-EDI
      "\x45\x89\xC0\x45\x89\xC9\x45\x89\xD2\x45\x89\xDB"         // R8D-R11D
      "\x45\x89\xE4\x45\x89\xED\x45\x89\xF6\x45\x89\xFF"_view);  // R12D-R15D
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_reg_to_reg_64bit) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  for (x86_64::Reg reg = x86_64::Reg::RAX; reg <= x86_64::Reg::R15;
       reg = x86_64::Reg(Byte(reg) + 1)) {
    assembler.mov(reg, reg);
  }
  // RAX-RDI: REX.W = 0x48. R8-R15: REX.W|REX.R|REX.B = 0x4D.
  EXPECT_HEX(
      machine_code.get_view(),
      "\x48\x89\xC0\x48\x89\xC9\x48\x89\xD2\x48\x89\xDB"         // RAX-RBX
      "\x48\x89\xE4\x48\x89\xED\x48\x89\xF6\x48\x89\xFF"         // RSP-RDI
      "\x4D\x89\xC0\x4D\x89\xC9\x4D\x89\xD2\x4D\x89\xDB"         // R8-R11
      "\x4D\x89\xE4\x4D\x89\xED\x4D\x89\xF6\x4D\x89\xFF"_view);  // R12-R15
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_r8_imm8) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  for (x86_64::Reg reg = x86_64::Reg::AL; reg <= x86_64::Reg::R15B;
       reg = x86_64::Reg(Byte(reg) + 1)) {
    assembler.mov(Byte(0x42), reg);
  }
  // AL-BL: B0+rd + imm8 (2 bytes). SPL-DIL: bare REX(0x40) + B4+rd + imm8 (3
  // bytes). R8B-R15B: REX.B(0x41) + B0+rd + imm8 (3 bytes).
  EXPECT_HEX(
      machine_code.get_view(),
      "\xB0\x42\xB1\x42\xB2\x42\xB3\x42"                  // AL CL DL BL
      "\x40\xB4\x42\x40\xB5\x42\x40\xB6\x42\x40\xB7\x42"  // SPL BPL SIL DIL
      "\x41\xB0\x42\x41\xB1\x42\x41\xB2\x42\x41\xB3\x42"  // R8B-R11B
      "\x41\xB4\x42\x41\xB5\x42\x41\xB6\x42\x41\xB7\x42"_view);  // R12B-R15B
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_r16_imm16) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  for (x86_64::Reg reg = x86_64::Reg::AX; reg <= x86_64::Reg::R15W;
       reg = x86_64::Reg(Byte(reg) + 1)) {
    assembler.mov(Bits_16(0x1234), reg);
  }
  // AX-DI: 0x66 + B8+rd + imm16 (4 bytes). R8W-R15W: 0x66 + REX.B(0x41) + B8+rd
  // + imm16 (5 bytes).
  EXPECT_HEX(
      machine_code.get_view(),
      "\x66\xB8\x34\x12\x66\xB9\x34\x12\x66\xBA\x34\x12\x66\xBB\x34\x12"
      "\x66\xBC\x34\x12\x66\xBD\x34\x12\x66\xBE\x34\x12\x66\xBF\x34\x12"
      "\x66\x41\xB8\x34\x12\x66\x41\xB9\x34\x12\x66\x41\xBA\x34\x12\x66\x41\xBB"
      "\x34\x12"
      "\x66\x41\xBC\x34\x12\x66\x41\xBD\x34\x12\x66\x41\xBE\x34\x12\x66\x41\xBF\x34\x12"_view);
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_r32_imm32) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  for (x86_64::Reg reg = x86_64::Reg::EAX; reg <= x86_64::Reg::R15D;
       reg = x86_64::Reg(Byte(reg) + 1)) {
    assembler.mov(Bits_32(0x12345678), reg);
  }
  // EAX-EDI: B8+rd, imm32 (5 bytes). R8D-R15D: REX.B(0x41) + B8+rd, imm32 (6
  // bytes).
  EXPECT_HEX(
      machine_code.get_view(),
      "\xB8\x78\x56\x34\x12\xB9\x78\x56\x34\x12\xBA\x78\x56\x34\x12\xBB\x78\x56"
      "\x34\x12"
      "\xBC\x78\x56\x34\x12\xBD\x78\x56\x34\x12\xBE\x78\x56\x34\x12\xBF\x78\x56"
      "\x34\x12"
      "\x41\xB8\x78\x56\x34\x12\x41\xB9\x78\x56\x34\x12\x41\xBA\x78\x56\x34\x12"
      "\x41\xBB\x78\x56\x34\x12"
      "\x41\xBC\x78\x56\x34\x12\x41\xBD\x78\x56\x34\x12\x41\xBE\x78\x56\x34\x12\x41\xBF\x78\x56\x34\x12"_view);
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_r64_imm64) {
  // Tetrodotoxin automatically tries to optimize 64bit mov encodings depending
  // on the input constant.
  // This is a basically free optimization at the codegen layer, but it does
  // require a lot of extra tests to make sure all the special cases work out.
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);

  // Value 0: XOR (via zero)
  assembler.mov(Bits_64(0), x86_64::Reg::RAX);
  assembler.mov(Bits_64(0), x86_64::Reg::R8);

  // Value 1: XOR + INC (via one)
  assembler.mov(Bits_64(1), x86_64::Reg::RAX);
  assembler.mov(Bits_64(1), x86_64::Reg::R8);

  // Value -1: XOR + DEC (via neg_one)
  assembler.mov(Bits_64(0xFFFFFFFFFFFFFFFF), x86_64::Reg::RAX);
  assembler.mov(Bits_64(0xFFFFFFFFFFFFFFFF), x86_64::Reg::R8);

  // Small uint32: zero-extending MOV r32, imm32 (no REX for RAX, REX.B for R8)
  assembler.mov(Bits_64(2), x86_64::Reg::RAX);
  assembler.mov(Bits_64(2), x86_64::Reg::R8);

  // Max uint32 (0xFFFFFFFF): still zero-extending, not MOVABS
  assembler.mov(Bits_64(0xFFFFFFFF), x86_64::Reg::RAX);
  assembler.mov(Bits_64(0xFFFFFFFF), x86_64::Reg::R8);

  // INT32_MIN (-2147483648): sign-extending MOV r/m64, imm32 (REX.W, opcode C7
  // /0)
  assembler.mov(Bits_64(0xFFFFFFFF80000000ULL), x86_64::Reg::RAX);
  assembler.mov(Bits_64(0xFFFFFFFF80000000ULL), x86_64::Reg::R8);

  // -2: sign-extending, just above the -1 special case
  assembler.mov(Bits_64(0xFFFFFFFFFFFFFFFE), x86_64::Reg::RAX);
  assembler.mov(Bits_64(0xFFFFFFFFFFFFFFFE), x86_64::Reg::R8);

  // Large positive (> uint32): MOVABS r64, imm64
  assembler.mov(Bits_64(0x100000000), x86_64::Reg::RAX);
  assembler.mov(Bits_64(0x100000000), x86_64::Reg::R8);

  EXPECT_HEX(
      machine_code.get_view(),
      // 0 → zero(RAX): XOR EAX,EAX (31 C0) — 32-bit write zero-extends to RAX
      //     zero(R8):  REX.W|R|B(4D) XOR(31) C0 — R8 needs full REX
      "\x31\xC0"
      "\x4D\x31\xC0"
      // 1 → one(RAX): XOR EAX,EAX + INC EAX (zero-extends to RAX=1)
      //     one(R8):   zero(R8) + REX.W|B(49) INC(FF) C0
      "\x31\xC0\xFF\xC0"
      "\x4D\x31\xC0\x49\xFF\xC0"
      // -1 → neg_one(RAX): XOR EAX,EAX + REX.W(48) DEC RAX (needs REX.W to wrap
      // 64-bit)
      //      neg_one(R8):   zero(R8) + REX.W|B(49) DEC(FF) C8
      "\x31\xC0\x48\xFF\xC8"
      "\x4D\x31\xC0\x49\xFF\xC8"
      // 2 → MOV EAX, 2 (B8+0, imm32, zero-extends); MOV R8D, 2 (REX.B=41, B8+0,
      // imm32)
      "\xB8\x02\x00\x00\x00"
      "\x41\xB8\x02\x00\x00\x00"
      // 0xFFFFFFFF → MOV EAX, 0xFFFFFFFF; MOV R8D, 0xFFFFFFFF
      "\xB8\xFF\xFF\xFF\xFF"
      "\x41\xB8\xFF\xFF\xFF\xFF"
      // INT32_MIN → MOV RAX, imm32 (sign-ext): REX.W(48) C7 /0(C0) 00 00 00 80
      //           → MOV R8,  imm32 (sign-ext): REX.W|B(49) C7 C0 00 00 00 80
      "\x48\xC7\xC0\x00\x00\x00\x80"
      "\x49\xC7\xC0\x00\x00\x00\x80"
      // -2 → MOV RAX, imm32 (sign-ext): REX.W(48) C7 C0 FE FF FF FF
      //    → MOV R8,  imm32 (sign-ext): REX.W|B(49) C7 C0 FE FF FF FF
      "\x48\xC7\xC0\xFE\xFF\xFF\xFF"
      "\x49\xC7\xC0\xFE\xFF\xFF\xFF"
      // 0x100000000 → MOVABS RAX, imm64: REX.W(48) B8 00 00 00 00 01 00 00 00
      //             → MOVABS R8,  imm64: REX.W|B(49) B8 00 00 00 00 01 00 00 00
      "\x48\xB8\x00\x00\x00\x00\x01\x00\x00\x00"
      "\x49\xB8\x00\x00\x00\x00\x01\x00\x00\x00"_view);
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_store_to_memory) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  // Store RAX to [RBX] — no displacement
  assembler.mov(x86_64::Reg::RAX, x86_64::Reg::RBX, SignedBits_32(0));
  // Store RAX to [RBX+8] — 8-bit displacement
  assembler.mov(x86_64::Reg::RAX, x86_64::Reg::RBX, SignedBits_32(8));
  // Store RAX to [RBP+0] — RBP forces 8-bit disp even when disp=0
  assembler.mov(x86_64::Reg::RAX, x86_64::Reg::RBP, SignedBits_32(0));
  // Store RAX to [RSP+8] — RSP requires SIB byte
  assembler.mov(x86_64::Reg::RAX, x86_64::Reg::RSP, SignedBits_32(8));
  // Store R8 to [RBX+8] — extended source register
  assembler.mov(x86_64::Reg::R8, x86_64::Reg::RBX, SignedBits_32(8));
  EXPECT_HEX(
      machine_code.get_view(),
      "\x48\x89\x03"             // RAX → [RBX]
      "\x48\x89\x43\x08"         // RAX → [RBX+8]
      "\x48\x89\x45\x00"         // RAX → [RBP+0]
      "\x48\x89\x44\x24\x08"     // RAX → [RSP+8] (SIB)
      "\x4C\x89\x43\x08"_view);  // R8 → [RBX+8] (REX.W+REX.R)
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_load_from_memory) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  // Load [RBX] to RAX — no displacement
  assembler.mov(x86_64::Reg::RBX, SignedBits_32(0), x86_64::Reg::RAX);
  // Load [RBX+8] to RAX — 8-bit displacement
  assembler.mov(x86_64::Reg::RBX, SignedBits_32(8), x86_64::Reg::RAX);
  // Load [RSP+8] to RAX — RSP base requires SIB byte
  assembler.mov(x86_64::Reg::RSP, SignedBits_32(8), x86_64::Reg::RAX);
  // Load [R8+8] to RAX — extended base register
  assembler.mov(x86_64::Reg::R8, SignedBits_32(8), x86_64::Reg::RAX);
  EXPECT_HEX(
      machine_code.get_view(),
      "\x48\x8B\x03"             // [RBX] → RAX
      "\x48\x8B\x43\x08"         // [RBX+8] → RAX
      "\x48\x8B\x44\x24\x08"     // [RSP+8] → RAX (SIB)
      "\x49\x8B\x40\x08"_view);  // [R8+8] → RAX (REX.W+REX.B)
}
