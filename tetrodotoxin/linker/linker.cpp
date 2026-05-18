// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/linker/linker.hpp"

#include "perimortem/core/data.hpp"

#include "perimortem/utility/null_terminated.hpp"

#include "tetrodotoxin/compiler/compiler.hpp"
#include "tetrodotoxin/compiler/context/relocation.hpp"
#include "tetrodotoxin/compiler/context/symbol.hpp"
#include "tetrodotoxin/linker/target/elf.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Linker;
using namespace Tetrodotoxin::Linker::Target;
using namespace Tetrodotoxin::Compiler;

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
  return sym.visability == Visability::Local;
}

static auto append_to_strtab(Dynamic::Bytes& table, View::Bytes name)
    -> Bits_32 {
  const Bits_32 offset = Bits_32(table.get_size());
  table.concat(name);
  table.append('\0');
  return offset;
}

// ---------------------------------------------------------------------------
// ar(1) helpers
// ---------------------------------------------------------------------------

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

static auto write_ar_symtab_member(
    Byte* out,
    Count cursor,
    Count exported_count,
    Count object_member_offset,
    const Managed::Vector<InputSymbol>& symbols) -> void {
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

// ---------------------------------------------------------------------------
// Compiler → ELF bridge
// ---------------------------------------------------------------------------

static auto to_elf_type(Context::Symbol::Type type) -> Elf::SymbolType {
  switch (type) {
  case Context::Symbol::Type::Func:
    return Elf::SymbolType::Function;
  case Context::Symbol::Type::Object:
    return Elf::SymbolType::Object;
  case Context::Symbol::Type::None:
    return Elf::SymbolType::None;
  }
  return Elf::SymbolType::None;
}

static auto to_elf_relocation_type(Context::Relocation::Type type)
    -> Elf::RelocationType {
  return type == Context::Relocation::Type::Plt32
             ? Elf::RelocationType::Plt32
             : Elf::RelocationType::PcRelative32;
}

// Byte content and shstrtab name offsets for the three string-like sections.
struct StringTables {
  Dynamic::Bytes strtab;
  Dynamic::Bytes shstrtab;
  Bits_32 symtab_name_offset;
  Bits_32 strtab_name_offset;
  Bits_32 shstrtab_name_offset;
};

// Serialised .symtab bytes and the ELF-required local-symbol boundary.
struct SymbolTableData {
  Dynamic::Bytes bytes;
  Count local_symbol_count;
};

static auto build_symbol_table(
    const Managed::Vector<InputSymbol>& symbols,
    ScratchArrays& scratch) -> SymbolTableData {
  SymbolTableData result = {};

  for (Count i = 0; i < scratch.symbol_count; i++) {
    if (is_local(symbols.at(i))) {
      result.local_symbol_count++;
    }
  }

  // Sort locals before globals (ELF spec requirement). Index 0 is the null
  // symbol.
  Count next_local = 1;
  Count next_global = 1 + result.local_symbol_count;
  for (Count i = 0; i < scratch.symbol_count; i++) {
    scratch.symbol_indices[i] = is_local(symbols.at(i))
                                    ? Bits_32(next_local++)
                                    : Bits_32(next_global++);
  }

  const Count entry_count = 1 + scratch.symbol_count;
  result.bytes.forgetful_resize(sizeof(Elf::Symbol) * entry_count);
  memset(
      result.bytes.get_access().get_data(), 0,
      sizeof(Elf::Symbol) * entry_count);

  auto* symtab =
      reinterpret_cast<Elf::Symbol*>(result.bytes.get_access().get_data());

  for (Count i = 0; i < scratch.symbol_count; i++) {
    const auto& symbol = symbols.at(i);
    auto& entry = symtab[scratch.symbol_indices[i]];
    entry.name_offset = scratch.symbol_name_offsets[i];
    entry.info = Elf::make_symbol_info(symbol.visability, symbol.type);
    entry.visibility = 0;  // STV_DEFAULT
    // Translate user section_index to ELF st_shndx.
    // Sentinel values >= 0xFF00 map to their ELF equivalents (UndefinedSection
    // maps to 0; AbsoluteSection and CommonSection pass through).
    if (symbol.section_index >= 0xFF00) {
      entry.section_index = (symbol.section_index == Elf::UndefinedSection)
                                ? 0
                                : symbol.section_index;
    } else {
      entry.section_index = Bits_16(symbol.section_index + 1);
    }
    entry.value = symbol.value;
    entry.size = symbol.size;
  }

  return result;
}

static auto compute_layout(
    const Managed::Vector<InputSection>& sections,
    ScratchArrays& scratch,
    const StringTables& strings) -> FileLayout {
  FileLayout layout = {};

  // Assign ELF section header indices:
  //   [0]            null
  //   [1..n]         user sections
  //   [n+1..xxx]     .rela.xxx (one per section with relocations)
  //   [symtab_idx]   .symtab
  //   [strtab_idx]   .strtab
  //   [shstrtab_idx] .shstrtab
  Bits_16 next_rela = Bits_16(1 + scratch.section_count);
  for (Count i = 0; i < scratch.section_count; i++) {
    if (!scratch.has_relocations[i]) {
      continue;
    }
    scratch.relocation_header_indices[i] = next_rela++;
    layout.relocation_section_count++;
  }

  layout.symtab_header_index =
      Bits_16(1 + scratch.section_count + layout.relocation_section_count);
  layout.strtab_header_index = Bits_16(layout.symtab_header_index + 1);
  layout.shstrtab_header_index = Bits_16(layout.strtab_header_index + 1);
  layout.total_section_headers = Bits_16(layout.shstrtab_header_index + 1);

  Count offset = sizeof(Elf::Header);

  for (Count i = 0; i < scratch.section_count; i++) {
    const auto& section = sections.at(i);
    if (section.data.get_size() == 0) {
      scratch.section_file_offsets[i] = offset;
      continue;
    }
    const auto props = Elf::section_properties(section.type);
    switch (props.alignment) {
    case 16:
      offset = Data::align<16>(offset);
      break;
    case 8:
      offset = Data::align<8>(offset);
      break;
    default:
      break;
    }
    scratch.section_file_offsets[i] = offset;
    offset += section.data.get_size();
  }

  for (Count i = 0; i < scratch.section_count; i++) {
    if (!scratch.has_relocations[i]) {
      continue;
    }
    offset = Data::align<8>(offset);
    scratch.relocation_file_offsets[i] = offset;
    offset += sizeof(Elf::Rela) * scratch.relocation_counts[i];
  }

  offset = Data::align<8>(offset);
  layout.symtab_file_offset = offset;
  offset += sizeof(Elf::Symbol) * (1 + scratch.symbol_count);

  layout.strtab_file_offset = offset;
  offset += strings.strtab.get_size();

  layout.shstrtab_file_offset = offset;
  offset += strings.shstrtab.get_size();

  offset = Data::align<8>(offset);
  layout.section_header_offset = offset;
  layout.total_file_size =
      layout.section_header_offset +
      sizeof(Elf::SectionHeader) * layout.total_section_headers;

  return layout;
}

static auto write_section_data(
    Byte* out,
    const Managed::Vector<InputSection>& sections,
    const ScratchArrays& scratch) -> void {
  for (Count i = 0; i < scratch.section_count; i++) {
    const auto& section = sections.at(i);
    if (section.data.get_size() == 0) {
      continue;
    }
    memcpy(
        out + scratch.section_file_offsets[i], section.data.get_data(),
        section.data.get_size());
  }
}

static auto write_rela_sections(
    Byte* out,
    const Managed::Vector<InputRelocation>& relocations,
    const ScratchArrays& scratch) -> void {
  for (Count i = 0; i < scratch.section_count; i++) {
    if (!scratch.has_relocations[i]) {
      continue;
    }
    auto* rela =
        reinterpret_cast<Elf::Rela*>(out + scratch.relocation_file_offsets[i]);
    Count slot = 0;
    for (Count j = 0; j < scratch.relocation_count; j++) {
      const auto& reloc = relocations.at(j);
      if (reloc.section_index != Bits_16(i)) {
        continue;
      }
      write_le(&rela[slot].offset, reloc.offset);
      write_le(
          &rela[slot].info,
          Elf::make_relocation_info(
              scratch.symbol_indices[reloc.symbol_index], reloc.type));
      write_le(&rela[slot].addend, reloc.addend);
      slot++;
    }
  }
}

static auto write_section_headers(
    Byte* out,
    const Managed::Vector<InputSection>& sections,
    const ScratchArrays& scratch,
    const StringTables& strings,
    const SymbolTableData& symtab,
    const FileLayout& layout) -> void {
  auto* headers =
      reinterpret_cast<Elf::SectionHeader*>(out + layout.section_header_offset);

  // [0] null — already zeroed.

  for (Count i = 0; i < scratch.section_count; i++) {
    const auto& section = sections.at(i);
    const auto props = Elf::section_properties(section.type);
    auto& h = headers[1 + i];
    write_le(&h.name_offset, Bits_32(scratch.section_name_offsets[i]));
    write_le(&h.type, Bits_32(props.type));
    write_le(&h.flags, Bits_64(props.flags));
    write_le(&h.file_offset, Bits_64(scratch.section_file_offsets[i]));
    write_le(
        &h.size, Bits_64(
                     section.data.get_size() > 0 ? section.data.get_size()
                                                 : section.size));
    write_le(&h.alignment, Bits_64(props.alignment > 0 ? props.alignment : 1));
  }

  for (Count i = 0; i < scratch.section_count; i++) {
    if (!scratch.has_relocations[i]) {
      continue;
    }
    auto& h = headers[scratch.relocation_header_indices[i]];
    write_le(&h.name_offset, Bits_32(scratch.relocation_name_offsets[i]));
    write_le(&h.type, Bits_32(Elf::SectionType::RelocationAddend));
    write_le(&h.file_offset, Bits_64(scratch.relocation_file_offsets[i]));
    write_le(
        &h.size, Bits_64(sizeof(Elf::Rela) * scratch.relocation_counts[i]));
    write_le(&h.link, Bits_32(layout.symtab_header_index));
    write_le(&h.info, Bits_32(1 + i));
    write_le(&h.alignment, Bits_64(8));
    write_le(&h.entry_size, Bits_64(sizeof(Elf::Rela)));
  }

  auto& symtab_h = headers[layout.symtab_header_index];
  write_le(&symtab_h.name_offset, strings.symtab_name_offset);
  write_le(&symtab_h.type, Bits_32(Elf::SectionType::SymbolTable));
  write_le(&symtab_h.file_offset, Bits_64(layout.symtab_file_offset));
  write_le(
      &symtab_h.size,
      Bits_64(sizeof(Elf::Symbol) * (1 + scratch.symbol_count)));
  write_le(&symtab_h.link, Bits_32(layout.strtab_header_index));
  write_le(&symtab_h.info, Bits_32(1 + symtab.local_symbol_count));
  write_le(&symtab_h.alignment, Bits_64(8));
  write_le(&symtab_h.entry_size, Bits_64(sizeof(Elf::Symbol)));

  auto& strtab_h = headers[layout.strtab_header_index];
  write_le(&strtab_h.name_offset, strings.strtab_name_offset);
  write_le(&strtab_h.type, Bits_32(Elf::SectionType::StringTable));
  write_le(&strtab_h.file_offset, Bits_64(layout.strtab_file_offset));
  write_le(&strtab_h.size, Bits_64(strings.strtab.get_size()));
  write_le(&strtab_h.alignment, Bits_64(1));

  auto& shstrtab_h = headers[layout.shstrtab_header_index];
  write_le(&shstrtab_h.name_offset, strings.shstrtab_name_offset);
  write_le(&shstrtab_h.type, Bits_32(Elf::SectionType::StringTable));
  write_le(&shstrtab_h.file_offset, Bits_64(layout.shstrtab_file_offset));
  write_le(&shstrtab_h.size, Bits_64(strings.shstrtab.get_size()));
  write_le(&shstrtab_h.alignment, Bits_64(1));
}

// ---------------------------------------------------------------------------
// Linker public methods
// ---------------------------------------------------------------------------

Linker::Linker(OS target) {
  // Force ELF output for now.
  format = new (arena.allocate<Target::Elf>()) Target::Elf();
}

Linker::~Linker() {
  // Deconstruct target
  format->~Format();
}

auto Linker::add_section(
    Context::Symbol::Location section,
    Perimortem::Core::View::Bytes data) -> void {
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

auto Linker::emit_object(Dynamic::Bytes& output) -> void {
  auto scratch = alloc_scratch(arena, sections, symbols, relocations);
  auto strings = build_string_tables(sections, symbols, scratch);
  auto symtab = build_symbol_table(symbols, scratch);
  auto layout = compute_layout(sections, scratch, strings);

  output.forgetful_resize(layout.total_file_size);
  Byte* out = output.get_access().get_data();
  memset(out, 0, layout.total_file_size);

  write_elf_header(out, layout);
  write_section_data(out, sections, scratch);
  write_rela_sections(out, relocations, scratch);
  memcpy(
      out + layout.symtab_file_offset, symtab.bytes.get_view().get_data(),
      symtab.bytes.get_size());
  memcpy(
      out + layout.strtab_file_offset, strings.strtab.get_view().get_data(),
      strings.strtab.get_size());
  memcpy(
      out + layout.shstrtab_file_offset, strings.shstrtab.get_view().get_data(),
      strings.shstrtab.get_size());
  write_section_headers(out, sections, scratch, strings, symtab, layout);
}

auto Linker::emit_archive(View::Bytes object_name) -> Dynamic::Bytes {
  Dynamic::Bytes object;
  emit_object(object);
  const View::Bytes object_view = object.get_view();

  Count exported_count = 0;
  Count exported_names_bytes = 0;
  for (Count i = 0; i < symbols.get_size(); i++) {
    if (!is_local(symbols.at(i))) {
      exported_count++;
      exported_names_bytes += symbols.at(i).name.get_size() + 1;
    }
  }

  // ar(1) SYSV symbol table layout:
  //   4 bytes BE : symbol count
  //   4 * n bytes BE : member offsets (all point to the single .o)
  //   null-terminated names
  const Count symtab_data_size = 4 + 4 * exported_count + exported_names_bytes;
  const Count symtab_data_padded = symtab_data_size + (symtab_data_size & 1);
  const Count object_member_offset = 8 + 60 + symtab_data_padded;
  const Count object_padded =
      object_view.get_size() + (object_view.get_size() & 1);
  const Count total = 8 + 60 + symtab_data_padded + 60 + object_padded;

  Dynamic::Bytes output;
  output.forgetful_resize(total);
  Byte* out = output.get_access().get_data();
  memset(out, 0, total);

  static constexpr Byte ar_magic[] = {'!', '<', 'a', 'r', 'c', 'h', '>', '\n'};
  memcpy(out, ar_magic, sizeof(ar_magic));
  Count cursor = 8;

  // Symbol table member header — SYSV format name "/" indicates the symbol
  // table.
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

auto Linker::reset() -> void {
  sections.clear();
  symbols.clear();
  relocations.clear();
}

// ---------------------------------------------------------------------------
// Free function: populate a Linker from Compiler output and emit an archive
// ---------------------------------------------------------------------------

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
    case Context::Symbol::Location::Program:
      section_index = text_section;
      break;
    case Context::Symbol::Location::Strings:
      section_index = rodata_section;
      break;
    case Context::Symbol::Location::External:
      section_index = Elf::UndefinedSection;
      break;
    }

    const Visability binding =
        sym.get_visability() == Context::Symbol::Visability::Global
            ? Visability::Global
            : Visability::Local;

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
