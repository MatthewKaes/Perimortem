// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/linker/target/elf.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Linker;

// ELF fields use little endian encoding while System V archive indices use big
// endian encoding. Keeping both orders named at the wire boundary makes every
// write below state which container owns it.
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

// The ELF header uses fixed width fields so its in memory record remains the
// exact 64 byte wire shape required by the format.
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

// Each section header uses the fixed 64 byte ELF wire shape. Object inventory
// counts remain outside this record until one encoding transaction assigns the
// final offsets and links.
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

// Symbol records contain only ELF wire values. Object vocabulary is translated
// explicitly before reaching this record so its enum ordinals remain private
// to the Module owner.
struct SymbolRecord {
  Unsigned_32 name_offset;
  Unsigned_8 info;
  Unsigned_8 visibility;
  Unsigned_16 section_index;
  Unsigned_64 value;
  Unsigned_64 size;
};
static_assert(sizeof(SymbolRecord) == 24);

// Rela keeps the explicit addend used by each relative relocation.
struct RelaRecord {
  Unsigned_64 offset;
  Unsigned_64 info;
  Signed_64 addend;
};
static_assert(sizeof(RelaRecord) == 24);

enum class RelocationType : Unsigned_32 {
  PcRelative32 = 2,
  Plt32 = 4,
};

// SectionDescriptor retains one transaction's section header inputs. Name and
// file offsets stay local because they depend on the final encoded inventory.
struct SectionDescriptor {
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

struct SymbolReference {
  const Object::Symbol* symbol;
  Count string_table_offset;
};

// System V archive headers use left aligned textual fields padded with spaces.
// The awkward representation stays here so Module values never absorb archive
// publication details.
struct ArHeader {
  Static::Bytes<16> name = "                "_bytes;
  Static::Bytes<12> timestamp = "0           "_bytes;
  Static::Bytes<6> owner_id = "0     "_bytes;
  Static::Bytes<6> group_id = "0     "_bytes;
  Static::Bytes<8> file_mode = "644     "_bytes;
  Static::Bytes<10> data_size = "          "_bytes;
  Static::Bytes<2> terminator = "`\n"_bytes;
};
static_assert(sizeof(ArHeader) == 60);

static auto fill_ar_header(ArHeader& header, View::Bytes name, Unsigned_64 size)
    -> void {
  const Count name_length = name.get_size() < 15 ? name.get_size() : 15;
  Data::copy(header.name.get_data(), name.get_data(), name_length);
  header.name[name_length] = '/';

  Writer::Textual(header.data_size.get_access()) << size;
}

static auto to_section_descriptor(const Object::Section& section)
    -> SectionDescriptor {
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

static auto to_symbol_binding(Object::Symbol::Visibility visibility)
    -> Unsigned_8 {
  switch (visibility) {
  case Object::Symbol::Visibility::Local:
    return 0;
  case Object::Symbol::Visibility::Global:
    return 1;
  default:
    return 0;
  }
}

static auto to_symbol_type(Object::Symbol::Type type) -> Unsigned_8 {
  switch (type) {
  case Object::Symbol::Type::None:
    return 0;
  case Object::Symbol::Type::Object:
    return 1;
  case Object::Symbol::Type::Function:
    return 2;
  default:
    return 0;
  }
}

static auto to_relocation_type(Object::Relocation::Type type) -> Unsigned_32 {
  switch (type) {
  case Object::Relocation::Type::Pc32:
    return Unsigned_32(RelocationType::PcRelative32);
  case Object::Relocation::Type::Plt32:
    return Unsigned_32(RelocationType::Plt32);
  default:
    return 0;
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

static auto append_symbols(
    Dynamic::Vector<SymbolReference>& sorted_symbols,
    View::Vector<Object::Symbol> symbols,
    Object::Symbol::Visibility visibility) -> void {
  for (Count i = 0; i < symbols.get_size(); i++) {
    if (symbols.get_data()[i].get_visibility() == visibility) {
      sorted_symbols.insert({symbols.get_data() + i, 0});
    }
  }
}

static auto sort_symbols(View::Vector<Object::Symbol> symbols)
    -> Dynamic::Vector<SymbolReference> {
  Dynamic::Vector<SymbolReference> sorted_symbols;
  append_symbols(sorted_symbols, symbols, Object::Symbol::Visibility::Local);
  append_symbols(sorted_symbols, symbols, Object::Symbol::Visibility::Global);
  return sorted_symbols;
}

static auto build_symbol_slots(View::Vector<Object::Symbol> symbols)
    -> Dynamic::Vector<Unsigned_32> {
  Count local_count = 0;
  const auto* symbol_data = symbols.get_data();
  for (Count i = 0; i < symbols.get_size(); i++) {
    local_count +=
        symbol_data[i].get_visibility() == Object::Symbol::Visibility::Local;
  }

  Dynamic::Vector<Unsigned_32> symbol_slots(symbols.get_size());
  Count next_local_slot = 1;
  Count next_global_slot = 1 + local_count;
  for (Count i = 0; i < symbols.get_size(); i++) {
    const Bool is_local =
        symbol_data[i].get_visibility() == Object::Symbol::Visibility::Local;
    Count& next_slot = is_local ? next_local_slot : next_global_slot;
    symbol_slots.insert(Unsigned_32(next_slot));
    next_slot++;
  }

  return symbol_slots;
}

static auto build_string_table(Access::Vector<SymbolReference> symbols)
    -> Dynamic::Bytes {
  Dynamic::Bytes string_table;
  string_table.append('\0');
  auto* symbol_data = symbols.get_data();
  for (Count i = 0; i < symbols.get_size(); i++) {
    symbol_data[i].string_table_offset = string_table.get_size();
    string_table.concat(symbol_data[i].symbol->get_name());
    string_table.append('\0');
  }

  return string_table;
}

static auto build_symbol_table(View::Vector<SymbolReference> sorted_symbols)
    -> Dynamic::Bytes {
  const Count entry_count = 1 + sorted_symbols.get_size();
  Dynamic::Bytes data;
  data.forgetful_resize(sizeof(SymbolRecord) * entry_count);
  memset(data.get_access().get_data(), 0, sizeof(SymbolRecord) * entry_count);
  auto* entries = Data::cast<SymbolRecord>(data.get_access().get_data());
  const auto* sorted_symbol_data = sorted_symbols.get_data();
  for (Count i = 0; i < sorted_symbols.get_size(); i++) {
    const auto& reference = sorted_symbol_data[i];
    const auto& symbol = *reference.symbol;
    auto& entry = entries[1 + i];
    const Unsigned_8 binding = to_symbol_binding(symbol.get_visibility());
    const Unsigned_8 type = to_symbol_type(symbol.get_type());
    Data::write<elf_endian>(
        &entry.name_offset, Unsigned_32(reference.string_table_offset));
    entry.info = Unsigned_8((binding << 4) | type);
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
  const auto* symbol_slot_data = symbol_slots.get_data();
  for (Count i = 0; i < relocations.get_size(); i++) {
    const auto& relocation = relocations.get_data()[i];
    const Unsigned_32 relocation_type =
        to_relocation_type(relocation.get_type());
    Data::write<elf_endian>(
        &entries[i].offset, Unsigned_64(relocation.get_offset()));
    Data::write<elf_endian>(
        &entries[i].info,
        (Unsigned_64(symbol_slot_data[relocation.get_symbol()]) << 32) |
            Unsigned_64(relocation_type));
    Data::write<elf_endian>(
        &entries[i].addend, Signed_64(relocation.get_addend()));
  }
}

static auto build_section_string_table(
    Access::Vector<SectionDescriptor> sections) -> Dynamic::Bytes {
  Dynamic::Bytes section_string_table;
  section_string_table.append('\0');
  auto* section_data = sections.get_data();
  for (Count i = 0; i < sections.get_size(); i++) {
    if (section_data[i].name.get_size() == 0) {
      section_data[i].name_offset = 0;
      continue;
    }

    section_data[i].name_offset = section_string_table.get_size();
    section_string_table.concat(section_data[i].name);
    section_string_table.append('\0');
  }

  return section_string_table;
}

static auto assign_offsets(Access::Vector<SectionDescriptor> sections)
    -> Count {
  Count offset = sizeof(Header);
  auto* section_data = sections.get_data();
  for (Count i = 0; i < sections.get_size(); i++) {
    if (section_data[i].data.get_size() == 0) {
      section_data[i].file_offset = 0;
      continue;
    }

    const Count alignment =
        section_data[i].alignment > 0 ? Count(section_data[i].alignment) : 1;
    offset = (offset + alignment - 1) & ~(alignment - 1);
    section_data[i].file_offset = offset;
    offset += section_data[i].data.get_size();
  }

  return Data::align<8>(offset);
}

static auto write_header(
    Access::Bytes buffer,
    Unsigned_64 section_offset,
    Unsigned_16 section_count,
    Unsigned_16 section_string_table_index) -> void {
  Static::Bytes<16> identity = {
    {0x7F, 'E', 'L', 'F', 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
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

static auto group_relocations(
    Count section_count,
    View::Vector<Object::Relocation> relocations)
    -> Dynamic::Vector<Dynamic::Vector<Object::Relocation>> {
  Dynamic::Vector<Dynamic::Vector<Object::Relocation>> relocation_tables;
  for (Count i = 0; i < section_count; i++) {
    relocation_tables.insert(Dynamic::Vector<Object::Relocation>());
  }

  for (Count i = 0; i < relocations.get_size(); i++) {
    const auto& relocation = relocations.get_data()[i];
    const Count section_index = relocation.get_section_index();
    // Module admission proves this edge before target encoding. Repeating that
    // proof here prevents a corrupt Module from escaping as an invalid object.
    if (section_index >= relocation_tables.get_size()) {
      Diagnostics::Log::fatal(
          "ELF encoder received a relocation for an absent Section."_view);
    }
    relocation_tables[section_index].insert(relocation);
  }

  return relocation_tables;
}

static auto write_section_headers(
    Access::Bytes buffer,
    View::Vector<SectionDescriptor> section_descriptors) -> void {
  auto* entries = Data::cast<SectionHeader>(buffer.get_data());
  const auto* descriptor_data = section_descriptors.get_data();
  for (Count i = 0; i < section_descriptors.get_size(); i++) {
    const auto& descriptor = descriptor_data[i];
    Data::write<elf_endian>(
        &entries[i].name_offset, Unsigned_32(descriptor.name_offset));
    Data::write<elf_endian>(&entries[i].type, Unsigned_32(descriptor.type));
    Data::write<elf_endian>(&entries[i].flags, descriptor.flags);
    Data::write<elf_endian>(
        &entries[i].file_offset, Unsigned_64(descriptor.file_offset));
    Data::write<elf_endian>(
        &entries[i].size, Unsigned_64(descriptor.data.get_size()));
    Data::write<elf_endian>(&entries[i].link, descriptor.link);
    Data::write<elf_endian>(&entries[i].info, descriptor.info);
    Data::write<elf_endian>(
        &entries[i].alignment,
        descriptor.alignment > 0 ? descriptor.alignment : Unsigned_64(1));
    Data::write<elf_endian>(&entries[i].entry_size, descriptor.entry_size);
  }
}

static auto build_section_descriptors(
    View::Vector<Object::Section> sections,
    Access::Bytes relocation_data,
    View::Vector<Dynamic::Vector<Object::Relocation>> relocation_tables,
    View::Vector<SymbolReference> sorted_symbols,
    View::Vector<Unsigned_32> symbol_slots,
    View::Bytes symbol_table,
    View::Bytes string_table,
    Count symbol_table_index,
    Count string_table_index) -> Dynamic::Vector<SectionDescriptor> {
  Dynamic::Vector<SectionDescriptor> descriptors;
  const auto* section_data = sections.get_data();
  const auto* relocation_table_data = relocation_tables.get_data();
  const auto* sorted_symbol_data = sorted_symbols.get_data();
  for (Count i = 0; i < sections.get_size(); i++) {
    descriptors.insert(to_section_descriptor(section_data[i]));
  }

  Count relocation_offset = 0;
  for (Count i = 1; i < sections.get_size(); i++) {
    View::Vector<Object::Relocation> relocations = relocation_table_data[i];
    if (relocations.get_size() == 0) {
      continue;
    }

    const Count data_size = sizeof(RelaRecord) * relocations.get_size();
    Access::Bytes data = relocation_data.slice(relocation_offset, data_size);
    write_relocations(data, relocations, symbol_slots);
    relocation_offset += data_size;

    descriptors.insert({
      relocation_name_for(section_data[i].get_type()),
      SectionType::RelocationAddend,
      0,
      8,
      data,
      Unsigned_32(symbol_table_index),
      Unsigned_32(i),
      sizeof(RelaRecord),
    });
  }

  // ELF stores the first global symbol slot in sh_info. Slot zero remains the
  // null record and the explicit sort above places every local Symbol before
  // this boundary.
  Count first_non_local_symbol = 1;
  for (Count i = 0; i < sorted_symbols.get_size(); i++) {
    if (sorted_symbol_data[i].symbol->get_visibility() !=
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

static auto build_object(const Object::Module& module) -> Dynamic::Bytes {
  const auto sections = module.get_sections();
  const auto symbols = module.get_symbols();
  const auto relocations = module.get_relocations();
  auto relocation_tables = group_relocations(sections.get_size(), relocations);
  Count relocation_section_count = 0;
  for (Count i = 0; i < relocation_tables.get_size(); i++) {
    if (relocation_tables[i].get_size() != 0) {
      relocation_section_count++;
    }
  }

  auto sorted_symbols = sort_symbols(symbols);
  auto symbol_slots = build_symbol_slots(symbols);
  auto string_table = build_string_table(sorted_symbols.get_access());
  auto symbol_table = build_symbol_table(sorted_symbols.get_view());

  Dynamic::Bytes relocation_data;
  relocation_data.forgetful_resize(sizeof(RelaRecord) * relocations.get_size());

  // Relocation sections are inserted after Module sections. The three shared
  // tables follow them, so every index can be frozen before any file offset is
  // assigned.
  const Count symbol_table_index =
      sections.get_size() + relocation_section_count;
  const Count string_table_index = symbol_table_index + 1;
  const Count section_string_table_index = string_table_index + 1;
  const Count total = section_string_table_index + 1;

  auto section_descriptors = build_section_descriptors(
      sections, relocation_data.get_access(), relocation_tables.get_view(),
      sorted_symbols.get_view(), symbol_slots.get_view(), symbol_table,
      string_table, symbol_table_index, string_table_index);

  // Section names depend on the complete descriptor inventory. Build that
  // string table after descriptors exist and retain its bytes for the rest of
  // this transaction.
  auto section_string_table =
      build_section_string_table(section_descriptors.get_access());
  section_descriptors.get_access().get_data()[section_string_table_index].data =
      section_string_table.get_view();

  // Payload offsets are assigned only after every descriptor exists. The
  // section header block can then follow the last aligned payload without
  // requiring retained target state.
  const Count section_headers_offset =
      assign_offsets(section_descriptors.get_access());
  const Count file_size =
      section_headers_offset + sizeof(SectionHeader) * total;

  Dynamic::Bytes output(file_size);
  output.forgetful_resize(file_size);
  output.set(0);
  auto output_bytes = output.get_access().get_data();
  memset(output_bytes, 0, file_size);

  write_header(
      output.get_access(), section_headers_offset, Unsigned_16(total),
      Unsigned_16(section_string_table_index));

  write_section_headers(
      output.get_access().slice(
          section_headers_offset, sizeof(SectionHeader) * total),
      section_descriptors.get_view());

  const auto* descriptor_data = section_descriptors.get_view().get_data();
  for (Count i = 0; i < total; i++) {
    const auto& descriptor = descriptor_data[i];
    if (descriptor.data.get_size() > 0) {
      Data::copy(
          output.get_access().get_data() + descriptor.file_offset,
          descriptor.data.get_data(), descriptor.data.get_size());
    }
  }

  return output;
}

auto Target::Elf::build_library(
    const Object::Module& module,
    View::Bytes object_name) const -> Dynamic::Bytes {
  const auto symbols = module.get_symbols();
  Dynamic::Bytes object = build_object(module);
  const View::Bytes object_view = object.get_view();

  // The archive index publishes only defined global Symbols. Undefined names
  // remain in the ELF member so a later native consumer can resolve them.
  Count exported_count = 0;
  Count exported_names_bytes = 0;
  for (Count i = 0; i < symbols.get_size(); i++) {
    if (symbols.get_data()[i].get_visibility() ==
            Object::Symbol::Visibility::Global &&
        symbols.get_data()[i].is_defined()) {
      exported_count++;
      exported_names_bytes += symbols.get_data()[i].get_name().get_size() + 1;
    }
  }

  // The System V index begins with a big endian count and member offsets, then
  // stores each exported name with a trailing zero byte.
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
    const auto& symbol = symbols.get_data()[i];
    if (symbol.get_visibility() != Object::Symbol::Visibility::Global ||
        symbol.is_undefined()) {
      continue;
    }

    const auto name = symbol.get_name();
    Data::copy(write_pointer, name.get_data(), name.get_size());
    write_pointer += name.get_size();
    *write_pointer++ = '\0';
  }

  cursor += symbol_table_padded;

  // The retained smoke contract carries one ELF member. K01 owns multiple
  // members and full archive compliance, so this transaction stays deliberately
  // narrow.
  ArHeader object_header;
  fill_ar_header(
      object_header, object_name, Unsigned_64(object_view.get_size()));
  Data::copy(output_bytes + cursor, &object_header, 1);
  cursor += sizeof(ArHeader);
  Data::copy(
      output_bytes + cursor, object_view.get_data(), object_view.get_size());
  return output;
}
