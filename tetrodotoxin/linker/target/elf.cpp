// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/linker/target/elf.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Linker::Object;
using namespace Tetrodotoxin::Linker::Target;

// Elf is a little endian format, however the ar format has
static constexpr auto elf_endian = Data::ByteOrder::Little;
static constexpr auto ar_endian = Data::ByteOrder::Big;

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

// The binary layout for symbol records.
// The object model uses ELF-compatible values so this wire record can serialize
// symbols directly without understanding the generator that produced them.
struct SymbolRecord {
  Bits_32 name_offset;
  Bits_8 info;        // (binding << 4) | type
  Bits_8 visibility;  // STV_DEFAULT = 0
  Bits_16 section_index;
  Bits_64 value;
  Bits_64 size;
};
static_assert(sizeof(SymbolRecord) == 24);

// Rela represents relative relocations
struct RelaRecord {
  Bits_64 offset;
  Bits_64 info;  // (symbol_index << 32) | reloc_type
  Signed_64 addend;
};
static_assert(sizeof(RelaRecord) == 24);

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
  const Symbol* symbol;
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

static auto fill_ar_header(ArHeader& header, View::Bytes name, Bits_64 size)
    -> void {
  const Count name_length = name.get_size() < 15 ? name.get_size() : 15;
  Data::copy(header.name.get_data(), name.get_data(), name_length);
  header.name[name_length] = '/';

  // Write only the data size, the rest of the fields are default initialized.
  Writer::Textual(header.data_size.get_access()) << size;
}

static auto to_section_desc(Section section) -> SectionDesc {
  switch (section.get_type()) {
  case Section::Type::Program:
    return {
      ".text"_view,
      SectionType::ProgramData,
      Bits_64(SectionFlags::Allocated) | Bits_64(SectionFlags::Executable),
      16,
      section.get_data(),
    };
  case Section::Type::Strings:
    return {
      ".rodata.str"_view,
      SectionType::ProgramData,
      Bits_64(SectionFlags::Allocated) | Bits_64(SectionFlags::Mergeable) |
          Bits_64(SectionFlags::Strings),
      1,
      section.get_data(),
    };
  case Section::Type::ReadOnly:
    return {
      ".rodata"_view,
      SectionType::ProgramData,
      Bits_64(SectionFlags::Allocated),
      8,
      section.get_data(),
    };
  default:
    return {};
  }
}

static auto sort_symbols(View::Vector<Symbol> symbols)
    -> Dynamic::Vector<SymbolRef> {
  Dynamic::Vector<SymbolRef> sorted;
  using Visibility = Symbol::Visibility;
  for (Count pass = 0; pass <= Count(Visibility::Global); pass++) {
    for (Count i = 0; i < symbols.get_size(); i++) {
      if (Count(symbols[i].get_visibility()) != pass) {
        continue;
      }
      sorted.insert({symbols.get_data() + i, i, 0});
    }
  }
  return sorted;
}

static auto find_sorted_position(
    View::Vector<SymbolRef> sorted,
    Count original_index) -> Count {
  for (Count i = 0; i < sorted.get_size(); i++) {
    if (sorted[i].original_index == original_index) {
      return i;
    }
  }
  return 0;
}

static auto build_string_table(Access::Vector<SymbolRef> symbols)
    -> Dynamic::Bytes {
  Dynamic::Bytes string_table;
  string_table.append('\0');
  for (Count i = 0; i < symbols.get_size(); i++) {
    symbols[i].string_table_offset = string_table.get_size();
    string_table.concat(symbols[i].symbol->get_name());
    string_table.append('\0');
  }
  return string_table;
}

// Returns the 1-based ELF section index for a symbol's location, or 0
// (SHN_UNDEF) if not found.
static auto section_type_for(Symbol::Location location) -> Section::Type {
  switch (location) {
  case Symbol::Location::Program:
    return Section::Type::Program;
  case Symbol::Location::Strings:
    return Section::Type::Strings;
  case Symbol::Location::ReadOnly:
    return Section::Type::ReadOnly;
  default:
    return Section::Type::Invalid;
  }
}

static auto section_index_for(
    View::Vector<Section> sections,
    Symbol::Location location) -> Bits_16 {
  const Section::Type type = section_type_for(location);
  if (type == Section::Type::Invalid) {
    return 0;
  }

  for (Count i = 0; i < sections.get_size(); i++) {
    if (sections[i].get_type() == type) {
      return Bits_16(1 + i);
    }
  }
  return 0;
}

static auto build_symbol_table(
    View::Vector<SymbolRef> sorted,
    View::Vector<Section> sections,
    Count& out_first_global) -> Dynamic::Bytes {
  out_first_global = 1;
  for (Count i = 0; i < sorted.get_size(); i++) {
    if (sorted[i].symbol->get_visibility() == Symbol::Visibility::Global) {
      break;
    }
    out_first_global++;
  }

  const Count entry_count = 1 + sorted.get_size();
  Dynamic::Bytes data;
  data.forgetful_resize(sizeof(SymbolRecord) * entry_count);
  memset(data.get_access().get_data(), 0, sizeof(SymbolRecord) * entry_count);
  auto* entries = Data::cast<SymbolRecord>(data.get_access().get_data());

  for (Count i = 0; i < sorted.get_size(); i++) {
    const auto& ref = sorted[i];
    const auto& symbol = *ref.symbol;
    auto& entry = entries[1 + i];
    const Bits_8 binding =
        symbol.get_visibility() == Symbol::Visibility::Global ? 1 : 0;
    Data::write<elf_endian>(
        &entry.name_offset, Bits_32(ref.string_table_offset));
    entry.info = Bits_8((binding << 4) | Bits_8(symbol.get_type()));
    const Bits_16 section_index =
        symbol.get_location() == Symbol::Location::External
            ? Bits_16(0)
            : section_index_for(sections, symbol.get_location());
    Data::write<elf_endian>(&entry.section_index, section_index);
    Data::write<elf_endian>(&entry.value, Bits_64(symbol.get_range().start));
    Data::write<elf_endian>(&entry.size, Bits_64(symbol.get_range().size));
  }
  return data;
}

static auto build_relocations(
    View::Vector<Relocation> relocations,
    View::Vector<SymbolRef> sorted) -> Dynamic::Bytes {
  Dynamic::Bytes data;
  if (relocations.get_size() == 0) {
    return data;
  }
  data.forgetful_resize(sizeof(RelaRecord) * relocations.get_size());
  auto* entries = Data::cast<RelaRecord>(data.get_access().get_data());

  for (Count i = 0; i < relocations.get_size(); i++) {
    const auto& reloc = relocations[i];
    const Bits_64 symbol_slot =
        Bits_64(1 + find_sorted_position(sorted, reloc.get_symbol()));
    const Bits_32 rtype = reloc.get_type() == Relocation::Type::Plt32
                              ? Bits_32(RelocationType::Plt32)
                              : Bits_32(RelocationType::PcRelative32);
    Data::write<elf_endian>(&entries[i].offset, Bits_64(reloc.get_offset()));
    Data::write<elf_endian>(
        &entries[i].info, (symbol_slot << 32) | Bits_64(rtype));
    Data::write<elf_endian>(
        &entries[i].addend, Signed_64(reloc.get_addend()));
  }
  return data;
}

static auto build_section_string_table(Access::Vector<SectionDesc> sections)
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

static auto assign_offsets(Access::Vector<SectionDesc> sections) -> Count {
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

auto Elf::add_section(Section section) -> void {
  sections.insert(section);
}

auto Elf::add_symbol(Symbol symbol) -> void {
  symbols.insert(symbol);
}

auto Elf::add_relocation(Relocation relocation) -> void {
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
  Static::Bytes<16> identity = {
    0x7F, 'E', 'L', 'F', 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
    View::Vector<Section> sections,
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
      sizeof(RelaRecord),
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
    sizeof(SymbolRecord),
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
  auto output_bytes = output.get_access().get_data();
  memset(output_bytes, 0, file_size);

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
    const auto& descriptor = section_descriptors.get_view()[i];
    if (descriptor.data.get_size() > 0) {
      Data::copy(
          output.get_access().get_data() + descriptor.file_offset,
          descriptor.data.get_data(), descriptor.data.get_size());
    }
  }

  return output;
}

auto Elf::build_library(View::Bytes object_name) -> Dynamic::Bytes {
  Dynamic::Bytes object = build_object();
  const View::Bytes object_view = object.get_view();

  // Add symbol entries into the AR for all globally exported symbols.
  Count exported_count = 0;
  Count exported_names_bytes = 0;
  for (Count i = 0; i < symbols.get_size(); i++) {
    if (symbols.get_view()[i].get_visibility() == Symbol::Visibility::Global) {
      exported_count++;
      exported_names_bytes += symbols.get_view()[i].get_name().get_size() + 1;
    }
  }

  // ar(1) SYSV symtab: 4-byte BE count, 4-byte BE offsets, null-terminated
  // names.
  const Count symbol_table_size = 4 + 4 * exported_count + exported_names_bytes;
  const Count symbol_table_padded = symbol_table_size + (symbol_table_size & 1);
  const Count object_offset = 8 + sizeof(ArHeader) + symbol_table_padded;
  const Count object_padded =
      object_view.get_size() + (object_view.get_size() & 1);
  const Count total = object_offset + sizeof(ArHeader) + object_padded;

  Dynamic::Bytes output;
  output.forgetful_resize(total);
  Bits_8* output_bytes = output.get_access().get_data();
  memset(output_bytes, 0, total);

  constexpr auto ar_magic = "!<arch>\n"_view;
  Data::copy(output_bytes, ar_magic.get_data(), ar_magic.get_size());
  Count cursor = 8;

  ArHeader symbol_header;
  fill_ar_header(symbol_header, ""_view, symbol_table_size);
  Data::copy(output_bytes + cursor, &symbol_header, 1);
  cursor += sizeof(ArHeader);

  Bits_8* write_pointer = output_bytes + cursor;
  Data::write<ar_endian>(
      Data::cast<Bits_32>(write_pointer), Bits_32(exported_count));
  write_pointer += 4;
  for (Count i = 0; i < exported_count; i++) {
    Data::write<ar_endian>(
        Data::cast<Bits_32>(write_pointer), Bits_32(object_offset));
    write_pointer += 4;
  }
  for (Count i = 0; i < symbols.get_size(); i++) {
    const auto& symbol = symbols.get_view()[i];
    if (symbol.get_visibility() != Symbol::Visibility::Global) {
      continue;
    }
    const auto name = symbol.get_name();
    Data::copy(write_pointer, name.get_data(), name.get_size());
    write_pointer += name.get_size();
    *write_pointer++ = '\0';
  }
  cursor += symbol_table_padded;

  // Write the actual object file into the archive.
  ArHeader object_header;
  fill_ar_header(object_header, object_name, Bits_64(object_view.get_size()));
  Data::copy(output_bytes + cursor, &object_header, 1);
  cursor += sizeof(ArHeader);
  Data::copy(
      output_bytes + cursor, object_view.get_data(), object_view.get_size());

  return output;
}
