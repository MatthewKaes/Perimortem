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
using namespace Tetrodotoxin::Linker;

// Elf is a little endian format, however the ar format has
static constexpr auto elf_endian = Data::ByteOrder::Little;
static constexpr auto ar_endian = Data::ByteOrder::Big;

enum class ObjectType : Unsigned_16 { Relocatable = 1 };
enum class Machine : Unsigned_16 { X86_64 = 62 };

enum class SectionType : Unsigned_32 {
  Null = 0,
  ProgramData = 1,
  SymbolTable = 2,
  StringTable = 3,
  RelocationAddend = 4,
};

enum class SectionFlags : Unsigned_64 {
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
  Unsigned_16 type;
  Unsigned_16 machine;
  Unsigned_32 version;
  Unsigned_64 entry_point;
  Unsigned_64 program_header_offset;
  Unsigned_64 section_header_offset;
  Unsigned_32 flags;
  Unsigned_16 header_size;
  Unsigned_16 program_header_entry_size;
  Unsigned_16 program_header_count;
  Unsigned_16 section_entry_size;
  Unsigned_16 section_count;
  Unsigned_16 string_section_index;
};
static_assert(sizeof(Header) == 64);

// Contains the binary layout for the section headers.
// The ELF file can contain any number of sections.
struct SectionHeader {
  Unsigned_32 name_offset;
  Unsigned_32 type;
  Unsigned_64 flags;
  Unsigned_64 virtual_address;
  Unsigned_64 file_offset;
  Unsigned_64 size;
  Unsigned_32 link;
  Unsigned_32 info;
  Unsigned_64 alignment;
  Unsigned_64 entry_size;
};
static_assert(sizeof(SectionHeader) == 64);

// The binary layout for symbol records.
// The object model uses ELF-compatible values so this wire record can serialize
// symbols directly without understanding the generator that produced them.
struct SymbolRecord {
  Unsigned_32 name_offset;
  Unsigned_8 info;        // (binding << 4) | type
  Unsigned_8 visibility;  // STV_DEFAULT = 0
  Unsigned_16 section_index;
  Unsigned_64 value;
  Unsigned_64 size;
};
static_assert(sizeof(SymbolRecord) == 24);

// Rela represents relative relocations
struct RelaRecord {
  Unsigned_64 offset;
  Unsigned_64 info;  // (symbol_index << 32) | reloc_type
  Signed_64 addend;
};
static_assert(sizeof(RelaRecord) == 24);

enum class RelocationType : Unsigned_32 {
  PcRelative32 = 2,
  Plt32 = 4,
};

// Carries every field needed to write a section header. name_offset and
// file_offset are filled in during the build pipeline.
struct SectionDesc {
  View::Bytes name;
  SectionType type = SectionType::Null;
  Unsigned_64 flags = 0;
  Unsigned_64 alignment = 0;
  View::Bytes data;
  Unsigned_32 link = 0;
  Unsigned_32 info = 0;
  Unsigned_64 entry_size = 0;
  Count name_offset = 0;
  Count file_offset = 0;
};

struct SymbolRef {
  const Object::Symbol* symbol;
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

static auto fill_ar_header(ArHeader& header, View::Bytes name, Unsigned_64 size)
    -> void {
  const Count name_length = name.get_size() < 15 ? name.get_size() : 15;
  Data::copy(header.name.get_data(), name.get_data(), name_length);
  header.name[name_length] = '/';

  // Write only the data size, the rest of the fields are default initialized.
  Writer::Textual(header.data_size.get_access()) << size;
}

static auto to_section_desc(Object::Section section) -> SectionDesc {
  switch (section.get_type()) {
  case Object::Section::Type::Undefined:
    return {};
  case Object::Section::Type::Program:
    return {
      ".text"_view,
      SectionType::ProgramData,
      Unsigned_64(SectionFlags::Allocated) |
          Unsigned_64(SectionFlags::Executable),
      16,
      section.get_data(),
    };
  case Object::Section::Type::Strings:
    return {
      ".rodata.str"_view,
      SectionType::ProgramData,
      Unsigned_64(SectionFlags::Allocated) |
          Unsigned_64(SectionFlags::Mergeable) |
          Unsigned_64(SectionFlags::Strings),
      1,
      section.get_data(),
    };
  case Object::Section::Type::ReadOnly:
    return {
      ".rodata"_view,
      SectionType::ProgramData,
      Unsigned_64(SectionFlags::Allocated),
      8,
      section.get_data(),
    };
  default:
    return {};
  }
}

static auto relocation_name_for(Object::Section::Type type) -> View::Bytes {
  switch (type) {
  case Object::Section::Type::Program:
    return ".rela.text"_view;
  case Object::Section::Type::Strings:
    return ".rela.rodata.str"_view;
  case Object::Section::Type::ReadOnly:
    return ".rela.rodata"_view;
  default:
    return ".rela"_view;
  }
}

static auto sort_symbols(View::Vector<Object::Symbol> symbols)
    -> Dynamic::Vector<SymbolRef> {
  Dynamic::Vector<SymbolRef> sorted;
  for (Count pass = 0; pass <= Count(Object::Symbol::Visibility::Global);
       pass++) {
    for (Count i = 0; i < symbols.get_size(); i++) {
      if (Count(symbols[i].get_visibility()) != pass) {
        continue;
      }

      sorted.insert({symbols.get_data() + i, i, 0});
    }
  }

  return sorted;
}

static auto build_symbol_slots(View::Vector<SymbolRef> sorted)
    -> Dynamic::Vector<Unsigned_32> {
  Dynamic::Vector<Unsigned_32> slots;
  slots.resize(sorted.get_size());
  for (Count i = 0; i < sorted.get_size(); i++) {
    slots[sorted[i].original_index] = Unsigned_32(1 + i);
  }

  return slots;
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

static auto build_symbol_table(View::Vector<SymbolRef> sorted)
    -> Dynamic::Bytes {
  const Count entry_count = 1 + sorted.get_size();
  Dynamic::Bytes data;
  data.forgetful_resize(sizeof(SymbolRecord) * entry_count);
  memset(data.get_access().get_data(), 0, sizeof(SymbolRecord) * entry_count);
  auto* entries = Data::cast<SymbolRecord>(data.get_access().get_data());
  for (Count i = 0; i < sorted.get_size(); i++) {
    const auto& ref = sorted[i];
    const auto& symbol = *ref.symbol;
    auto& entry = entries[1 + i];
    const Unsigned_8 binding =
        symbol.get_visibility() == Object::Symbol::Visibility::Global ? 1 : 0;
    Data::write<elf_endian>(
        &entry.name_offset, Unsigned_32(ref.string_table_offset));
    entry.info = Unsigned_8((binding << 4) | Unsigned_8(symbol.get_type()));
    Data::write<elf_endian>(&entry.section_index, symbol.get_section_index());
    Data::write<elf_endian>(
        &entry.value, Unsigned_64(symbol.get_range().start));
    Data::write<elf_endian>(&entry.size, Unsigned_64(symbol.get_range().size));
  }

  return data;
}

static auto write_relocations(
    Access::Bytes data,
    View::Vector<Object::Relocation> relocations,
    View::Vector<Unsigned_32> symbol_slots) -> void {
  auto* entries = Data::cast<RelaRecord>(data.get_data());
  for (Count i = 0; i < relocations.get_size(); i++) {
    const auto& reloc = relocations[i];
    const Unsigned_32 rtype =
        reloc.get_type() == Object::Relocation::Type::Plt32
            ? Unsigned_32(RelocationType::Plt32)
            : Unsigned_32(RelocationType::PcRelative32);
    Data::write<elf_endian>(
        &entries[i].offset, Unsigned_64(reloc.get_offset()));
    Data::write<elf_endian>(
        &entries[i].info,
        (Unsigned_64(symbol_slots[reloc.get_symbol()]) << 32) |
            Unsigned_64(rtype));
    Data::write<elf_endian>(&entries[i].addend, Signed_64(reloc.get_addend()));
  }
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

Target::Elf::Elf() {
  reset();
}

auto Target::Elf::add_section(Object::Section section) -> Unsigned_16 {
  const Unsigned_16 index = Unsigned_16(sections.get_size());
  sections.insert(section);
  relocation_tables.insert(Dynamic::Vector<Object::Relocation>());
  return index;
}

auto Target::Elf::add_symbol(Object::Symbol symbol) -> void {
  symbols.insert(symbol);
}

auto Target::Elf::add_relocation(Object::Relocation relocation) -> void {
  if (relocation_tables[relocation.get_section_index()].get_size() == 0) {
    relocation_section_count++;
  }

  relocation_tables[relocation.get_section_index()].insert(relocation);
  relocation_count++;
}

auto Target::Elf::reset() -> void {
  sections.clear();
  relocation_tables.clear();
  sections.insert(Object::Section::undefined());
  relocation_tables.insert(Dynamic::Vector<Object::Relocation>());
  symbols.clear();
  relocation_count = 0;
  relocation_section_count = 0;
}

auto Target::Elf::write_header(
    Access::Bytes buffer,
    Unsigned_64 section_offset,
    Unsigned_16 section_count,
    Unsigned_16 section_string_table_index) -> void {
  Static::Bytes<16> identity = {
    0x7F, 'E', 'L', 'F', 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };
  auto* header = Data::cast<Header>(buffer.get_data());
  header->identity = identity;
  Data::write<elf_endian>(&header->type, Unsigned_16(ObjectType::Relocatable));
  Data::write<elf_endian>(&header->machine, Unsigned_16(Machine::X86_64));
  Data::write<elf_endian>(&header->version, Unsigned_32(1));
  Data::write<elf_endian>(&header->section_header_offset, section_offset);
  Data::write<elf_endian>(&header->header_size, Unsigned_16(sizeof(Header)));
  Data::write<elf_endian>(
      &header->section_entry_size, Unsigned_16(sizeof(SectionHeader)));
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
    Data::write<elf_endian>(
        &entries[i].name_offset, Unsigned_32(d.name_offset));
    Data::write<elf_endian>(&entries[i].type, Unsigned_32(d.type));
    Data::write<elf_endian>(&entries[i].flags, d.flags);
    Data::write<elf_endian>(
        &entries[i].file_offset, Unsigned_64(d.file_offset));
    Data::write<elf_endian>(&entries[i].size, Unsigned_64(d.data.get_size()));
    Data::write<elf_endian>(&entries[i].link, d.link);
    Data::write<elf_endian>(&entries[i].info, d.info);
    Data::write<elf_endian>(
        &entries[i].alignment, d.alignment > 0 ? d.alignment : Unsigned_64(1));
    Data::write<elf_endian>(&entries[i].entry_size, d.entry_size);
  }
}

static auto build_section_descriptors(
    View::Vector<Object::Section> sections,
    Access::Bytes relocation_data,
    View::Vector<Dynamic::Vector<Object::Relocation>> relocation_tables,
    View::Vector<SymbolRef> sorted_symbols,
    View::Vector<Unsigned_32> symbol_slots,
    View::Bytes symbol_table,
    View::Bytes string_table,
    Count symbol_table_index,
    Count string_table_index) -> Dynamic::Vector<SectionDesc> {
  Dynamic::Vector<SectionDesc> descriptors;
  for (Count i = 0; i < sections.get_size(); i++) {
    descriptors.insert(to_section_desc(sections[i]));
  }

  Count relocation_offset = 0;
  for (Count i = 1; i < sections.get_size(); i++) {
    View::Vector<Object::Relocation> relocations = relocation_tables[i];
    if (relocations.get_size() == 0) {
      continue;
    }

    const Count data_size = sizeof(RelaRecord) * relocations.get_size();
    Access::Bytes data = relocation_data.slice(relocation_offset, data_size);
    write_relocations(data, relocations, symbol_slots);
    relocation_offset += data_size;

    descriptors.insert({
      relocation_name_for(sections[i].get_type()),
      SectionType::RelocationAddend,
      0,
      8,
      data,
      Unsigned_32(symbol_table_index),
      Unsigned_32(i),
      sizeof(RelaRecord),
    });
  }

  // SHT_SYMTAB stores the first non-local symbol slot in sh_info. Slot zero is
  // the null symbol, and sort_symbols already places local symbols first.
  Count first_non_local_symbol = 1;
  for (Count i = 0; i < sorted_symbols.get_size(); i++) {
    if (sorted_symbols[i].symbol->get_visibility() !=
        Object::Symbol::Visibility::Local) {
      break;
    }

    first_non_local_symbol++;
  }

  descriptors.insert({
    ".symtab"_view,
    SectionType::SymbolTable,
    0,
    8,
    symbol_table,
    Unsigned_32(string_table_index),
    Unsigned_32(first_non_local_symbol),
    sizeof(SymbolRecord),
  });

  descriptors.insert(
      {".strtab"_view, SectionType::StringTable, 0, 1, string_table});
  descriptors.insert({".shstrtab"_view, SectionType::StringTable, 0, 1});
  return descriptors;
}

auto Target::Elf::build_object() -> Dynamic::Bytes {
  auto sorted = sort_symbols(symbols.get_view());
  auto symbol_slots = build_symbol_slots(sorted.get_view());
  auto string_table = build_string_table(sorted.get_access());
  auto symbol_table = build_symbol_table(sorted.get_view());

  Dynamic::Bytes relocation_data;
  relocation_data.forgetful_resize(sizeof(RelaRecord) * relocation_count);

  // Get the indexs of the additional sections we need to add.
  const Count symbol_table_index =
      sections.get_size() + relocation_section_count;
  const Count string_table_index = symbol_table_index + 1;
  const Count section_string_table_index = string_table_index + 1;
  const Count total = section_string_table_index + 1;

  // Build the final section descriptor set.
  auto section_descriptors = build_section_descriptors(
      sections.get_view(), relocation_data.get_access(),
      relocation_tables.get_view(), sorted.get_view(), symbol_slots.get_view(),
      symbol_table, string_table, symbol_table_index, string_table_index);

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
      output.get_access(), section_headers_offset, Unsigned_16(total),
      Unsigned_16(section_string_table_index));

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

auto Target::Elf::build_library(View::Bytes object_name) -> Dynamic::Bytes {
  Dynamic::Bytes object = build_object();
  const View::Bytes object_view = object.get_view();

  // Add symbol entries into the AR for all globally exported symbols.
  Count exported_count = 0;
  Count exported_names_bytes = 0;
  for (Count i = 0; i < symbols.get_size(); i++) {
    if (symbols.get_view()[i].get_visibility() ==
            Object::Symbol::Visibility::Global &&
        !symbols.get_view()[i].is_external()) {
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
  Unsigned_8* output_bytes = output.get_access().get_data();
  memset(output_bytes, 0, total);

  constexpr auto ar_magic = "!<arch>\n"_view;
  Data::copy(output_bytes, ar_magic.get_data(), ar_magic.get_size());
  Count cursor = 8;

  ArHeader symbol_header;
  fill_ar_header(symbol_header, ""_view, symbol_table_size);
  Data::copy(output_bytes + cursor, &symbol_header, 1);
  cursor += sizeof(ArHeader);

  Unsigned_8* write_pointer = output_bytes + cursor;
  Data::write<ar_endian>(
      Data::cast<Unsigned_32>(write_pointer), Unsigned_32(exported_count));
  write_pointer += 4;
  for (Count i = 0; i < exported_count; i++) {
    Data::write<ar_endian>(
        Data::cast<Unsigned_32>(write_pointer), Unsigned_32(object_offset));
    write_pointer += 4;
  }

  for (Count i = 0; i < symbols.get_size(); i++) {
    const auto& symbol = symbols.get_view()[i];
    if (symbol.get_visibility() != Object::Symbol::Visibility::Global ||
        symbol.is_external()) {
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
  fill_ar_header(
      object_header, object_name, Unsigned_64(object_view.get_size()));
  Data::copy(output_bytes + cursor, &object_header, 1);
  cursor += sizeof(ArHeader);
  Data::copy(
      output_bytes + cursor, object_view.get_data(), object_view.get_size());
  return output;
}
