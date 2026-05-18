// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/linker/target/elf.hpp"

#include "perimortem/core/static/bytes.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/utility/null_terminated.hpp"

#include "tetrodotoxin/compiler/context/symbol.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Linker::Target;
using namespace Tetrodotoxin::Compiler;

static constexpr View::Bytes text_section = ".text"_view;
static constexpr View::Bytes rodata_str_section = ".rodata.str"_view;
static constexpr View::Bytes rodata_section = ".rodata"_view;

constexpr auto elf_endian = Data::ByteOrder::Little;
constexpr auto sym_endian = Data::ByteOrder::Big;

// Sentinel values for InputSymbol::section_index — values >= 0xFF00 are
// reserved. UndefinedSection maps to ELF SHN_UNDEF (0) during output; the
// others pass through directly since they match the ELF SHN_ABS / SHN_COMMON
// values.
static constexpr Bits_16 UndefinedSection = 0xFFFF;
static constexpr Bits_16 AbsoluteSection = 0xFFF1;
static constexpr Bits_16 CommonSection = 0xFFF2;

// ELF object file type (e_type).
enum class ObjectType : Bits_16 {
  Relocatable = 1,
};

// ELF target machine (e_machine).
enum class Machine : Bits_16 {
  X86_64 = 62,
};

// x86-64 relocation types (r_info type field).
enum class RelocationType : Bits_32 {
  None = 0,
  Absolute64 = 1,    // S + A
  PcRelative32 = 2,  // S + A - P
  Got32 = 3,
  Plt32 = 4,          // S + A - P (via PLT)
  Absolute32 = 10,    // S + A (zero-extend to 64)
  Absolute32S = 11,   // S + A (sign-extend to 64)
  PcRelative64 = 24,  // S + A - P
};

// ELF section header type (sh_type).
enum class SectionType : Bits_32 {
  Null = 0,
  ProgramData = 1,
  SymbolTable = 2,
  StringTable = 3,
  RelocationAddend = 4,
  NoBits = 8,
};

struct SectionProperties {
  Perimortem::Core::View::Bytes name;
  SectionType type;
  Bits_64 flags;
  Bits_64 alignment;
  Perimortem::Core::View::Bytes data = ""_view;
  Count name_index = 0;
  Count offset = 0;
};

// ELF64 section header — 64 bytes.
struct SectionHeader {
  Bits_32 name_offset;
  Bits_32 type;
  Bits_64 flags;
  Bits_64 virtual_address;
  Bits_64 file_offset;
  Bits_64 size;
  Bits_32 link;
  Bits_32 info;
  Bits_64 alignment;
  Bits_64 entry_size;
};
static_assert(sizeof(SectionHeader) == 64);

struct SymbolRef {
  const Context::Symbol* symbol;
  Count string_index;
};

enum class SectionFlags : Bits_64 {
  Writable = 0x1,
  Allocated = 0x2,
  Executable = 0x4,
  Mergeable = 0x10,
  Strings = 0x20,
};

auto user_sections(Format::Section user_section) -> SectionProperties {
  switch (user_section.type) {
  case Context::Symbol::Location::Program:
    return {
      ".text\0"_view,
      SectionType::ProgramData,
      Bits_64(SectionFlags::Allocated) | Bits_64(SectionFlags::Executable),
      16,
      user_section.data,
    };
  case Context::Symbol::Location::Strings:
    return {
      ".rodata.str\0"_view,
      SectionType::ProgramData,
      Bits_64(SectionFlags::Allocated) | Bits_64(SectionFlags::Mergeable) |
          Bits_64(SectionFlags::Strings),
      1,
      user_section.data,
    };
  case Context::Symbol::Location::ReadOnly:
    return {
      ".rodata\0"_view,
      SectionType::ProgramData,
      Bits_64(SectionFlags::Allocated),
      8,
      user_section.data,
    };

    // Unsupported section mapping.
  default:
    return {};
  }
}

auto Elf::write_header(
    Access::Bytes buffer,
    Bits_64 section_offset,
    Bits_16 section_count,
    Bits_16 shstrtab_index) -> void {
  // ELF64 file header, should be exactly 64 bytes.
  struct Header {
    Perimortem::Core::Static::Bytes<16> identity;
    ObjectType type;
    Machine machine;
    Bits_32 version;
    Bits_64 entry_point;
    Bits_64 program_header_offset;
    Bits_64 section_header_offset;
    Bits_32 flags;
    Bits_16 header_size;
    Bits_16 program_header_entry_size;
    Bits_16 program_header_count;
    Bits_16 section_entry_size;
    Bits_16 section_count;
    Bits_16 string_section_index;
  };
  static_assert(sizeof(Header) == 64);

  static constexpr Byte identity[16] = {
    // Magic elf header
    0x7F, 'E', 'L', 'F',
    2,  // 64 bit target (2)
    1,  // Set ELF data to use little endien.
    1,  // Always 1 for ELF version.
    0,  // OS ABI set to zero, possibly set to 3 (linux) in the future
    0,  // ABI version is unused
    // 7 Padding bytes of zero
    0, 0, 0, 0, 0, 0, 0};

  Header* header = reinterpret_cast<Header*>(buffer.get_data());
  header->identity = identity;
  Data::write<elf_endian>(&header->type, ObjectType::Relocatable);
  Data::write<elf_endian>(&header->machine, Machine::X86_64);
  Data::write<elf_endian>(&header->version, Bits_32(1));
  Data::write<elf_endian>(&header->section_header_offset, section_offset);
  Data::write<elf_endian>(&header->header_size, Bits_16(sizeof(Header)));
  Data::write<elf_endian>(
      &header->section_entry_size, Bits_16(sizeof(SectionHeader)));
  Data::write<elf_endian>(&header->section_count, section_count);
  Data::write<elf_endian>(&header->string_section_index, shstrtab_index);
}

auto Elf::create_elf_sections(View::Vector<Format::Section> sections) -> void {
  // Create all user sections.
  for (Count i = 0; i < sections.get_size(); i++) {
    elf_sections.insert(user_sections(sections[i]));
  }

  // Create ELF support sections
  str_section_index = elf_sections.get_size();
  elf_sections.insert({
    ".strtab\0"_view,
    SectionType::StringTable,
    Bits_64(SectionFlags::Allocated) | Bits_64(SectionFlags::Mergeable) |
        Bits_64(SectionFlags::Strings),
    8,
  });

  // TODO: Add .symtab .strtab .shstrtab
  // t.symtab_name_offset = append_to_strtab(t.shstrtab, ".symtab\0"_view);
  // t.strtab_name_offset = append_to_strtab(t.shstrtab, ".strtab\0"_view);
  // t.shstrtab_name_offset = append_to_strtab(t.shstrtab, ".shstrtab\0"_view);
}

auto build_name_table(
    Allocator::Arena& arena,
    SectionProperties& sym_table,
    Access::Vector<SymbolRef> symbols) -> void {
  Count size = 1;  // Reserve a single null
  for (Count i = 0; i < symbols.get_size(); i++) {
    // The string index is equal to the current size.
    // Then we reserve enough space for the string + a null character.
    symbols[i].string_index = size;
    size += symbols[i].symbol->get_name().get_size() + 1;
  }

  // Allocate the actual table and copy over the data.
  Byte* name_table = arena.allocate(size);
  Count write_ptr = 0;
  name_table[write_ptr++] = '\0';
  for (Count i = 0; i < symbols.get_size(); i++) {
    Data::copy(
        name_table + write_ptr, symbols[i].symbol->get_name().get_data(),
        symbols[i].symbol->get_name().get_size());
    name_table[write_ptr++] = '\0';
  }

  sym_table.data = View::Bytes(name_table, size);
}

auto create_elf_symbols(View::Vector<Context::Symbol> symbols)
    -> Dynamic::Vector<SymbolRef> {
  // Sort symbols, putting local symbols first as required by ELF.
  using Visability = Context::Symbol::Visability;
  Dynamic::Vector<SymbolRef> elf_symbols;
  for (Count vis = 0; vis <= Count(Visability::Global); vis++) {
    for (Count i = 0; i < symbols.get_size(); i++) {
      if (symbols[i].get_visability() != Visability(vis)) {
        continue;
      }

      elf_symbols.insert({symbols.get_data() + i, 0});
    }
  }

  return elf_symbols;
}

static auto build_header_string_table(
  // t.strtab.append('\0');
  // for (Count i = 0; i < scratch.symbol_count; i++) {
  // scratch.symbol_name_offsets[i] =
  //     append_to_strtab(t.strtab, symbols.at(i).name);
  // }

  // t.shstrtab.append('\0');
  // for (Count i = 0; i < scratch.section_count; i++) {
  // const auto props = Elf::section_properties(sections.at(i).type);
  // scratch.section_name_offsets[i] = append_to_strtab(t.shstrtab, props.name);
  // }
  // for (Count i = 0; i < scratch.section_count; i++) {
  // if (!scratch.has_relocations[i]) {
  //   continue;
  // }
  // const auto props = Elf::section_properties(sections.at(i).type);
  // scratch.relocation_name_offsets[i] = Bits_32(t.shstrtab.get_size());
  // t.shstrtab.concat(".rela"_view);
  // t.shstrtab.concat(props.name);
  // t.shstrtab.append('\0');
  // }

  // t.symtab_name_offset = append_to_strtab(t.shstrtab, ".symtab\0"_view);
  // t.strtab_name_offset = append_to_strtab(t.shstrtab, ".strtab\0"_view);
  // t.shstrtab_name_offset = append_to_strtab(t.shstrtab, ".shstrtab\0"_view);

  return t;
}

auto Elf::build_object(
    Allocator::Arena& arena,
    View::Vector<Format::Section> sections,
    View::Vector<Context::Symbol> symbols,
    View::Vector<Context::Relocation> relocations) -> Dynamic::Bytes {
  auto elf_sections = create_elf_sections(arena, sections);
  auto elf_symbols = create_elf_symbols(symbols);

  auto strings = build_string_tables(sections, symbols, scratch);
  auto symtab = build_symbol_table(symbols, scratch);
  auto layout = compute_layout(sections, scratch, strings);

  Dynamic::Bytes object(layout.total_file_size);
  Byte* out = object.get_access();
  memset(out, 0, layout.total_file_size);

  write_elf_header(out, layout);
  write_section_data(out, sections);
  write_rela_sections(out, relocations);
}
