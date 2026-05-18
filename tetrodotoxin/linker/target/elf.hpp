// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

// ELF-64 constants and on-disk structures for x86-64 relocatable objects.
// All multi-byte fields are little-endian. See elf(5):
// https://man7.org/linux/man-pages/man5/elf.5.html

namespace Tetrodotoxin::Linker::Target {

enum class SectionTypes {
  Program,  // .text       — executable code
  Strings,  // .rodata.str — merged string constants
  ROData,   // .rodata     — read-only data
};

// Symbol visibility for linker input: whether the symbol is exported.
enum class SymbolVisability : Byte {
  Local  = 0,
  Global = 1,
};

class Elf {
 public:
  // ELF object file type (e_type).
  enum class ObjectType : Bits_16 {
    Relocatable = 1,
  };

  // ELF target machine (e_machine).
  enum class Machine : Bits_16 {
    X86_64 = 62,
  };

  // ELF section header type (sh_type).
  enum class SectionType : Bits_32 {
    Null             = 0,
    ProgramData      = 1,
    SymbolTable      = 2,
    StringTable      = 3,
    RelocationAddend = 4,
    NoBits           = 8,
  };

  // ELF section header flags (sh_flags). Multiple flags may be combined with |.
  enum class SectionFlags : Bits_64 {
    Writable   = 0x1,
    Allocated  = 0x2,
    Executable = 0x4,
    Mergeable  = 0x10,
    Strings    = 0x20,
  };

  // ELF symbol binding, encoded in the high nibble of st_info.
  enum class SymbolBinding : Byte {
    Local  = 0,
    Global = 1,
  };

  // ELF symbol type, encoded in the low nibble of st_info.
  enum class SymbolType : Byte {
    None     = 0,
    Object   = 1,
    Function = 2,
    Section  = 3,
    File     = 4,
  };

  // x86-64 relocation types (r_info type field).
  enum class RelocationType : Bits_32 {
    None         = 0,
    Absolute64   = 1,   // S + A
    PcRelative32 = 2,   // S + A - P
    Got32        = 3,
    Plt32        = 4,   // S + A - P (via PLT)
    Absolute32   = 10,  // S + A (zero-extend to 64)
    Absolute32S  = 11,  // S + A (sign-extend to 64)
    PcRelative64 = 24,  // S + A - P
  };

  // ELF64 file header — 64 bytes.
  struct Header {
    Byte    identity[16];
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

  // ELF64 symbol table entry — 24 bytes.
  struct Symbol {
    Bits_32 name_offset;
    Byte    info;        // SymbolBinding (high nibble) | SymbolType (low nibble)
    Byte    visibility;  // STV_DEFAULT = 0
    Bits_16 section_index;
    Bits_64 value;
    Bits_64 size;
  };
  static_assert(sizeof(Symbol) == 24);

  // ELF64 relocation with explicit addend — 24 bytes.
  struct Rela {
    Bits_64       offset;
    Bits_64       info;    // symbol index (high 32) | RelocationType (low 32)
    SignedBits_64 addend;
  };
  static_assert(sizeof(Rela) == 24);

  // Sentinel values for InputSymbol::section_index — values >= 0xFF00 are reserved.
  // UndefinedSection maps to ELF SHN_UNDEF (0) during output; the others pass
  // through directly since they match the ELF SHN_ABS / SHN_COMMON values.
  static constexpr Bits_16 UndefinedSection = 0xFFFF;
  static constexpr Bits_16 AbsoluteSection  = 0xFFF1;
  static constexpr Bits_16 CommonSection    = 0xFFF2;

  static constexpr auto make_symbol_info(SymbolBinding binding, SymbolType type) -> Byte {
    return Byte((Byte(binding) << 4) | (Byte(type) & 0xF));
  }

  static constexpr auto make_relocation_info(Bits_32 symbol, RelocationType type) -> Bits_64 {
    return (Bits_64(symbol) << 32) | Bits_64(type);
  }

  struct SectionProperties {
    Perimortem::Core::View::Bytes name;
    SectionType type;
    SectionFlags flags;
    Bits_64 alignment;
  };

  static auto section_properties(SectionTypes type) -> SectionProperties;
};

constexpr auto operator|(Elf::SectionFlags a, Elf::SectionFlags b) -> Elf::SectionFlags {
  return Elf::SectionFlags(Bits_64(a) | Bits_64(b));
}

// A section to emit into the object file.
// For BSS-like sections with no data, data must be empty and size holds the
// byte count. For all other types size is ignored and data.get_size() is used.
struct InputSection {
  SectionTypes type;
  Perimortem::Core::View::Bytes data;
  Bits_64 size;
};

// A symbol to record in .symtab.
// section_index is a 0-based index into the InputSection list, or one of the
// Elf::*Section sentinel values.
struct InputSymbol {
  Perimortem::Core::View::Bytes name;
  Bits_16 section_index;
  Bits_64 value;
  Bits_64 size;
  SymbolVisability visability;
  Elf::SymbolType type;
};

// A RELA relocation entry referencing a symbol by its 0-based InputSymbol index.
struct InputRelocation {
  Bits_16 section_index;
  Bits_64 offset;
  Bits_32 symbol_index;
  Elf::RelocationType type;
  SignedBits_64 addend;
};

}  // namespace Tetrodotoxin::Linker::Target
