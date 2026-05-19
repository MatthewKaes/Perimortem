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
