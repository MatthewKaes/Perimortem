// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/compiler/assembler/spirv.hpp"
#include "tetrodotoxin/compiler/assembler/x86_64.hpp"

using namespace Perimortem::Core;
using namespace Validation;

using namespace Tetrodotoxin::Compiler::Assembler;

static Harness TtxSpirV = {
  .name = "TTX::SPIR-V"_view,
};

static Harness Ttxx86_64 = {
  .name = "TTX::x86_64"_view,
};

PERIMORTEM_UNIT_TEST(TtxSpirV, module_header) {
  Perimortem::Memory::Dynamic::Bytes words;
  spirv assembler(words);

  assembler.begin_module(7);

  constexpr Static::Bytes<20> expected = {
    0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  EXPECT_HEX(words, expected);
  EXPECT(spirv::is_valid_module(words));
}

PERIMORTEM_UNIT_TEST(TtxSpirV, module_instructions) {
  Perimortem::Memory::Dynamic::Bytes words;
  spirv assembler(words);

  assembler.begin_module(2);
  assembler.capability(spirv::Capability::Shader);
  assembler.memory_model(
      spirv::AddressingModel::Logical, spirv::MemoryModel::GLSL450);
  assembler.entry_point(spirv::ExecutionModel::Vertex, 1, "main"_view);

  constexpr Static::Bytes<60> expected = {
    0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00,
  };
  EXPECT_HEX(words, expected);
  EXPECT(spirv::literal_string_word_count("main"_view) == 2);
  EXPECT(spirv::is_valid_module(words));
}

PERIMORTEM_UNIT_TEST(TtxSpirV, void_function) {
  Perimortem::Memory::Dynamic::Bytes words;
  spirv assembler(words);

  assembler.type_void(2);
  assembler.type_function(3, 2);
  assembler.function(2, 1, spirv::FunctionControl::None, 3);
  assembler.label(4);
  assembler.return_void();
  assembler.function_end();

  constexpr Static::Bytes<56> expected = {
    0x13, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00,
    0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00,
  };
  EXPECT_HEX(words, expected);
}

PERIMORTEM_UNIT_TEST(TtxSpirV, instruction_bounds) {
  Perimortem::Memory::Dynamic::Bytes words;
  spirv assembler(words);
  assembler.begin_module(2);
  assembler.instruction(spirv::Op::Nop, 1);

  EXPECT(spirv::is_valid_module(words));

  words.clear();
  assembler.begin_module(2);
  assembler.instruction(spirv::Op::Nop, 2);

  EXPECT_NOT(spirv::is_valid_module(words));
}

PERIMORTEM_UNIT_TEST(TtxSpirV, bad_headers) {
  EXPECT_NOT(spirv::is_valid_module(View::Bytes()));

  constexpr Static::Bytes<20> bad_magic = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  EXPECT_NOT(spirv::is_valid_module(bad_magic));

  Perimortem::Memory::Dynamic::Bytes bad_bound;
  spirv assembler(bad_bound);
  assembler.begin_module(0);
  EXPECT_NOT(spirv::is_valid_module(bad_bound));
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, inc) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  // Perform all the 32 bit operations.
  for (x86_64::Reg reg = x86_64::Reg::EAX; reg <= x86_64::Reg::R15D;
       reg = x86_64::Reg(Bits_8(reg) + 1)) {
    assembler.inc(reg);
  }

  EXPECT_HEX(
      machine_code,
      "\xFF\xC0\xFF\xC1\xFF\xC2\xFF\xC3\xFF\xC4\xFF\xC5\xFF\xC6\xFF\xC7\x41\xFF"
      "\xC0\x41\xFF\xC1\x41\xFF\xC2\x41\xFF\xC3\x41\xFF\xC4\x41\xFF\xC5\x41\xFF"
      "\xC6\x41\xFF\xC7"_view);

  // Perform all 64 bit operations.
  machine_code.clear();
  for (x86_64::Reg reg = x86_64::Reg::RAX; reg <= x86_64::Reg::R15;
       reg = x86_64::Reg(Bits_8(reg) + 1)) {
    assembler.inc(reg);
  }
  EXPECT_HEX(
      machine_code,
      "\x48\xFF\xC0\x48\xFF\xC1\x48\xFF\xC2\x48\xFF\xC3\x48\xFF\xC4\x48\xFF\xC5"
      "\x48\xFF\xC6\x48\xFF\xC7\x49\xFF\xC0\x49\xFF\xC1\x49\xFF\xC2\x49\xFF\xC3"
      "\x49\xFF\xC4\x49\xFF\xC5\x49\xFF\xC6\x49\xFF\xC7"_view);
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, dec) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  // Perform all the 32 bit operations.
  for (x86_64::Reg reg = x86_64::Reg::EAX; reg <= x86_64::Reg::R15D;
       reg = x86_64::Reg(Bits_8(reg) + 1)) {
    assembler.dec(reg);
  }

  EXPECT_HEX(
      machine_code,
      "\xFF\xC8\xFF\xC9\xFF\xCA\xFF\xCB\xFF\xCC\xFF\xCD\xFF\xCE\xFF\xCF\x41\xFF"
      "\xC8\x41\xFF\xC9\x41\xFF\xCA\x41\xFF\xCB\x41\xFF\xCC\x41\xFF\xCD\x41\xFF"
      "\xCE\x41\xFF\xCF"_view);

  // Perform all 64 bit operations.
  machine_code.clear();
  for (x86_64::Reg reg = x86_64::Reg::RAX; reg <= x86_64::Reg::R15;
       reg = x86_64::Reg(Bits_8(reg) + 1)) {
    assembler.dec(reg);
  }
  EXPECT_HEX(
      machine_code,
      "\x48\xFF\xC8\x48\xFF\xC9\x48\xFF\xCA\x48\xFF\xCB\x48\xFF\xCC\x48\xFF\xCD"
      "\x48\xFF\xCE\x48\xFF\xCF\x49\xFF\xC8\x49\xFF\xC9\x49\xFF\xCA\x49\xFF\xCB"
      "\x49\xFF\xCC\x49\xFF\xCD\x49\xFF\xCE\x49\xFF\xCF"_view);
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, zero) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  // Perform all the 32 bit operations.
  for (x86_64::Reg reg = x86_64::Reg::EAX; reg <= x86_64::Reg::R15D;
       reg = x86_64::Reg(Bits_8(reg) + 1)) {
    assembler.zero(reg);
  }

  EXPECT_HEX(
      machine_code,
      "\x31\xC0\x31\xC9\x31\xD2\x31\xDB\x31\xE4\x31\xED\x31\xF6\x31\xFF\x45\x31"
      "\xC0\x45\x31\xC9\x45\x31\xD2\x45\x31\xDB\x45\x31\xE4\x45\x31\xED\x45\x31"
      "\xF6\x45\x31\xFF"_view);

  // Perform all 64 bit operations.
  machine_code.clear();
  for (x86_64::Reg reg = x86_64::Reg::RAX; reg <= x86_64::Reg::R15;
       reg = x86_64::Reg(Bits_8(reg) + 1)) {
    assembler.zero(reg);
  }
  // Should be the same as the 64 bit operations.
  EXPECT_HEX(
      machine_code,
      "\x31\xC0\x31\xC9\x31\xD2\x31\xDB\x31\xE4\x31\xED\x31\xF6\x31\xFF\x4D\x31"
      "\xC0\x4D\x31\xC9\x4D\x31\xD2\x4D\x31\xDB\x4D\x31\xE4\x4D\x31\xED\x4D\x31"
      "\xF6\x4D\x31\xFF"_view);
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, one) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  // Perform all the 32 bit operations.
  for (x86_64::Reg reg = x86_64::Reg::EAX; reg <= x86_64::Reg::R15D;
       reg = x86_64::Reg(Bits_8(reg) + 1)) {
    assembler.one(reg);
  }

  EXPECT_HEX(
      machine_code,
      "\x31\xC0\xFF\xC0\x31\xC9\xFF\xC1\x31\xD2\xFF\xC2\x31\xDB\xFF\xC3\x31\xE4"
      "\xFF\xC4\x31\xED\xFF\xC5\x31\xF6\xFF\xC6\x31\xFF\xFF\xC7\x45\x31\xC0\x41"
      "\xFF\xC0\x45\x31\xC9\x41\xFF\xC1\x45\x31\xD2\x41\xFF\xC2\x45\x31\xDB\x41"
      "\xFF\xC3\x45\x31\xE4\x41\xFF\xC4\x45\x31\xED\x41\xFF\xC5\x45\x31\xF6\x41"
      "\xFF\xC6\x45\x31\xFF\x41\xFF\xC7"_view);

  // Perform all 64 bit operations.
  machine_code.clear();
  for (x86_64::Reg reg = x86_64::Reg::RAX; reg <= x86_64::Reg::R15;
       reg = x86_64::Reg(Bits_8(reg) + 1)) {
    assembler.one(reg);
  }
  EXPECT_HEX(
      machine_code,
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
       reg = x86_64::Reg(Bits_8(reg) + 1)) {
    assembler.neg_one(reg);
  }

  EXPECT_HEX(
      machine_code,
      "\x31\xC0\xFF\xC8\x31\xC9\xFF\xC9\x31\xD2\xFF\xCA\x31\xDB\xFF\xCB\x31\xE4"
      "\xFF\xCC\x31\xED\xFF\xCD\x31\xF6\xFF\xCE\x31\xFF\xFF\xCF\x45\x31\xC0\x41"
      "\xFF\xC8\x45\x31\xC9\x41\xFF\xC9\x45\x31\xD2\x41\xFF\xCA\x45\x31\xDB\x41"
      "\xFF\xCB\x45\x31\xE4\x41\xFF\xCC\x45\x31\xED\x41\xFF\xCD\x45\x31\xF6\x41"
      "\xFF\xCE\x45\x31\xFF\x41\xFF\xCF"_view);

  // Perform all 64 bit operations.
  machine_code.clear();
  for (x86_64::Reg reg = x86_64::Reg::RAX; reg <= x86_64::Reg::R15;
       reg = x86_64::Reg(Bits_8(reg) + 1)) {
    assembler.neg_one(reg);
  }
  // Neg one can't optimize for the lower 3 bit registers since dec needs to
  // wrap around.
  EXPECT_HEX(
      machine_code,
      "\x31\xC0\x48\xFF\xC8\x31\xC9\x48\xFF\xC9\x31\xD2\x48\xFF\xCA\x31\xDB\x48"
      "\xFF\xCB\x31\xE4\x48\xFF\xCC\x31\xED\x48\xFF\xCD\x31\xF6\x48\xFF\xCE\x31"
      "\xFF\x48\xFF\xCF\x4D\x31\xC0\x49\xFF\xC8\x4D\x31\xC9\x49\xFF\xC9\x4D\x31"
      "\xD2\x49\xFF\xCA\x4D\x31\xDB\x49\xFF\xCB\x4D\x31\xE4\x49\xFF\xCC\x4D\x31"
      "\xED\x49\xFF\xCD\x4D\x31\xF6\x49\xFF\xCE\x4D\x31\xFF\x49\xFF\xCF"_view);
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_reg8) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  for (x86_64::Reg reg = x86_64::Reg::AL; reg <= x86_64::Reg::R15B;
       reg = x86_64::Reg(Bits_8(reg) + 1)) {
    assembler.mov(reg, reg);
  }
  // AL-BL: no REX. SPL-DIL: bare REX 0x40. R8B-R15B: REX.R|REX.B = 0x45.
  EXPECT_HEX(
      machine_code,
      "\x88\xC0\x88\xC9\x88\xD2\x88\xDB"                  // AL CL DL BL
      "\x40\x88\xE4\x40\x88\xED\x40\x88\xF6\x40\x88\xFF"  // SPL BPL SIL DIL
      "\x45\x88\xC0\x45\x88\xC9\x45\x88\xD2\x45\x88\xDB"  // R8B-R11B
      "\x45\x88\xE4\x45\x88\xED\x45\x88\xF6\x45\x88\xFF"_view);  // R12B-R15B

  // Cross-register to catch ModRM bugs with reg & r/m and all of the weird
  // REX combos that can show up.
  machine_code.clear();
  assembler.mov(
      x86_64::Reg::AL, x86_64::Reg::CL);  // no REX, reg=AL(0),  rm=CL(1)
  assembler.mov(
      x86_64::Reg::CL, x86_64::Reg::AL);  // no REX, reg=CL(1),  rm=AL(0)
  assembler.mov(
      x86_64::Reg::AL, x86_64::Reg::SPL);  // bare REX, reg=AL(0), rm=SPL(4)
  assembler.mov(
      x86_64::Reg::SPL, x86_64::Reg::AL);  // bare REX, reg=SPL(4), rm=AL(0)
  assembler.mov(
      x86_64::Reg::R8B, x86_64::Reg::AL);  // REX.R, reg=R8B(0), rm=AL(0)
  assembler.mov(
      x86_64::Reg::AL, x86_64::Reg::R8B);  // REX.B, reg=AL(0),  rm=R8B(0)
  assembler.mov(
      x86_64::Reg::R9B, x86_64::Reg::R8B);  // REX.R|B, reg=R9B(1), rm=R8B(0)
  assembler.mov(
      x86_64::Reg::R8B, x86_64::Reg::R9B);  // REX.R|B, reg=R8B(0), rm=R9B(1)
  EXPECT_HEX(
      machine_code,
      "\x88\xC1"             // AL  → CL   (ModRM reg=0, rm=1)
      "\x88\xC8"             // CL  → AL   (ModRM reg=1, rm=0)
      "\x40\x88\xC4"         // AL  → SPL  (bare REX; ModRM reg=0=AL, rm=4=SPL)
      "\x40\x88\xE0"         // SPL → AL   (bare REX; ModRM reg=4=SPL, rm=0=AL)
      "\x44\x88\xC0"         // R8B → AL   (REX.R extends reg to R8B)
      "\x41\x88\xC0"         // AL  → R8B  (REX.B extends rm  to R8B)
      "\x45\x88\xC8"         // R9B → R8B  (ModRM reg=1=R9B, rm=0=R8B)
      "\x45\x88\xC1"_view);  // R8B → R9B  (ModRM reg=0=R8B, rm=1=R9B)
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_reg16) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  for (x86_64::Reg reg = x86_64::Reg::AX; reg <= x86_64::Reg::R15W;
       reg = x86_64::Reg(Bits_8(reg) + 1)) {
    assembler.mov(reg, reg);
  }
  // AX-DI: 0x66 prefix, no REX. R8W-R15W: 0x66 + REX.R|REX.B = 0x45.
  EXPECT_HEX(
      machine_code,
      "\x66\x89\xC0\x66\x89\xC9\x66\x89\xD2\x66\x89\xDB"  // AX CX DX BX
      "\x66\x89\xE4\x66\x89\xED\x66\x89\xF6\x66\x89\xFF"  // SP BP SI DI
      "\x66\x45\x89\xC0\x66\x45\x89\xC9\x66\x45\x89\xD2\x66\x45\x89\xDB"  // R8W-R11W
      "\x66\x45\x89\xE4\x66\x45\x89\xED\x66\x45\x89\xF6\x66\x45\x89\xFF"_view);  // R12W-R15W

  // Cross-register to catch ModRM bugs with reg & r/m.
  machine_code.clear();
  assembler.mov(x86_64::Reg::AX, x86_64::Reg::CX);    // 66 89 C1
  assembler.mov(x86_64::Reg::CX, x86_64::Reg::AX);    // 66 89 C8
  assembler.mov(x86_64::Reg::R8W, x86_64::Reg::AX);   // 66 REX.R(44) 89 C0
  assembler.mov(x86_64::Reg::AX, x86_64::Reg::R8W);   // 66 REX.B(41) 89 C0
  assembler.mov(x86_64::Reg::R9W, x86_64::Reg::R8W);  // 66 REX.R|B(45) 89 C8
  assembler.mov(x86_64::Reg::R8W, x86_64::Reg::R9W);  // 66 REX.R|B(45) 89 C1
  EXPECT_HEX(
      machine_code,
      "\x66\x89\xC1"             // AX  → CX
      "\x66\x89\xC8"             // CX  → AX
      "\x66\x44\x89\xC0"         // R8W → AX   (REX.R extends reg to R8W)
      "\x66\x41\x89\xC0"         // AX  → R8W  (REX.B extends rm  to R8W)
      "\x66\x45\x89\xC8"         // R9W → R8W  (ModRM reg=1=R9W, rm=0=R8W)
      "\x66\x45\x89\xC1"_view);  // R8W → R9W  (ModRM reg=0=R8W, rm=1=R9W)
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_reg32) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  for (x86_64::Reg reg = x86_64::Reg::EAX; reg <= x86_64::Reg::R15D;
       reg = x86_64::Reg(Bits_8(reg) + 1)) {
    assembler.mov(reg, reg);
  }
  // EAX-EDI: no REX prefix. R8D-R15D: REX.R|REX.B = 0x45.
  EXPECT_HEX(
      machine_code,
      "\x89\xC0\x89\xC9\x89\xD2\x89\xDB\x89\xE4\x89\xED\x89\xF6\x89\xFF"  // EAX-EDI
      "\x45\x89\xC0\x45\x89\xC9\x45\x89\xD2\x45\x89\xDB"         // R8D-R11D
      "\x45\x89\xE4\x45\x89\xED\x45\x89\xF6\x45\x89\xFF"_view);  // R12D-R15D

  // Cross-register to catch ModRM bugs with reg & r/m.
  machine_code.clear();
  assembler.mov(x86_64::Reg::EAX, x86_64::Reg::EBX);  // 89 C3
  assembler.mov(x86_64::Reg::EBX, x86_64::Reg::EAX);  // 89 D8
  assembler.mov(x86_64::Reg::R8D, x86_64::Reg::EAX);  // REX.R(44) 89 C0
  assembler.mov(x86_64::Reg::EAX, x86_64::Reg::R8D);  // REX.B(41) 89 C0
  assembler.mov(x86_64::Reg::R9D, x86_64::Reg::R8D);  // REX.R|B(45) 89 C8
  assembler.mov(x86_64::Reg::R8D, x86_64::Reg::R9D);  // REX.R|B(45) 89 C1
  EXPECT_HEX(
      machine_code,
      "\x89\xC3"             // EAX → EBX
      "\x89\xD8"             // EBX → EAX
      "\x44\x89\xC0"         // R8D → EAX  (REX.R extends reg to R8D)
      "\x41\x89\xC0"         // EAX → R8D  (REX.B extends rm  to R8D)
      "\x45\x89\xC8"         // R9D → R8D  (ModRM reg=1=R9D, rm=0=R8D)
      "\x45\x89\xC1"_view);  // R8D → R9D  (ModRM reg=0=R8D, rm=1=R9D)
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_reg64) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  for (x86_64::Reg reg = x86_64::Reg::RAX; reg <= x86_64::Reg::R15;
       reg = x86_64::Reg(Bits_8(reg) + 1)) {
    assembler.mov(reg, reg);
  }
  // RAX-RDI: REX.W = 0x48. R8-R15: REX.W|REX.R|REX.B = 0x4D.
  EXPECT_HEX(
      machine_code,
      "\x48\x89\xC0\x48\x89\xC9\x48\x89\xD2\x48\x89\xDB"         // RAX-RBX
      "\x48\x89\xE4\x48\x89\xED\x48\x89\xF6\x48\x89\xFF"         // RSP-RDI
      "\x4D\x89\xC0\x4D\x89\xC9\x4D\x89\xD2\x4D\x89\xDB"         // R8-R11
      "\x4D\x89\xE4\x4D\x89\xED\x4D\x89\xF6\x4D\x89\xFF"_view);  // R12-R15

  // Cross-register to catch ModRM bugs with reg & r/m.
  machine_code.clear();
  assembler.mov(x86_64::Reg::RAX, x86_64::Reg::RBX);  // REX.W(48) 89 C3
  assembler.mov(x86_64::Reg::RBX, x86_64::Reg::RAX);  // REX.W(48) 89 D8
  assembler.mov(x86_64::Reg::R8, x86_64::Reg::RAX);   // REX.W|R(4C) 89 C0
  assembler.mov(x86_64::Reg::RAX, x86_64::Reg::R8);   // REX.W|B(49) 89 C0
  assembler.mov(x86_64::Reg::R9, x86_64::Reg::R8);    // REX.W|R|B(4D) 89 C8
  assembler.mov(x86_64::Reg::R8, x86_64::Reg::R9);    // REX.W|R|B(4D) 89 C1
  EXPECT_HEX(
      machine_code,
      "\x48\x89\xC3"         // RAX → RBX
      "\x48\x89\xD8"         // RBX → RAX
      "\x4C\x89\xC0"         // R8  → RAX  (REX.W|R extends reg to R8)
      "\x49\x89\xC0"         // RAX → R8   (REX.W|B extends rm  to R8)
      "\x4D\x89\xC8"         // R9  → R8   (ModRM reg=1=R9, rm=0=R8)
      "\x4D\x89\xC1"_view);  // R8  → R9   (ModRM reg=0=R8, rm=1=R9)
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_r8_imm8) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  for (x86_64::Reg reg = x86_64::Reg::AL; reg <= x86_64::Reg::R15B;
       reg = x86_64::Reg(Bits_8(reg) + 1)) {
    assembler.mov(Bits_8(0x42), reg);
  }
  // AL-BL: B0+rd + imm8 (2 bytes). SPL-DIL: bare REX(0x40) + B4+rd + imm8 (3
  // bytes). R8B-R15B: REX.B(0x41) + B0+rd + imm8 (3 bytes).
  EXPECT_HEX(
      machine_code,
      "\xB0\x42\xB1\x42\xB2\x42\xB3\x42"                  // AL CL DL BL
      "\x40\xB4\x42\x40\xB5\x42\x40\xB6\x42\x40\xB7\x42"  // SPL BPL SIL DIL
      "\x41\xB0\x42\x41\xB1\x42\x41\xB2\x42\x41\xB3\x42"  // R8B-R11B
      "\x41\xB4\x42\x41\xB5\x42\x41\xB6\x42\x41\xB7\x42"_view);  // R12B-R15B
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_r16_imm16) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  for (x86_64::Reg reg = x86_64::Reg::AX; reg <= x86_64::Reg::R15W;
       reg = x86_64::Reg(Bits_8(reg) + 1)) {
    assembler.mov(Bits_16(0x1234), reg);
  }
  // AX-DI: 0x66 + B8+rd + imm16 (4 bytes). R8W-R15W: 0x66 + REX.B(0x41) + B8+rd
  // + imm16 (5 bytes).
  EXPECT_HEX(
      machine_code,
      "\x66\xB8\x34\x12\x66\xB9\x34\x12\x66\xBA\x34\x12\x66\xBB\x34\x12"
      "\x66\xBC\x34\x12\x66\xBD\x34\x12\x66\xBE\x34\x12\x66\xBF\x34\x12"
      "\x66\x41\xB8\x34\x12\x66\x41\xB9\x34\x12\x66\x41\xBA\x34\x12\x66\x41\xBB"
      "\x34\x12"
      "\x66\x41\xBC\x34\x12\x66\x41\xBD\x34\x12\x66\x41\xBE\x34\x12\x66\x41\xBF\x34\x12"_view);

  // Zero: XOR r/m32, r/m32 (zero-extends to full width; smaller than 0x66 +
  // B8+rd + 0x00 0x00)
  machine_code.clear();
  assembler.mov(Bits_16(0), x86_64::Reg::AX);   // zero(AX)  → 31 C0
  assembler.mov(Bits_16(0), x86_64::Reg::R8W);  // zero(R8W) → 45 31 C0
  EXPECT_HEX(
      machine_code,
      "\x31\xC0"             // zero(AX)
      "\x45\x31\xC0"_view);  // zero(R8W)
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_r32_imm32) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  for (x86_64::Reg reg = x86_64::Reg::EAX; reg <= x86_64::Reg::R15D;
       reg = x86_64::Reg(Bits_8(reg) + 1)) {
    assembler.mov(Bits_32(0x12345678), reg);
  }
  // EAX-EDI: B8+rd, imm32 (5 bytes). R8D-R15D: REX.B(0x41) + B8+rd, imm32 (6
  // bytes).
  EXPECT_HEX(
      machine_code,
      "\xB8\x78\x56\x34\x12\xB9\x78\x56\x34\x12\xBA\x78\x56\x34\x12\xBB\x78\x56"
      "\x34\x12"
      "\xBC\x78\x56\x34\x12\xBD\x78\x56\x34\x12\xBE\x78\x56\x34\x12\xBF\x78\x56"
      "\x34\x12"
      "\x41\xB8\x78\x56\x34\x12\x41\xB9\x78\x56\x34\x12\x41\xBA\x78\x56\x34\x12"
      "\x41\xBB\x78\x56\x34\x12"
      "\x41\xBC\x78\x56\x34\x12\x41\xBD\x78\x56\x34\x12\x41\xBE\x78\x56\x34\x12\x41\xBF\x78\x56\x34\x12"_view);

  // Zero: XOR r/m32, r/m32 (3 bytes vs 5 byte B8+rd + 0x00 0x00 0x00 0x00)
  machine_code.clear();
  assembler.mov(Bits_32(0), x86_64::Reg::EAX);  // zero(EAX) → 31 C0
  assembler.mov(Bits_32(0), x86_64::Reg::R8D);  // zero(R8D) → 45 31 C0
  EXPECT_HEX(
      machine_code,
      "\x31\xC0"             // zero(EAX)
      "\x45\x31\xC0"_view);  // zero(R8D)

  // One: XOR + INC (4 bytes vs 5 byte B8+rd + 0x01 0x00 0x00 0x00)
  machine_code.clear();
  assembler.mov(Bits_32(1), x86_64::Reg::EAX);  // one(EAX) → 31 C0 FF C0
  assembler.mov(Bits_32(1), x86_64::Reg::R8D);  // one(R8D) → 45 31 C0 41 FF C0
  EXPECT_HEX(
      machine_code,
      "\x31\xC0\xFF\xC0"                 // one(EAX): zero + INC EAX
      "\x45\x31\xC0\x41\xFF\xC0"_view);  // one(R8D): zero + REX.B INC R8D
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
      machine_code,
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

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_store) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  // Store RAX to [RBX] — no displacement
  assembler.mov(x86_64::Reg::RAX, x86_64::Reg::RBX, Signed_32(0));
  // Store RAX to [RBX+8] — 8-bit displacement
  assembler.mov(x86_64::Reg::RAX, x86_64::Reg::RBX, Signed_32(8));
  // Store RAX to [RBP+0] — RBP forces 8-bit disp even when disp=0
  assembler.mov(x86_64::Reg::RAX, x86_64::Reg::RBP, Signed_32(0));
  // Store RAX to [RSP+8] — RSP requires SIB byte
  assembler.mov(x86_64::Reg::RAX, x86_64::Reg::RSP, Signed_32(8));
  // Store R8 to [RBX+8] — extended source register
  assembler.mov(x86_64::Reg::R8, x86_64::Reg::RBX, Signed_32(8));
  // Store smaller operands to [RBP+0] without letting the base force REX.W.
  assembler.mov(x86_64::Reg::EAX, x86_64::Reg::RBP, Signed_32(0));
  assembler.mov(x86_64::Reg::AX, x86_64::Reg::RBP, Signed_32(0));
  assembler.mov(x86_64::Reg::AL, x86_64::Reg::RBP, Signed_32(0));
  EXPECT_HEX(
      machine_code,
      "\x48\x89\x03"             // RAX → [RBX]
      "\x48\x89\x43\x08"         // RAX → [RBX+8]
      "\x48\x89\x45\x00"         // RAX → [RBP+0]
      "\x48\x89\x44\x24\x08"     // RAX → [RSP+8] (SIB)
      "\x4C\x89\x43\x08"         // R8 → [RBX+8] (REX.W+REX.R)
      "\x89\x45\x00"             // EAX → [RBP+0]
      "\x66\x89\x45\x00"         // AX → [RBP+0]
      "\x40\x88\x45\x00"_view);  // AL → [RBP+0] (bare REX)
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, mov_load) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);
  // Load [RBX] to RAX — no displacement
  assembler.mov(x86_64::Reg::RBX, Signed_32(0), x86_64::Reg::RAX);
  // Load [RBX+8] to RAX — 8-bit displacement
  assembler.mov(x86_64::Reg::RBX, Signed_32(8), x86_64::Reg::RAX);
  // Load [RSP+8] to RAX — RSP base requires SIB byte
  assembler.mov(x86_64::Reg::RSP, Signed_32(8), x86_64::Reg::RAX);
  // Load [R8+8] to RAX — extended base register
  assembler.mov(x86_64::Reg::R8, Signed_32(8), x86_64::Reg::RAX);
  // Load smaller operands from [RBP+0] without letting the base force REX.W.
  assembler.mov(x86_64::Reg::RBP, Signed_32(0), x86_64::Reg::EAX);
  assembler.mov(x86_64::Reg::RBP, Signed_32(0), x86_64::Reg::AX);
  assembler.mov(x86_64::Reg::RBP, Signed_32(0), x86_64::Reg::AL);
  EXPECT_HEX(
      machine_code,
      "\x48\x8B\x03"             // [RBX] → RAX
      "\x48\x8B\x43\x08"         // [RBX+8] → RAX
      "\x48\x8B\x44\x24\x08"     // [RSP+8] → RAX (SIB)
      "\x49\x8B\x40\x08"         // [R8+8] → RAX (REX.W+REX.B)
      "\x8B\x45\x00"             // [RBP+0] → EAX
      "\x66\x8B\x45\x00"         // [RBP+0] → AX
      "\x40\x8A\x45\x00"_view);  // [RBP+0] → AL (bare REX)
}

PERIMORTEM_UNIT_TEST(Ttxx86_64, call) {
  Perimortem::Memory::Dynamic::Bytes machine_code;
  x86_64 assembler(machine_code);

  // Perform a basic call to generate a place holder instruction.
  assembler.call();

  // Should create a single byte call with 4 bytes left disp32.
  EXPECT_HEX(machine_code, "\xE8\0\0\0\0"_view);
}
