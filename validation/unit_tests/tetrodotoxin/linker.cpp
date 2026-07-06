// Perimortem Engine
// Copyright (c) Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/compiler/assembler/x86_64.hpp"
#include "tetrodotoxin/linker/linker.hpp"
#include "tetrodotoxin/linker/object/symbol.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Compiler;
using namespace Tetrodotoxin::Linker;
using namespace Validation;


static Harness TetrodotoxinTerminal = {
  .name = "Tetrodotoxin::Terminal"_view,
};

PERIMORTEM_UNIT_TEST(TetrodotoxinTerminal, section_ids) {
  Dynamic::Bytes code;
  Assembler::x86_64 assembler(code);
  assembler.ret();

  Tetrodotoxin::Linker::Linker linker;
  EXPECT_EQ(
      linker.add_section(Object::Section::Type::Program, code), Bits_16(1));
  EXPECT_EQ(
      linker.add_section(Object::Section::Type::Program, code), Bits_16(2));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinTerminal, archive_symbol) {
  Dynamic::Bytes code;
  Assembler::x86_64 assembler(code);
  assembler.ret();

  EXPECT(code.get_size() > 0);

  Tetrodotoxin::Linker::Linker linker;
  Bits_16 program_section = linker.add_section(
      Object::Section::Type::Program, code);

  auto symbol = Object::Symbol::create_function(
      "module_entry"_view,
      program_section,
      Object::Symbol::Visibility::Global);
  symbol.set_range({0, code.get_size()});
  linker.add_symbol(symbol);

  auto archive = linker.build_library("module.o"_view);
  ASSERT(archive.get_size() > 8);
  EXPECT_TEXT(archive.get_view().slice(0, 8), "!<arch>\n"_view);
  EXPECT(
      Algorithm::search(archive.get_view(), "module_entry"_view) != Count(-1));
}
