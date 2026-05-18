// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/linker/target/elf.hpp"

#include "perimortem/utility/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Linker::Target;

static constexpr View::Bytes text_section_name        = ".text"_view;
static constexpr View::Bytes rodata_str_section_name  = ".rodata.str"_view;
static constexpr View::Bytes rodata_section_name      = ".rodata"_view;

auto Elf::section_properties(SectionTypes type) -> SectionProperties {
  switch (type) {
  case SectionTypes::Program:
    return {
      text_section_name,
      SectionType::ProgramData,
      SectionFlags::Allocated | SectionFlags::Executable,
      16,
    };
  case SectionTypes::Strings:
    return {
      rodata_str_section_name,
      SectionType::ProgramData,
      SectionFlags::Allocated | SectionFlags::Mergeable | SectionFlags::Strings,
      1,
    };
  case SectionTypes::ROData:
    return {
      rodata_section_name,
      SectionType::ProgramData,
      SectionFlags::Allocated,
      8,
    };
  }
  return {};
}
