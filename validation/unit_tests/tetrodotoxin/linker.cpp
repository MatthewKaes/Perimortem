// Perimortem Engine
// Copyright (c) Matt Kaes

#include "tetrodotoxin/linker/linker.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Linker;
using namespace Validation;

static Harness TetrodotoxinLinker = {
  .name = "Tetrodotoxin::Linker"_view,
};

static auto expect_inventory(
    Test::TestResult& result,
    const Object::Module& module,
    Count section_count,
    Count symbol_count,
    Count relocation_count) -> void {
  EXPECT_EQ(module.get_sections().get_size(), section_count);
  EXPECT_EQ(module.get_symbols().get_size(), symbol_count);
  EXPECT_EQ(module.get_relocations().get_size(), relocation_count);
}

static auto add_temporary_section(Object::Module& module) {
  Dynamic::Bytes data("temporary section"_view);
  return module.add_section(Object::Section::Type::ReadOnly, data);
}

static auto add_temporary_symbol(
    Object::Module& module,
    Unsigned_16 section_index) {
  Dynamic::Bytes name("temporary_symbol"_view);
  auto symbol = Object::Symbol::create_function(
      name, section_index, Object::Symbol::Visibility::Global);
  return module.add_symbol(symbol);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLinker, section_indices) {
  Object::Module module;
  auto rejected_undefined = module.add_section(Object::Section::undefined());
  EXPECT_NOT(rejected_undefined);
  expect_inventory(result, module, 1, 0, 0);

  auto first = module.add_section(Object::Section::Type::Program, "\xC3"_view);
  auto second =
      module.add_section(Object::Section::Type::ReadOnly, "data"_view);

  ASSERT(first);
  ASSERT(second);
  EXPECT_EQ(*first, Unsigned_16(1));
  EXPECT_EQ(*second, Unsigned_16(2));
  EXPECT(
      module.get_sections()[0].get_type() == Object::Section::Type::Undefined);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLinker, section_storage) {
  Object::Module module;
  Dynamic::Bytes caller_data("caller section"_view);
  auto retained =
      module.add_section(Object::Section::Type::Program, caller_data);

  ASSERT(retained);
  caller_data.set('x');
  EXPECT_TEXT(
      module.get_sections()[*retained].get_data(), "caller section"_view);

  auto temporary = add_temporary_section(module);
  ASSERT(temporary);
  EXPECT_TEXT(
      module.get_sections()[*temporary].get_data(), "temporary section"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLinker, symbol_storage) {
  Object::Module module;
  auto section =
      module.add_section(Object::Section::Type::Program, "\xC3"_view);
  ASSERT(section);

  Dynamic::Bytes caller_name("caller_symbol"_view);
  auto symbol = Object::Symbol::create_function(
      caller_name, *section, Object::Symbol::Visibility::Global);
  auto retained = module.add_symbol(symbol);

  ASSERT(retained);
  caller_name.set('x');
  EXPECT_TEXT(module.get_symbols()[*retained].get_name(), "caller_symbol"_view);

  auto temporary = add_temporary_symbol(module, *section);
  ASSERT(temporary);
  EXPECT_TEXT(
      module.get_symbols()[*temporary].get_name(), "temporary_symbol"_view);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLinker, symbol_definitions) {
  Object::Module module;
  auto section =
      module.add_section(Object::Section::Type::Program, "\xC3"_view);
  ASSERT(section);

  auto defined = Object::Symbol::create_function(
      "module_entry"_view, *section, Object::Symbol::Visibility::Global);
  auto undefined = Object::Symbol::create_undefined(
      "write_line"_view, Object::Symbol::Type::Function);
  auto defined_index = module.add_symbol(defined);
  auto undefined_index = module.add_symbol(undefined);

  ASSERT(defined_index);
  ASSERT(undefined_index);
  EXPECT(module.get_symbols()[*defined_index].is_defined());
  EXPECT_EQ(module.get_symbols()[*defined_index].get_section_index(), *section);
  EXPECT(module.get_symbols()[*undefined_index].is_undefined());
  EXPECT_EQ(
      module.get_symbols()[*undefined_index].get_section_index(),
      Unsigned_16(0));
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLinker, invalid_defined_sections) {
  Object::Module module;
  auto section =
      module.add_section(Object::Section::Type::Program, "\xC3"_view);
  ASSERT(section);

  auto zero = Object::Symbol::create_function(
      "zero"_view, 0, Object::Symbol::Visibility::Global);
  auto rejected_zero = module.add_symbol(zero);
  EXPECT_NOT(rejected_zero);
  expect_inventory(result, module, 2, 0, 0);

  auto missing = Object::Symbol::create_function(
      "missing"_view, Unsigned_16(2), Object::Symbol::Visibility::Global);
  auto rejected_missing = module.add_symbol(missing);
  EXPECT_NOT(rejected_missing);
  expect_inventory(result, module, 2, 0, 0);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLinker, invalid_relocation_sections) {
  Object::Module module;
  auto section =
      module.add_section(Object::Section::Type::Program, "\xC3"_view);
  ASSERT(section);

  auto symbol = Object::Symbol::create_undefined(
      "write_line"_view, Object::Symbol::Type::Function);
  auto symbol_index = module.add_symbol(symbol);
  ASSERT(symbol_index);

  auto zero = Object::Relocation::create_plt32(0, *symbol_index, 4);
  Bool rejected_zero = module.add_relocation(zero);
  EXPECT_NOT(rejected_zero);
  expect_inventory(result, module, 2, 1, 0);

  auto missing =
      Object::Relocation::create_plt32(Unsigned_16(2), *symbol_index, 4);
  Bool rejected_missing = module.add_relocation(missing);
  EXPECT_NOT(rejected_missing);
  expect_inventory(result, module, 2, 1, 0);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLinker, invalid_relocation_symbols) {
  Object::Module module;
  auto section =
      module.add_section(Object::Section::Type::Program, "\xC3"_view);
  ASSERT(section);

  auto symbol = Object::Symbol::create_undefined(
      "write_line"_view, Object::Symbol::Type::Function);
  auto symbol_index = module.add_symbol(symbol);
  ASSERT(symbol_index);

  auto missing = Object::Relocation::create_plt32(*section, 1, 4);
  Bool rejected = module.add_relocation(missing);
  EXPECT_NOT(rejected);
  expect_inventory(result, module, 2, 1, 0);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLinker, construction_order) {
  Object::Module module;
  auto section =
      module.add_section(Object::Section::Type::Program, "\xC3"_view);
  ASSERT(section);

  auto symbol = Object::Symbol::create_undefined(
      "write_line"_view, Object::Symbol::Type::Function);
  auto symbol_index = module.add_symbol(symbol);
  ASSERT(symbol_index);

  auto late_section =
      module.add_section(Object::Section::Type::ReadOnly, "late"_view);
  EXPECT_NOT(late_section);
  expect_inventory(result, module, 2, 1, 0);

  auto relocation =
      Object::Relocation::create_plt32(*section, *symbol_index, 4);
  Bool relocated = module.add_relocation(relocation);
  EXPECT(relocated);

  auto late_symbol = Object::Symbol::create_undefined(
      "late"_view, Object::Symbol::Type::Function);
  auto rejected_symbol = module.add_symbol(late_symbol);
  EXPECT_NOT(rejected_symbol);
  expect_inventory(result, module, 2, 1, 1);
}

PERIMORTEM_UNIT_TEST(TetrodotoxinLinker, archive_symbol) {
  Object::Module module;
  auto program_section =
      module.add_section(Object::Section::Type::Program, "\xC3"_view);
  ASSERT(program_section);

  auto symbol = Object::Symbol::create_function(
      "module_entry"_view, *program_section,
      Object::Symbol::Visibility::Global);
  symbol.set_range({0, 1});
  auto symbol_index = module.add_symbol(symbol);
  ASSERT(symbol_index);

  Linker linker;
  auto archive = linker.build_library(module, "module.o"_view);
  ASSERT(archive.get_size() > 8);
  EXPECT_TEXT(archive.get_view().slice(0, 8), "!<arch>\n"_view);
  EXPECT(
      Algorithm::search(archive.get_view(), "module_entry"_view) != Count(-1));
}
