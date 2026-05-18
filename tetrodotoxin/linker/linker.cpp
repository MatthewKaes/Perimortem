// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/linker/linker.hpp"

#include "perimortem/core/data.hpp"

#include "perimortem/utility/null_terminated.hpp"

#include "tetrodotoxin/compiler/compiler.hpp"
#include "tetrodotoxin/compiler/context/relocation.hpp"
#include "tetrodotoxin/compiler/context/symbol.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Linker;
using namespace Tetrodotoxin::Linker::Target;
using namespace Tetrodotoxin::Compiler;

template <typename bit_type>
static auto write_le(void* dest, bit_type value) -> void {
  value =
      Data::ensure_endian<Data::ByteOrder::Native, Data::ByteOrder::Little>(
          value);
  Data::copy(reinterpret_cast<Byte*>(dest), &value, 1);
}

template <typename bit_type>
static auto write_be(void* dest, bit_type value) -> void {
  value =
      Data::ensure_endian<Data::ByteOrder::Native, Data::ByteOrder::Big>(value);
  Data::copy(reinterpret_cast<Byte*>(dest), &value, 1);
}

static auto write_decimal(Byte* field, Count field_size, Bits_64 val) -> void {
  Byte tmp[20];
  Count len = 0;
  if (val == 0) {
    tmp[len++] = '0';
  } else {
    while (val > 0) {
      tmp[len++] = Byte('0' + val % 10);
      val /= 10;
    }
  }
  Data::set(field, ' ', field_size);
  for (Count i = 0; i < len && i < field_size; i++) {
    field[i] = tmp[len - 1 - i];
  }
}

static auto is_local(const InputSymbol& sym) -> Bool {
  return sym.visability == SymbolVisability::Local;
}

// ar(1) member header — 60 bytes, all ASCII fields, space-padded.
// See ar(1): https://man7.org/linux/man-pages/man1/ar.1.html
struct ArMemberHeader {
  Byte name[16];
  Byte timestamp[12];
  Byte owner_id[6];
  Byte group_id[6];
  Byte file_mode[8];
  Byte data_size[10];
  Byte terminator[2];  // must be "`\n"
};
static_assert(sizeof(ArMemberHeader) == 60);

static auto fill_ar_header(
    ArMemberHeader& header,
    View::Bytes name,
    Bits_64 size) -> void {
  // SYSV format: "name/" left-justified, space-padded to 16 (max 15-char name).
  Data::set(header.name, ' ', 16);
  const Count name_len = name.get_size() < 15 ? name.get_size() : 15;
  memcpy(header.name, name.get_data(), name_len);
  header.name[name_len] = '/';

  write_decimal(header.timestamp, 12, 0);
  write_decimal(header.owner_id, 6, 0);
  write_decimal(header.group_id, 6, 0);

  Data::set(header.file_mode, ' ', 8);
  header.file_mode[0] = '6';
  header.file_mode[1] = '4';
  header.file_mode[2] = '4';

  write_decimal(header.data_size, 10, size);
  header.terminator[0] = '`';
  header.terminator[1] = '\n';
}

Linker::Linker(Allocator::Arena& arena)
    : arena(arena), sections(arena), symbols(arena), relocations(arena) {}

auto Linker::add_section(InputSection section) -> Bits_16 {
  const Bits_16 index = Bits_16(sections.get_size());
  sections.insert(section);
  return index;
}

auto Linker::add_symbol(InputSymbol symbol) -> Bits_32 {
  const Bits_32 index = Bits_32(symbols.get_size());
  symbols.insert(symbol);
  return index;
}

auto Linker::add_relocation(InputRelocation relocation) -> void {
  relocations.insert(relocation);
}

auto Linker::reset() -> void {
  sections.clear();
  symbols.clear();
  relocations.clear();
}

auto Linker::emit_object(Dynamic::Bytes& output) -> void {
  const Count section_count    = sections.get_size();
  const Count symbol_count     = symbols.get_size();
  const Count relocation_count = relocations.get_size();

  // Arena-backed scratch arrays, freed when the caller resets the arena.
  auto* section_name_offsets =
      reinterpret_cast<Bits_32*>(arena.allocate(sizeof(Bits_32) * section_count));
  auto* relocation_name_offsets =
      reinterpret_cast<Bits_32*>(arena.allocate(sizeof(Bits_32) * section_count));
  auto* symbol_name_offsets =
      reinterpret_cast<Bits_32*>(arena.allocate(sizeof(Bits_32) * symbol_count));
  auto* symbol_indices =
      reinterpret_cast<Bits_32*>(arena.allocate(sizeof(Bits_32) * symbol_count));
  auto* has_relocations = reinterpret_cast<Byte*>(arena.allocate(section_count));
  auto* relocation_counts =
      reinterpret_cast<Bits_32*>(arena.allocate(sizeof(Bits_32) * section_count));
  auto* section_file_offsets =
      reinterpret_cast<Bits_64*>(arena.allocate(sizeof(Bits_64) * section_count));
  auto* relocation_file_offsets =
      reinterpret_cast<Bits_64*>(arena.allocate(sizeof(Bits_64) * section_count));
  auto* relocation_header_indices =
      reinterpret_cast<Bits_16*>(arena.allocate(sizeof(Bits_16) * section_count));

  memset(has_relocations, 0, section_count);
  memset(relocation_counts, 0, sizeof(Bits_32) * section_count);
  memset(relocation_name_offsets, 0, sizeof(Bits_32) * section_count);
  memset(relocation_file_offsets, 0, sizeof(Bits_64) * section_count);
  memset(relocation_header_indices, 0, sizeof(Bits_16) * section_count);

  for (Count i = 0; i < relocation_count; i++) {
    const Bits_16 section_index = relocations.at(i).section_index;
    has_relocations[section_index] = 1;
    relocation_counts[section_index]++;
  }

  // Build .strtab (symbol name strings).
  Dynamic::Bytes strtab;
  strtab.append('\0');
  for (Count i = 0; i < symbol_count; i++) {
    symbol_name_offsets[i] = Bits_32(strtab.get_size());
    strtab.concat(symbols.at(i).name);
    strtab.append('\0');
  }

  // Build .shstrtab (section name strings).
  Dynamic::Bytes shstrtab;
  shstrtab.append('\0');

  for (Count i = 0; i < section_count; i++) {
    const auto props = Elf::section_properties(sections.at(i).type);
    section_name_offsets[i] = Bits_32(shstrtab.get_size());
    shstrtab.concat(props.name);
    shstrtab.append('\0');
  }

  for (Count i = 0; i < section_count; i++) {
    if (!has_relocations[i]) {
      continue;
    }
    const auto props = Elf::section_properties(sections.at(i).type);
    relocation_name_offsets[i] = Bits_32(shstrtab.get_size());
    shstrtab.concat(".rela"_view);
    shstrtab.concat(props.name);
    shstrtab.append('\0');
  }

  const Bits_32 symtab_name_offset = Bits_32(shstrtab.get_size());
  shstrtab.concat(".symtab"_view);
  shstrtab.append('\0');

  const Bits_32 strtab_name_offset = Bits_32(shstrtab.get_size());
  shstrtab.concat(".strtab"_view);
  shstrtab.append('\0');

  const Bits_32 shstrtab_name_offset = Bits_32(shstrtab.get_size());
  shstrtab.concat(".shstrtab"_view);
  shstrtab.append('\0');

  // Sort symbols: locals first, then globals (ELF spec requirement).
  Count local_symbol_count = 0;
  for (Count i = 0; i < symbol_count; i++) {
    if (is_local(symbols.at(i))) {
      local_symbol_count++;
    }
  }

  Count next_local_index  = 1;  // symtab index 0 = null symbol
  Count next_global_index = 1 + local_symbol_count;
  for (Count i = 0; i < symbol_count; i++) {
    symbol_indices[i] = is_local(symbols.at(i)) ? Bits_32(next_local_index++)
                                                 : Bits_32(next_global_index++);
  }

  // Build .symtab data.
  const Count symtab_entry_count = 1 + symbol_count;
  Dynamic::Bytes symtab_data;
  symtab_data.forgetful_resize(sizeof(Elf::Symbol) * symtab_entry_count);
  memset(symtab_data.get_access().get_data(), 0,
      sizeof(Elf::Symbol) * symtab_entry_count);

  auto* symtab =
      reinterpret_cast<Elf::Symbol*>(symtab_data.get_access().get_data());

  for (Count i = 0; i < symbol_count; i++) {
    const auto& symbol = symbols.at(i);
    auto& entry = symtab[symbol_indices[i]];
    entry.name_offset  = symbol_name_offsets[i];
    entry.info         = Elf::make_symbol_info(Elf::SymbolBinding(Byte(symbol.visability)), symbol.type);
    entry.visibility   = 0;  // STV_DEFAULT
    // Translate user section_index to ELF st_shndx.
    // Sentinel values >= 0xFF00 map to their ELF equivalents (UndefinedSection
    // maps to 0; AbsoluteSection and CommonSection pass through).
    if (symbol.section_index >= 0xFF00) {
      entry.section_index =
          (symbol.section_index == Elf::UndefinedSection) ? 0 : symbol.section_index;
    } else {
      entry.section_index = Bits_16(symbol.section_index + 1);
    }
    entry.value = symbol.value;
    entry.size  = symbol.size;
  }

  // Assign ELF section header indices:
  //   [0]            null
  //   [1..n_sec]     user sections
  //   [n_sec+1..]    .rela.xxx (one per section that has relocations)
  //   [symtab_idx]   .symtab
  //   [strtab_idx]   .strtab
  //   [shstrtab_idx] .shstrtab

  Count relocation_section_count = 0;
  Bits_16 relocation_header_index = Bits_16(1 + section_count);
  for (Count i = 0; i < section_count; i++) {
    if (!has_relocations[i]) {
      continue;
    }
    relocation_header_indices[i] = relocation_header_index++;
    relocation_section_count++;
  }

  const Bits_16 symtab_header_index  = Bits_16(1 + section_count + relocation_section_count);
  const Bits_16 strtab_header_index  = Bits_16(symtab_header_index + 1);
  const Bits_16 shstrtab_header_index = Bits_16(strtab_header_index + 1);
  const Bits_16 total_section_headers = Bits_16(shstrtab_header_index + 1);

  // Compute file offsets.
  Count file_offset = sizeof(Elf::Header);

  for (Count i = 0; i < section_count; i++) {
    const auto& section = sections.at(i);
    if (section.data.get_size() == 0) {
      section_file_offsets[i] = file_offset;
      continue;
    }
    const auto props = Elf::section_properties(section.type);
    switch (props.alignment) {
    case 16: file_offset = Data::align<16>(file_offset); break;
    case 8:  file_offset = Data::align<8>(file_offset); break;
    default: break;
    }
    section_file_offsets[i] = file_offset;
    file_offset += section.data.get_size();
  }

  for (Count i = 0; i < section_count; i++) {
    if (!has_relocations[i]) {
      continue;
    }
    file_offset = Data::align<8>(file_offset);
    relocation_file_offsets[i] = file_offset;
    file_offset += sizeof(Elf::Rela) * relocation_counts[i];
  }

  file_offset = Data::align<8>(file_offset);
  const Count symtab_offset = file_offset;
  file_offset += sizeof(Elf::Symbol) * symtab_entry_count;

  const Count strtab_offset = file_offset;
  file_offset += strtab.get_size();

  const Count shstrtab_offset = file_offset;
  file_offset += shstrtab.get_size();

  file_offset = Data::align<8>(file_offset);
  const Count section_header_offset = file_offset;
  const Count total_size = section_header_offset + sizeof(Elf::SectionHeader) * total_section_headers;

  // Allocate and zero output.
  output.forgetful_resize(total_size);
  Byte* out = output.get_access().get_data();
  memset(out, 0, total_size);

  // ELF file header.
  static constexpr Byte elf_identity[16] = {
    0x7F, 'E', 'L', 'F',  // magic
    2,                     // ELFCLASS64
    1,                     // ELFDATA2LSB
    1,                     // EV_CURRENT
    0,                     // ELFOSABI_NONE
    0, 0, 0, 0, 0, 0, 0, 0,
  };
  auto& file_header = *reinterpret_cast<Elf::Header*>(out);
  memcpy(file_header.identity, elf_identity, sizeof(elf_identity));
  write_le(&file_header.type,                   Bits_16(Elf::ObjectType::Relocatable));
  write_le(&file_header.machine,                Bits_16(Elf::Machine::X86_64));
  write_le(&file_header.version,                Bits_32(1));
  write_le(&file_header.section_header_offset,  Bits_64(section_header_offset));
  write_le(&file_header.header_size,            Bits_16(sizeof(Elf::Header)));
  write_le(&file_header.section_entry_size,     Bits_16(sizeof(Elf::SectionHeader)));
  write_le(&file_header.section_count,          Bits_16(total_section_headers));
  write_le(&file_header.string_section_index,   Bits_16(shstrtab_header_index));

  // Section data.
  for (Count i = 0; i < section_count; i++) {
    const auto& section = sections.at(i);
    if (section.data.get_size() == 0) {
      continue;
    }
    memcpy(out + section_file_offsets[i], section.data.get_data(), section.data.get_size());
  }

  // .rela.xxx sections.
  for (Count i = 0; i < section_count; i++) {
    if (!has_relocations[i]) {
      continue;
    }
    auto* rela_out = reinterpret_cast<Elf::Rela*>(out + relocation_file_offsets[i]);
    Count slot = 0;
    for (Count j = 0; j < relocation_count; j++) {
      const auto& reloc = relocations.at(j);
      if (reloc.section_index != Bits_16(i)) {
        continue;
      }
      write_le(&rela_out[slot].offset,  reloc.offset);
      write_le(&rela_out[slot].info,    Elf::make_relocation_info(symbol_indices[reloc.symbol_index], reloc.type));
      write_le(&rela_out[slot].addend,  reloc.addend);
      slot++;
    }
  }

  // .symtab, .strtab, .shstrtab.
  memcpy(out + symtab_offset,   symtab_data.get_view().get_data(), symtab_data.get_size());
  memcpy(out + strtab_offset,   strtab.get_view().get_data(),      strtab.get_size());
  memcpy(out + shstrtab_offset, shstrtab.get_view().get_data(),    shstrtab.get_size());

  // Section header table.
  auto* section_headers = reinterpret_cast<Elf::SectionHeader*>(out + section_header_offset);

  // [0] null — already zeroed.

  for (Count i = 0; i < section_count; i++) {
    const auto& section = sections.at(i);
    const auto props = Elf::section_properties(section.type);
    auto& header = section_headers[1 + i];
    write_le(&header.name_offset, Bits_32(section_name_offsets[i]));
    write_le(&header.type,        Bits_32(props.type));
    write_le(&header.flags,       Bits_64(props.flags));
    write_le(&header.file_offset, Bits_64(section_file_offsets[i]));
    write_le(&header.size,
        Bits_64(section.data.get_size() > 0 ? section.data.get_size() : section.size));
    write_le(&header.alignment,   Bits_64(props.alignment > 0 ? props.alignment : 1));
  }

  for (Count i = 0; i < section_count; i++) {
    if (!has_relocations[i]) {
      continue;
    }
    auto& header = section_headers[relocation_header_indices[i]];
    write_le(&header.name_offset, Bits_32(relocation_name_offsets[i]));
    write_le(&header.type,        Bits_32(Elf::SectionType::RelocationAddend));
    write_le(&header.file_offset, Bits_64(relocation_file_offsets[i]));
    write_le(&header.size,        Bits_64(sizeof(Elf::Rela) * relocation_counts[i]));
    write_le(&header.link,        Bits_32(symtab_header_index));
    write_le(&header.info,        Bits_32(1 + i));
    write_le(&header.alignment,   Bits_64(8));
    write_le(&header.entry_size,  Bits_64(sizeof(Elf::Rela)));
  }

  auto& symtab_header = section_headers[symtab_header_index];
  write_le(&symtab_header.name_offset, symtab_name_offset);
  write_le(&symtab_header.type,        Bits_32(Elf::SectionType::SymbolTable));
  write_le(&symtab_header.file_offset, Bits_64(symtab_offset));
  write_le(&symtab_header.size,        Bits_64(sizeof(Elf::Symbol) * symtab_entry_count));
  write_le(&symtab_header.link,        Bits_32(strtab_header_index));
  write_le(&symtab_header.info,        Bits_32(1 + local_symbol_count));
  write_le(&symtab_header.alignment,   Bits_64(8));
  write_le(&symtab_header.entry_size,  Bits_64(sizeof(Elf::Symbol)));

  auto& strtab_header = section_headers[strtab_header_index];
  write_le(&strtab_header.name_offset, strtab_name_offset);
  write_le(&strtab_header.type,        Bits_32(Elf::SectionType::StringTable));
  write_le(&strtab_header.file_offset, Bits_64(strtab_offset));
  write_le(&strtab_header.size,        Bits_64(strtab.get_size()));
  write_le(&strtab_header.alignment,   Bits_64(1));

  auto& shstrtab_header = section_headers[shstrtab_header_index];
  write_le(&shstrtab_header.name_offset, shstrtab_name_offset);
  write_le(&shstrtab_header.type,        Bits_32(Elf::SectionType::StringTable));
  write_le(&shstrtab_header.file_offset, Bits_64(shstrtab_offset));
  write_le(&shstrtab_header.size,        Bits_64(shstrtab.get_size()));
  write_le(&shstrtab_header.alignment,   Bits_64(1));
}

static auto write_ar_symtab_member(
    Byte* out,
    Count cursor,
    Count exported_count,
    Count object_member_offset,
    const Perimortem::Memory::Managed::Vector<InputSymbol>& symbols) -> void {
  Byte* pos = out + cursor;

  write_be(pos, Bits_32(exported_count));
  pos += 4;

  for (Count i = 0; i < exported_count; i++) {
    write_be(pos, Bits_32(object_member_offset));
    pos += 4;
  }

  const Count symbol_count = symbols.get_size();
  for (Count i = 0; i < symbol_count; i++) {
    if (is_local(symbols.at(i))) {
      continue;
    }
    const auto name = symbols.at(i).name;
    memcpy(pos, name.get_data(), name.get_size());
    pos += name.get_size();
    *pos++ = '\0';
  }
}

auto Linker::emit_archive(View::Bytes object_name) -> Dynamic::Bytes {
  Dynamic::Bytes object;
  emit_object(object);
  const View::Bytes object_view = object.get_view();

  const Count symbol_count = symbols.get_size();
  Count exported_count = 0;
  Count exported_names_bytes = 0;
  for (Count i = 0; i < symbol_count; i++) {
    if (!is_local(symbols.at(i))) {
      exported_count++;
      exported_names_bytes += symbols.at(i).name.get_size() + 1;
    }
  }

  // ar(1) SYSV symbol table layout:
  //   4 bytes BE : symbol count
  //   4 * n bytes BE : member offsets (all point to the single .o)
  //   null-terminated names
  const Count symtab_data_size   = 4 + 4 * exported_count + exported_names_bytes;
  const Count symtab_data_padded = symtab_data_size + (symtab_data_size & 1);

  const Count object_member_offset = 8 + 60 + symtab_data_padded;

  const Count object_padded = object_view.get_size() + (object_view.get_size() & 1);
  const Count total = 8 + 60 + symtab_data_padded + 60 + object_padded;

  Dynamic::Bytes output;
  output.forgetful_resize(total);
  Byte* out = output.get_access().get_data();
  memset(out, 0, total);

  static constexpr Byte ar_magic[] = {'!','<','a','r','c','h','>','\n'};
  memcpy(out, ar_magic, sizeof(ar_magic));
  Count cursor = 8;

  // Symbol table member header — SYSV format name "/" indicates the symbol table.
  auto& symtab_header = *reinterpret_cast<ArMemberHeader*>(out + cursor);
  Data::set(symtab_header.name, ' ', 16);
  symtab_header.name[0] = '/';
  write_decimal(symtab_header.timestamp, 12, 0);
  write_decimal(symtab_header.owner_id, 6, 0);
  write_decimal(symtab_header.group_id, 6, 0);
  Data::set(symtab_header.file_mode, ' ', 8);
  write_decimal(symtab_header.data_size, 10, Bits_64(symtab_data_size));
  symtab_header.terminator[0] = '`';
  symtab_header.terminator[1] = '\n';
  cursor += 60;

  write_ar_symtab_member(
      out, cursor, exported_count, object_member_offset, symbols);
  cursor += symtab_data_padded;

  auto& object_header = *reinterpret_cast<ArMemberHeader*>(out + cursor);
  fill_ar_header(object_header, object_name, Bits_64(object_view.get_size()));
  cursor += 60;

  memcpy(out + cursor, object_view.get_data(), object_view.get_size());

  return output;
}

static auto to_elf_type(Context::Symbol::Type type) -> Elf::SymbolType {
  switch (type) {
  case Context::Symbol::Type::Func:   return Elf::SymbolType::Function;
  case Context::Symbol::Type::Object: return Elf::SymbolType::Object;
  case Context::Symbol::Type::None:   return Elf::SymbolType::None;
  }
  return Elf::SymbolType::None;
}

static auto to_elf_relocation_type(
    Context::Relocation::Type type) -> Elf::RelocationType {
  return type == Context::Relocation::Type::Plt32
      ? Elf::RelocationType::Plt32
      : Elf::RelocationType::PcRelative32;
}

auto Tetrodotoxin::Linker::emit_archive(
    Compiler::Compiler& compiler,
    View::Bytes object_name) -> Dynamic::Bytes {
  Linker linker(compiler.get_arena());

  const Bits_16 text_section = linker.add_section({
    SectionTypes::Program,
    compiler.get_machine_code(),
    0,
  });

  Bits_16 rodata_section = 0;
  if (compiler.get_string_data().get_size() > 0) {
    rodata_section = linker.add_section({
      SectionTypes::Strings,
      compiler.get_string_data(),
      0,
    });
  }

  // Symbols must be added in compiler order so that symbol index i in the
  // compiler maps to symbol index i in the linker, keeping relocation
  // symbol_index fields correct.
  for (Count i = 0; i < compiler.get_symbol_count(); i++) {
    const auto& sym = compiler.get_symbol(i);

    Bits_16 section_index = Elf::UndefinedSection;
    switch (sym.get_context()) {
    case Context::Symbol::TargetContext::Program:
      section_index = text_section;
      break;
    case Context::Symbol::TargetContext::Strings:
      section_index = rodata_section;
      break;
    case Context::Symbol::TargetContext::External:
      section_index = Elf::UndefinedSection;
      break;
    }

    const SymbolVisability binding =
        sym.get_visability() == Context::Symbol::Visability::Global
        ? SymbolVisability::Global
        : SymbolVisability::Local;

    linker.add_symbol({
      sym.get_name(),
      section_index,
      sym.get_range().start,
      sym.get_range().size,
      binding,
      to_elf_type(sym.get_type()),
    });
  }

  for (Count i = 0; i < compiler.get_relocation_count(); i++) {
    const auto& reloc = compiler.get_relocation(i);
    linker.add_relocation({
      text_section,
      reloc.offset,
      Bits_32(reloc.symbol),
      to_elf_relocation_type(reloc.type),
      reloc.addend,
    });
  }

  return linker.emit_archive(object_name);
}
