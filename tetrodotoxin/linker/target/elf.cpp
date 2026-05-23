// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/linker/target/elf.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "tetrodotoxin/compiler/context/relocation.hpp"
#include "tetrodotoxin/compiler/context/symbol.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Linker::Target;
using namespace Tetrodotoxin::Compiler;

// Elf is a little endian format, however the ar format has
constexpr auto elf_endian = Data::ByteOrder::Little;
constexpr auto ar_endian = Data::ByteOrder::Big;

enum class ObjectType : Bits_16 { Relocatable = 1 };
enum class Machine : Bits_16 { X86_64 = 62 };

enum class SectionType : Bits_32 {
  Null = 0,
  ProgramData = 1,
  SymbolTable = 2,
  StringTable = 3,
  RelocationAddend = 4,
};

enum class SectionFlags : Bits_64 {
  Writable = 0x01,
  Allocated = 0x02,
  Executable = 0x04,
  Mergeable = 0x10,
  Strings = 0x20,
};

// The ELF header block which contains core info of the binary.
// Should always be exactly 64 bits so a lot of the offsets need
// to have specific bit widths in order to be wire compatable.
struct Header {
  Static::Bytes<16> identity;
  Bits_16 type;
  Bits_16 machine;
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

// Contains the binary layout for the section headers.
// The ELF file can contain any number of sections.
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

// The binary layout for Symbol entries.
// The Compiler format is mostly based on ELF so the two instances are close but
// we use this instance to streamline serialization.
struct SymbolEntry {
  Bits_32 name_offset;
  Byte info;        // (binding << 4) | type
  Byte visibility;  // STV_DEFAULT = 0
  Bits_16 section_index;
  Bits_64 value;
  Bits_64 size;
};
static_assert(sizeof(SymbolEntry) == 24);

// Rela represents relative relocations
struct RelaEntry {
  Bits_64 offset;
  Bits_64 info;  // (symbol_index << 32) | reloc_type
  SignedBits_64 addend;
};
static_assert(sizeof(RelaEntry) == 24);

enum class RelocationType : Bits_32 {
  PcRelative32 = 2,
  Plt32 = 4,
};

// Carries every field needed to write a section header. name_offset and
// file_offset are filled in during the build pipeline.
struct SectionDesc {
  View::Bytes name;
  SectionType type = SectionType::Null;
  Bits_64 flags = 0;
  Bits_64 alignment = 0;
  View::Bytes data;
  Bits_32 link = 0;
  Bits_32 info = 0;
  Bits_64 entry_size = 0;
  Count name_offset = 0;
  Count file_offset = 0;
};

struct SymbolRef {
  const Context::Symbol* symbol;
  Count original_index;
  Count string_table_offset;
};

// The AR header is fairly janky and uses left aligned, space padded, human
// readable values for describing the archive.
struct ArHeader {
  // Filled in with the actual name.
  Static::Bytes<16> name = "                "_bytes;
  // Unused for now.
  Static::Bytes<12> timestamp = "0           "_bytes;
  Static::Bytes<6> owner_id = "0     "_bytes;
  Static::Bytes<6> group_id = "0     "_bytes;
  Static::Bytes<8> file_mode = "644     "_bytes;
  // Stores the number of bytes of data in a textual format.
  Static::Bytes<10> data_size = "          "_bytes;
  // must be "`\n"
  Static::Bytes<2> terminator = "`\n"_bytes;
};
static_assert(sizeof(ArHeader) == 60);

auto fill_ar_header(ArHeader& h, View::Bytes name, Bits_64 size) -> void {
  const Count name_len = name.get_size() < 15 ? name.get_size() : 15;
  Data::copy(h.name.get_data(), name.get_data(), name_len);
  h.name[name_len] = '/';

  // Write only the data size, the rest of the fields are default initalized.
  Writer::Textual(h.data_size.get_access()) << size;
}

auto to_section_desc(Format::Section sec) -> SectionDesc {
  switch (sec.type) {
  case Context::Symbol::Location::Program:
    return {
      ".text"_view,
      SectionType::ProgramData,
      Bits_64(SectionFlags::Allocated) | Bits_64(SectionFlags::Executable),
      16,
      sec.data,
    };
  case Context::Symbol::Location::Strings:
    return {
      ".rodata.str"_view,
      SectionType::ProgramData,
      Bits_64(SectionFlags::Allocated) | Bits_64(SectionFlags::Mergeable) |
          Bits_64(SectionFlags::Strings),
      1,
      sec.data,
    };
  case Context::Symbol::Location::ReadOnly:
    return {
      ".rodata"_view,
      SectionType::ProgramData,
      Bits_64(SectionFlags::Allocated),
      8,
      sec.data,
    };
  default:
    return {};
  }
}

auto sort_symbols(View::Vector<Context::Symbol> symbols)
    -> Dynamic::Vector<SymbolRef> {
  Dynamic::Vector<SymbolRef> sorted;
  using Vis = Context::Symbol::Visability;
  for (Count pass = 0; pass <= Count(Vis::Global); pass++) {
    for (Count i = 0; i < symbols.get_size(); i++) {
      if (Count(symbols[i].get_visability()) != pass) {
        continue;
      }
      sorted.insert({symbols.get_data() + i, i, 0});
    }
  }
  return sorted;
}

auto find_sorted_position(View::Vector<SymbolRef> sorted, Count original_index)
    -> Count {
  for (Count i = 0; i < sorted.get_size(); i++) {
    if (sorted[i].original_index == original_index) {
      return i;
    }
  }
  return 0;
}

auto build_string_table(Access::Vector<SymbolRef> symbols) -> Dynamic::Bytes {
  Dynamic::Bytes strtab;
  strtab.append('\0');
  for (Count i = 0; i < symbols.get_size(); i++) {
    symbols[i].string_table_offset = strtab.get_size();
    strtab.concat(symbols[i].symbol->get_name());
    strtab.append('\0');
  }
  return strtab;
}

// Returns the 1-based ELF section index for a symbol's location, or 0
// (SHN_UNDEF) if not found.
auto section_index_for(
    View::Vector<Format::Section> sections,
    Context::Symbol::Location loc) -> Bits_16 {
  for (Count i = 0; i < sections.get_size(); i++) {
    if (sections[i].type == loc) {
      return Bits_16(1 + i);
    }
  }
  return 0;
}

auto build_symbol_table(
    View::Vector<SymbolRef> sorted,
    View::Vector<Format::Section> sections,
    Count& out_first_global) -> Dynamic::Bytes {
  out_first_global = 1;
  for (Count i = 0; i < sorted.get_size(); i++) {
    if (sorted[i].symbol->get_visability() ==
        Context::Symbol::Visability::Global) {
      break;
    }
    out_first_global++;
  }

  const Count entry_count = 1 + sorted.get_size();
  Dynamic::Bytes data;
  data.forgetful_resize(sizeof(SymbolEntry) * entry_count);
  memset(data.get_access().get_data(), 0, sizeof(SymbolEntry) * entry_count);
  auto* entries = Data::cast<SymbolEntry>(data.get_access().get_data());

  for (Count i = 0; i < sorted.get_size(); i++) {
    const auto& ref = sorted[i];
    const auto& symbol = *ref.symbol;
    auto& entry = entries[1 + i];
    const Byte binding =
        symbol.get_visability() == Context::Symbol::Visability::Global ? 1 : 0;
    Data::write<elf_endian>(
        &entry.name_offset, Bits_32(ref.string_table_offset));
    entry.info = Byte((binding << 4) | Byte(symbol.get_type()));
    const Bits_16 section_index =
        symbol.get_context() == Context::Symbol::Location::External
            ? Bits_16(0)
            : section_index_for(sections, symbol.get_context());
    Data::write<elf_endian>(&entry.section_index, section_index);
    Data::write<elf_endian>(&entry.value, Bits_64(symbol.get_range().start));
    Data::write<elf_endian>(&entry.size, Bits_64(symbol.get_range().size));
  }
  return data;
}

auto build_relocations(
    View::Vector<Context::Relocation> relocations,
    View::Vector<SymbolRef> sorted) -> Dynamic::Bytes {
  Dynamic::Bytes data;
  if (relocations.get_size() == 0) {
    return data;
  }
  data.forgetful_resize(sizeof(RelaEntry) * relocations.get_size());
  auto* entries = Data::cast<RelaEntry>(data.get_access().get_data());

  for (Count i = 0; i < relocations.get_size(); i++) {
    const auto& reloc = relocations[i];
    const Bits_64 symbol_slot =
        Bits_64(1 + find_sorted_position(sorted, reloc.symbol));
    const Bits_32 rtype = reloc.type == Context::Relocation::Type::Plt32
                              ? Bits_32(RelocationType::Plt32)
                              : Bits_32(RelocationType::PcRelative32);
    Data::write<elf_endian>(&entries[i].offset, Bits_64(reloc.offset));
    Data::write<elf_endian>(
        &entries[i].info, (symbol_slot << 32) | Bits_64(rtype));
    Data::write<elf_endian>(&entries[i].addend, SignedBits_64(reloc.addend));
  }
  return data;
}

auto build_section_string_table(Access::Vector<SectionDesc> sections)
    -> Dynamic::Bytes {
  Dynamic::Bytes shstrtab;
  shstrtab.append('\0');
  for (Count i = 0; i < sections.get_size(); i++) {
    if (sections[i].name.get_size() == 0) {
      sections[i].name_offset = 0;
      continue;
    }
    sections[i].name_offset = shstrtab.get_size();
    shstrtab.concat(sections[i].name);
    shstrtab.append('\0');
  }
  return shstrtab;
}

auto assign_offsets(Access::Vector<SectionDesc> sections) -> Count {
  Count offset = sizeof(Header);
  for (Count i = 0; i < sections.get_size(); i++) {
    if (sections[i].data.get_size() == 0) {
      sections[i].file_offset = 0;
      continue;
    }
    const Count align =
        sections[i].alignment > 0 ? Count(sections[i].alignment) : 1;
    offset = (offset + align - 1) & ~(align - 1);
    sections[i].file_offset = offset;
    offset += sections[i].data.get_size();
  }

  // Align the final offset to an 8 byte boundary.
  return Data::align<8>(offset);
}

auto Elf::add_section(Format::Section section) -> void {
  sections.insert(section);
}

auto Elf::add_symbol(Context::Symbol symbol) -> void {
  symbols.insert(symbol);
}

auto Elf::add_relocation(Context::Relocation relocation) -> void {
  relocations.insert(relocation);
}

auto Elf::reset() -> void {
  sections.clear();
  symbols.clear();
  relocations.clear();
}

auto Elf::write_header(
    Access::Bytes buffer,
    Bits_64 section_offset,
    Bits_16 section_count,
    Bits_16 section_string_table_index) -> void {
  static constexpr Byte identity[16] = {
    0x7F, 'E', 'L', 'F',
    2,  // ELFCLASS64
    1,  // ELFDATA2LSB
    1,  // EV_CURRENT
    0,  // ELFOSABI_NONE
    0,    0,   0,   0,   0, 0, 0, 0,
  };
  auto* header = Data::cast<Header>(buffer.get_data());
  header->identity = identity;
  Data::write<elf_endian>(&header->type, Bits_16(ObjectType::Relocatable));
  Data::write<elf_endian>(&header->machine, Bits_16(Machine::X86_64));
  Data::write<elf_endian>(&header->version, Bits_32(1));
  Data::write<elf_endian>(&header->section_header_offset, section_offset);
  Data::write<elf_endian>(&header->header_size, Bits_16(sizeof(Header)));
  Data::write<elf_endian>(
      &header->section_entry_size, Bits_16(sizeof(SectionHeader)));
  Data::write<elf_endian>(&header->section_count, section_count);
  Data::write<elf_endian>(
      &header->string_section_index, section_string_table_index);
}

static auto write_section_headers(
    Access::Bytes buffer,
    View::Vector<SectionDesc> section_descriptors) -> void {
  auto* entries = Data::cast<SectionHeader>(buffer.get_data());
  for (Count i = 0; i < section_descriptors.get_size(); i++) {
    const auto& d = section_descriptors[i];
    Data::write<elf_endian>(&entries[i].name_offset, Bits_32(d.name_offset));
    Data::write<elf_endian>(&entries[i].type, Bits_32(d.type));
    Data::write<elf_endian>(&entries[i].flags, d.flags);
    Data::write<elf_endian>(&entries[i].file_offset, Bits_64(d.file_offset));
    Data::write<elf_endian>(&entries[i].size, Bits_64(d.data.get_size()));
    Data::write<elf_endian>(&entries[i].link, d.link);
    Data::write<elf_endian>(&entries[i].info, d.info);
    Data::write<elf_endian>(
        &entries[i].alignment, d.alignment > 0 ? d.alignment : Bits_64(1));
    Data::write<elf_endian>(&entries[i].entry_size, d.entry_size);
  }
}

static auto build_section_descriptors(
    View::Vector<Format::Section> sections,
    View::Bytes relocation_data,
    View::Bytes symbol_table,
    View::Bytes string_table,
    Count symbol_table_index,
    Count string_table_index,
    Count first_global,
    Bool has_relocations) -> Dynamic::Vector<SectionDesc> {
  Dynamic::Vector<SectionDesc> descriptors;
  descriptors.insert({});  // [0] null

  for (Count i = 0; i < sections.get_size(); i++) {
    descriptors.insert(to_section_desc(sections[i]));
  }

  if (has_relocations) {
    descriptors.insert({
      ".rela.text"_view,
      SectionType::RelocationAddend,
      0,
      8,
      relocation_data,
      Bits_32(symbol_table_index),
      Bits_32(1),
      sizeof(RelaEntry),
    });
  }

  descriptors.insert({
    ".symtab"_view,
    SectionType::SymbolTable,
    0,
    8,
    symbol_table,
    Bits_32(string_table_index),
    Bits_32(first_global),
    sizeof(SymbolEntry),
  });

  descriptors.insert(
      {".strtab"_view, SectionType::StringTable, 0, 1, string_table});
  descriptors.insert({".shstrtab"_view, SectionType::StringTable, 0, 1});
  return descriptors;
}

auto Elf::build_object() -> Dynamic::Bytes {
  auto sorted = sort_symbols(symbols.get_view());
  auto string_table = build_string_table(sorted.get_access());

  Count first_global = 0;
  auto symbol_table =
      build_symbol_table(sorted.get_view(), sections.get_view(), first_global);
  auto relocation_data =
      build_relocations(relocations.get_view(), sorted.get_view());
  const Count has_relocations = relocations.get_size() > 0 ? 1 : 0;

  // Get the indexs of the additional sections we need to add.
  const Count symbol_table_index = 1 + sections.get_size() + has_relocations;
  const Count string_table_index = symbol_table_index + 1;
  const Count section_string_table_index = string_table_index + 1;
  const Count total = section_string_table_index + 1;

  // Build the final section descriptor set.
  auto section_descriptors = build_section_descriptors(
      sections.get_view(), relocation_data.get_view(), symbol_table.get_view(),
      string_table.get_view(), symbol_table_index, string_table_index,
      first_global, has_relocations);

  // The string table can now be populated as it requires all of the section
  // descriptor names.
  auto section_string_table =
      build_section_string_table(section_descriptors.get_access());
  section_descriptors.get_access()[section_string_table_index].data =
      section_string_table.get_view();

  // Allocates offsets for each header so it has a valid location to write it's
  // data that also meets it's alignment requirements.
  const Count section_headers_offset =
      assign_offsets(section_descriptors.get_access());
  const Count file_size =
      section_headers_offset + sizeof(SectionHeader) * total;

  // Allocate a valid buffer and make sure it's clear of any junk data.
  Dynamic::Bytes output(file_size);
  output.forgetful_resize(file_size);
  output.set(0);
  Byte* buf = output.get_access().get_data();
  memset(buf, 0, file_size);

  // Write the ELF header
  write_header(
      output.get_access(), section_headers_offset, Bits_16(total),
      Bits_16(section_string_table_index));

  // Write the section headers
  write_section_headers(
      output.get_access().slice(
          section_headers_offset, sizeof(SectionHeader) * total),
      section_descriptors.get_view());

  // Write the actual section data
  for (Count i = 0; i < total; i++) {
    const auto& d = section_descriptors.get_view()[i];
    if (d.data.get_size() > 0) {
      Data::copy(
          output.get_access().get_data() + d.file_offset, d.data.get_data(),
          d.data.get_size());
    }
  }

  return output;
}

auto Elf::build_library(View::Bytes object_name) -> Dynamic::Bytes {
  Dynamic::Bytes object = build_object();
  const View::Bytes obj_view = object.get_view();

  // Add symbol entries into the AR for all globally exported symbols.
  Count exported_count = 0;
  Count exported_names_bytes = 0;
  for (Count i = 0; i < symbols.get_size(); i++) {
    if (symbols.get_view()[i].get_visability() ==
        Context::Symbol::Visability::Global) {
      exported_count++;
      exported_names_bytes += symbols.get_view()[i].get_name().get_size() + 1;
    }
  }

  // ar(1) SYSV symtab: 4-byte BE count, 4-byte BE offsets, null-terminated
  // names.
  const Count symbol_table_size = 4 + 4 * exported_count + exported_names_bytes;
  const Count symbol_table_padded = symbol_table_size + (symbol_table_size & 1);
  const Count obj_offset = 8 + sizeof(ArHeader) + symbol_table_padded;
  const Count obj_padded = obj_view.get_size() + (obj_view.get_size() & 1);
  const Count total = obj_offset + sizeof(ArHeader) + obj_padded;

  Dynamic::Bytes output;
  output.forgetful_resize(total);
  Byte* buf = output.get_access().get_data();
  memset(buf, 0, total);

  constexpr auto ar_magic = "!<arch>\n"_view;
  Data::copy(buf, ar_magic.get_data(), ar_magic.get_size());
  Count cursor = 8;

  ArHeader symbol_header;
  fill_ar_header(symbol_header, ""_view, symbol_table_size);
  Data::copy(buf + cursor, &symbol_header, 1);
  cursor += sizeof(ArHeader);

  Byte* pos = buf + cursor;
  Data::write<ar_endian>(Data::cast<Bits_32>(pos), Bits_32(exported_count));
  pos += 4;
  for (Count i = 0; i < exported_count; i++) {
    Data::write<ar_endian>(Data::cast<Bits_32>(pos), Bits_32(obj_offset));
    pos += 4;
  }
  for (Count i = 0; i < symbols.get_size(); i++) {
    const auto& symbol = symbols.get_view()[i];
    if (symbol.get_visability() != Context::Symbol::Visability::Global) {
      continue;
    }
    const auto name = symbol.get_name();
    Data::copy(pos, name.get_data(), name.get_size());
    pos += name.get_size();
    *pos++ = '\0';
  }
  cursor += symbol_table_padded;

  // Write the actual object file into the archive.
  ArHeader object_header;
  fill_ar_header(object_header, object_name, Bits_64(obj_view.get_size()));
  Data::copy(buf + cursor, &object_header, 1);
  cursor += sizeof(ArHeader);
  Data::copy(buf + cursor, obj_view.get_data(), obj_view.get_size());

  return output;
}
