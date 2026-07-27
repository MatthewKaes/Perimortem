// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/assembler/x86_64.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Library;

static constexpr auto reg_code(Assembler::x86_64::Reg reg) -> Unsigned_8 {
  return Unsigned_8(reg) & Unsigned_8(0x7);
}

static constexpr auto xmm_code(Assembler::x86_64::Xmm reg) -> Unsigned_8 {
  return Unsigned_8(reg) & Unsigned_8(0x7);
}

// REX extensions.
// Can be bit or'ed together to add multiple extensions.
enum class RexExt : Unsigned_8 {
  // Expands r/m to 4 bits
  // Also expands short call to 4 bits
  B = 0x41,
  X = 0x42,  // Expands index in SIB
  R = 0x44,  // Expands reg to 4 bits.
  W = 0x48,  // Operand size expanded to 64 bits.
  // Special case for when we need to emit a REX byte with no flags.
  Bare = 0x40,
};

// gen_modrm_byte
// Can be bit or'ed together to add multiple extensions.
enum class AddressMode : Unsigned_8 {
  Memory = 0b00000000,     // Memory with no displacement
  Memory_8 = 0b01000000,   // Memory with 1 byte of displacement (signed)
  Memory_32 = 0b10000000,  // Memory with 4 bytes of displacement (signed)
  RegToReg = 0b11000000,   // Operand size expanded to 64 bits.
};

template <typename bit_type>
static constexpr auto write_const(Dynamic::Bytes& code, bit_type data) -> void {
  // Ensure data is written in little endian.
  data = Data::ensure_endian<Data::ByteOrder::Native, Data::ByteOrder::Little>(
      data);

  code.resize(code.get_size() + sizeof(bit_type));
  Data::copy(
      code.get_access().get_data() + code.get_size() - sizeof(bit_type), &data,
      1);
}

static constexpr auto rex_short(
    Dynamic::Bytes& code,
    Assembler::x86_64::Reg reg) -> void {
  if (reg > Assembler::x86_64::Reg::RDI) {
    code.append(Unsigned_8(RexExt::B));
  }
}

static constexpr auto gen_rex_byte(
    Dynamic::Bytes& code,
    Assembler::x86_64::Reg reg,
    Assembler::x86_64::Reg rm) -> void {
  Unsigned_8 rex_code = 0;

  // REX.W only for 64-bit operand size (upper nibble == 0x1)
  if ((reg != Assembler::x86_64::Reg::None &&
       (Unsigned_8(reg) & Unsigned_8(0xF0)) == Unsigned_8(0x10)) ||
      (rm != Assembler::x86_64::Reg::None &&
       (Unsigned_8(rm) & Unsigned_8(0xF0)) == Unsigned_8(0x10))) {
    rex_code = Unsigned_8(RexExt::W);
  }

  // Register extension to 4 bit address
  if (reg != Assembler::x86_64::Reg::None &&
      (Unsigned_8(reg) & Unsigned_8(0x0F)) > 0x07) {
    rex_code |= Unsigned_8(RexExt::R);
  }

  // Reg / Mem extension to 4 bit address
  if (rm != Assembler::x86_64::Reg::None &&
      (Unsigned_8(rm) & Unsigned_8(0x0F)) > 0x07) {
    rex_code |= Unsigned_8(RexExt::B);
  }

  if (rex_code) {
    code.append(rex_code);
  }
}

// REX for 16-bit register operands (0x66 prefix handles operand size, no
// REX.W).
static constexpr auto gen_rex_byte_16(
    Dynamic::Bytes& code,
    Assembler::x86_64::Reg reg,
    Assembler::x86_64::Reg rm) -> void {
  Unsigned_8 rex_code = 0;
  if (reg != Assembler::x86_64::Reg::None &&
      (Unsigned_8(reg) & Unsigned_8(0x0F)) > 0x07) {
    rex_code |= Unsigned_8(RexExt::R);
  }

  if (rm != Assembler::x86_64::Reg::None &&
      (Unsigned_8(rm) & Unsigned_8(0x0F)) > 0x07) {
    rex_code |= Unsigned_8(RexExt::B);
  }

  if (rex_code) {
    code.append(rex_code);
  }
}

// REX for 32-bit memory operands. The base register may be 64-bit for
// addressing. It must not force REX.W because only the extended register and
// base bits matter for this form.
static constexpr auto gen_rex_byte_32(
    Dynamic::Bytes& code,
    Assembler::x86_64::Reg reg,
    Assembler::x86_64::Reg rm) -> void {
  Unsigned_8 rex_code = 0;
  if (reg != Assembler::x86_64::Reg::None &&
      (Unsigned_8(reg) & Unsigned_8(0x0F)) > 0x07) {
    rex_code |= Unsigned_8(RexExt::R);
  }

  if (rm != Assembler::x86_64::Reg::None &&
      (Unsigned_8(rm) & Unsigned_8(0x0F)) > 0x07) {
    rex_code |= Unsigned_8(RexExt::B);
  }

  if (rex_code) {
    code.append(rex_code);
  }
}

// REX for 8-bit register operands.
// SPL/BPL/SIL/DIL (codes 4-7) require a bare REX prefix to distinguish them
// from the legacy AH/CH/DH/BH registers which share those encoding codes.
static constexpr auto gen_rex_byte_8(
    Dynamic::Bytes& code,
    Assembler::x86_64::Reg reg,
    Assembler::x86_64::Reg rm) -> void {
  Unsigned_8 rex_code = 0;
  if (reg != Assembler::x86_64::Reg::None &&
      (Unsigned_8(reg) & Unsigned_8(0x0F)) > 0x07) {
    rex_code |= Unsigned_8(RexExt::R);
  }

  if (rm != Assembler::x86_64::Reg::None &&
      (Unsigned_8(rm) & Unsigned_8(0x0F)) > 0x07) {
    rex_code |= Unsigned_8(RexExt::B);
  }

  // SPL/BPL/SIL/DIL special case due to encoding aliasing.
  auto anti_alias_rex = [](Assembler::x86_64::Reg r) -> bool {
    auto c = Unsigned_8(r) & Unsigned_8(0x0F);
    return c >= 0x04 && c <= 0x07;
  };
  if (anti_alias_rex(reg) || anti_alias_rex(rm)) {
    rex_code |= Unsigned_8(RexExt::Bare);
  }

  if (rex_code) {
    code.append(rex_code);
  }
}

static constexpr auto gen_modrm_byte(
    AddressMode mode,
    Assembler::x86_64::Reg reg,
    Assembler::x86_64::Reg rm) -> Unsigned_8 {
  return Unsigned_8(mode) | (reg_code(reg) << 3) | reg_code(rm);
}

static constexpr auto gen_modrm_byte(
    Assembler::x86_64::Reg reg,
    Assembler::x86_64::Reg rm) -> Unsigned_8 {
  return Unsigned_8(AddressMode::RegToReg) | (reg_code(reg) << 3) |
         reg_code(rm);
}

// Writes ModRM and optional SIB/displacement bytes for a
// [base + displacement] memory operand.
static auto gen_memory_operand(
    Dynamic::Bytes& code,
    Assembler::x86_64::Reg reg,
    Assembler::x86_64::Reg base,
    Signed_32 displacement) -> void {
  const bool rip_override = (Unsigned_8(base) & Unsigned_8(0x7)) ==
                            5;  // RBP/R13: mod=00 means RIP-relative
  const bool sib_override = (Unsigned_8(base) & Unsigned_8(0x7)) ==
                            4;  // RSP/R12: rm=4 means SIB byte

  AddressMode mod;
  if (displacement == 0 && !rip_override) {
    mod = AddressMode::Memory;
  } else if (displacement >= -128 && displacement <= 127) {
    mod = AddressMode::Memory_8;
  } else {
    mod = AddressMode::Memory_32;
  }

  if (sib_override) {
    // rm=4 signals SIB present
    code.append(gen_modrm_byte(mod, reg, Assembler::x86_64::Reg(0x4)));
    code.append(Unsigned_8(0x24));  // SIB: scale=0, no index, RSP/R12 base
  } else {
    code.append(gen_modrm_byte(mod, reg, base));
  }

  if (mod == AddressMode::Memory_8) {
    code.append(Unsigned_8(displacement));
  } else if (mod == AddressMode::Memory_32) {
    write_const(code, Unsigned_32(displacement));
  }
}

auto Assembler::x86_64::mov(
    Assembler::x86_64::Reg source,
    Assembler::x86_64::Reg destination) -> void {
  // 8-bit: opcode 0x88 (MR encoding: r/m8 = r8)
  if ((Unsigned_8(source) & Unsigned_8(0xF0)) == Unsigned_8(0x20)) {
    gen_rex_byte_8(code, source, destination);
    code.append(0x88);
    code.append(gen_modrm_byte(source, destination));
    return;
  }

  // 16-bit: 0x66 operand size prefix + opcode 0x89
  if ((Unsigned_8(source) & Unsigned_8(0xF0)) == Unsigned_8(0x30)) {
    code.append(Unsigned_8(0x66));
    gen_rex_byte_16(code, source, destination);
    code.append(0x89);
    code.append(gen_modrm_byte(source, destination));
    return;
  }

  // 32-bit and 64-bit: opcode 0x89 (MR encoding: r/m = r)
  gen_rex_byte(code, source, destination);
  code.append(0x89);
  code.append(gen_modrm_byte(source, destination));
}

auto Assembler::x86_64::mov(
    Unsigned_8 immediate,
    Assembler::x86_64::Reg destination) -> void {
  // B0+rd encoding for 8-bit immediate.
  // SPL/BPL/SIL/DIL (codes 4-7) need a bare REX to avoid selecting AH/CH/DH/BH.
  auto code_low = Unsigned_8(destination) & Unsigned_8(0x0F);
  if (code_low > 0x7) {
    code.append(Unsigned_8(RexExt::B));  // REX.B for R8B-R15B
  } else if (code_low >= 4) {
    code.append(Unsigned_8(0x40));  // Bare REX for SPL/BPL/SIL/DIL
  }

  code.append(0xB0 + reg_code(destination));
  code.append(immediate);
}

auto Assembler::x86_64::mov(
    Unsigned_16 immediate,
    Assembler::x86_64::Reg destination) -> void {
  // Only one 16 bit alternate encoding seems to be smaller.
  // XOR r32, r32 clears the 16 bit register with no 0x66 prefix (2 vs 4 bytes).
  if (immediate == 0) {
    zero(destination);
    return;
  }

  // 0x66 operand size prefix + B8 + rd encoding.
  code.append(Unsigned_8(0x66));
  if ((Unsigned_8(destination) & Unsigned_8(0x0F)) > 0x7) {
    code.append(Unsigned_8(RexExt::B));
  }

  code.append(0xB8 + reg_code(destination));
  write_const(code, immediate);
}

auto Assembler::x86_64::mov(
    Unsigned_32 immediate,
    Assembler::x86_64::Reg destination) -> void {
  // Special cases with more compact alternatives to mov.
  switch (immediate) {
  case 0:
    zero(destination);
    return;
  case 1:
    one(destination);
    return;
  }

  // B8 + rd encoding: zero-extends to 64 bits implicitly.
  if ((Unsigned_8(destination) & Unsigned_8(0x0F)) > 0x7) {
    code.append(Unsigned_8(RexExt::B));
  }

  code.append(0xB8 + reg_code(destination));
  write_const(code, immediate);
}

auto Assembler::x86_64::mov(
    Unsigned_64 immediate,
    Assembler::x86_64::Reg destination) -> void {
  // Special cases with more compact alternatives to mov.
  switch (immediate) {
  case 0:
    zero(destination);
    return;
  case 1:
    one(destination);
    return;
  case 0xFFFFFFFFFFFFFFFF:
    neg_one(destination);
    return;
  }

  // Zero-extending 32-bit move: MOV r32, imm32 (5-6 bytes, zero-extends to 64).
  if (immediate <= Unsigned_64(0xFFFFFFFF)) {
    if ((Unsigned_8(destination) & Unsigned_8(0x0F)) > 0x7) {
      code.append(Unsigned_8(RexExt::B));
    }

    code.append(0xB8 + reg_code(destination));
    write_const(code, Unsigned_32(immediate));
    return;
  }

  // Sign-extending 32-bit move: MOV r/m64, imm32 (7-8 bytes).
  // Handles [INT32_MIN, -2] more compactly than a 10-byte MOVABS.
  if (immediate >= Unsigned_64(0xFFFFFFFF80000000ULL)) {
    gen_rex_byte(code, Assembler::x86_64::Reg::None, destination);
    code.append(0xC7);
    code.append(gen_modrm_byte(Assembler::x86_64::Reg(0x00), destination));
    write_const(code, Unsigned_32(immediate));
    return;
  }

  // 10-byte absolute move: MOVABS r64, imm64.
  gen_rex_byte(code, Assembler::x86_64::Reg::None, destination);
  code.append(0xB8 + reg_code(destination));
  write_const(code, immediate);
}

auto Assembler::x86_64::mov(
    Assembler::x86_64::Reg source,
    Assembler::x86_64::Reg base,
    Signed_32 displacement) -> void {
  // Store: mov source, [base + displacement]
  if ((Unsigned_8(source) & Unsigned_8(0xF0)) == Unsigned_8(0x20)) {
    gen_rex_byte_8(code, source, base);
    code.append(0x88);
    gen_memory_operand(code, source, base, displacement);
    return;
  }

  if ((Unsigned_8(source) & Unsigned_8(0xF0)) == Unsigned_8(0x30)) {
    code.append(Unsigned_8(0x66));
    gen_rex_byte_16(code, source, base);
    code.append(0x89);
    gen_memory_operand(code, source, base, displacement);
    return;
  }

  if ((Unsigned_8(source) & Unsigned_8(0xF0)) == Unsigned_8(0x00)) {
    gen_rex_byte_32(code, source, base);
  } else {
    gen_rex_byte(code, source, base);
  }

  code.append(0x89);
  gen_memory_operand(code, source, base, displacement);
}

auto Assembler::x86_64::mov(
    Assembler::x86_64::Reg base,
    Signed_32 displacement,
    Assembler::x86_64::Reg destination) -> void {
  // Load: mov [base + displacement], destination
  if ((Unsigned_8(destination) & Unsigned_8(0xF0)) == Unsigned_8(0x20)) {
    gen_rex_byte_8(code, destination, base);
    code.append(0x8A);
    gen_memory_operand(code, destination, base, displacement);
    return;
  }

  if ((Unsigned_8(destination) & Unsigned_8(0xF0)) == Unsigned_8(0x30)) {
    code.append(Unsigned_8(0x66));
    gen_rex_byte_16(code, destination, base);
    code.append(0x8B);
    gen_memory_operand(code, destination, base, displacement);
    return;
  }

  if ((Unsigned_8(destination) & Unsigned_8(0xF0)) == Unsigned_8(0x00)) {
    gen_rex_byte_32(code, destination, base);
  } else {
    gen_rex_byte(code, destination, base);
  }

  code.append(0x8B);
  gen_memory_operand(code, destination, base, displacement);
}

auto Assembler::x86_64::mov_bits(
    Assembler::x86_64::Reg source,
    Assembler::x86_64::Xmm destination) -> void {
  code.append(Unsigned_8(0x66));
  Unsigned_8 rex = Unsigned_8(RexExt::W);
  if (Unsigned_8(destination) > 7) {
    rex |= Unsigned_8(RexExt::R);
  }

  if ((Unsigned_8(source) & Unsigned_8(0x0F)) > 7) {
    rex |= Unsigned_8(RexExt::B);
  }

  code.append(rex);
  code.append(Unsigned_8(0x0F));
  code.append(Unsigned_8(0x6E));
  code.append(
      Unsigned_8(AddressMode::RegToReg) |
      Unsigned_8(xmm_code(destination) << 3) | reg_code(source));
}

auto Assembler::x86_64::mov_bits(
    Assembler::x86_64::Xmm source,
    Assembler::x86_64::Reg destination) -> void {
  code.append(Unsigned_8(0x66));
  Unsigned_8 rex = Unsigned_8(RexExt::W);
  if (Unsigned_8(source) > 7) {
    rex |= Unsigned_8(RexExt::R);
  }

  if ((Unsigned_8(destination) & Unsigned_8(0x0F)) > 7) {
    rex |= Unsigned_8(RexExt::B);
  }

  code.append(rex);
  code.append(Unsigned_8(0x0F));
  code.append(Unsigned_8(0x7E));
  code.append(
      Unsigned_8(AddressMode::RegToReg) | Unsigned_8(xmm_code(source) << 3) |
      reg_code(destination));
}

auto Assembler::x86_64::push(Assembler::x86_64::Reg reg) -> void {
  // Short form push reg encoding.
  rex_short(code, reg);
  code.append(0x50 + reg_code(reg));
}

auto Assembler::x86_64::pop(Assembler::x86_64::Reg reg) -> void {
  // Short form push reg encoding.
  rex_short(code, reg);
  code.append(0x58 + reg_code(reg));
}

auto Assembler::x86_64::zero(Assembler::x86_64::Reg reg) -> void {
  // Save the REX byte if we can alias through the 32 bit reg.
  if (reg > Assembler::x86_64::Reg::RDI || reg < Assembler::x86_64::Reg::RAX) {
    gen_rex_byte(code, reg, reg);
  }

  // XOR
  code.append(0x31);
  code.append(gen_modrm_byte(reg, reg));
}

auto Assembler::x86_64::inc(Assembler::x86_64::Reg reg) -> void {
  gen_rex_byte(code, Assembler::x86_64::Reg::None, reg);

  // Inc calls 0xFF with reg=0 in gen_modrm_byte.
  code.append(0xFF);
  code.append(gen_modrm_byte(Assembler::x86_64::Reg(0x00), reg));
}

auto Assembler::x86_64::dec(Assembler::x86_64::Reg reg) -> void {
  gen_rex_byte(code, Assembler::x86_64::Reg::None, reg);

  // Inc calls 0xFF with reg=1 in gen_modrm_byte.
  code.append(0xFF);
  code.append(gen_modrm_byte(Assembler::x86_64::Reg(0x01), reg));
}

auto Assembler::x86_64::add(
    Assembler::x86_64::Reg source,
    Assembler::x86_64::Reg destination) -> void {
  gen_rex_byte(code, source, destination);
  code.append(0x01);
  code.append(gen_modrm_byte(source, destination));
}

auto Assembler::x86_64::add(
    Unsigned_32 immediate,
    Assembler::x86_64::Reg destination) -> void {
  if (immediate == 0) {
    return;
  }

  gen_rex_byte(code, Assembler::x86_64::Reg::None, destination);
  code.append(0x81);
  code.append(gen_modrm_byte(Assembler::x86_64::Reg(0x00), destination));
  write_const(code, immediate);
}

auto Assembler::x86_64::sub(
    Assembler::x86_64::Reg source,
    Assembler::x86_64::Reg destination) -> void {
  gen_rex_byte(code, source, destination);
  code.append(0x29);
  code.append(gen_modrm_byte(source, destination));
}

auto Assembler::x86_64::sub(
    Unsigned_32 immediate,
    Assembler::x86_64::Reg destination) -> void {
  if (immediate == 0) {
    return;
  }

  gen_rex_byte(code, Assembler::x86_64::Reg::None, destination);
  code.append(0x81);
  code.append(gen_modrm_byte(Assembler::x86_64::Reg(0x05), destination));
  write_const(code, immediate);
}

auto Assembler::x86_64::multiply(
    Assembler::x86_64::Reg source,
    Assembler::x86_64::Reg destination) -> void {
  gen_rex_byte(code, destination, source);
  code.append(0x0F);
  code.append(0xAF);
  code.append(gen_modrm_byte(destination, source));
}

auto Assembler::x86_64::compare(
    Assembler::x86_64::Reg source,
    Assembler::x86_64::Reg destination) -> void {
  gen_rex_byte(code, source, destination);
  code.append(Unsigned_8(0x39));
  code.append(gen_modrm_byte(source, destination));
}

auto Assembler::x86_64::set_equal(Assembler::x86_64::Reg destination) -> void {
  gen_rex_byte_8(code, Assembler::x86_64::Reg::None, destination);
  code.append(Unsigned_8(0x0F));
  code.append(Unsigned_8(0x94));
  code.append(gen_modrm_byte(Assembler::x86_64::Reg(0), destination));
}

auto Assembler::x86_64::divide(Assembler::x86_64::Reg divisor) -> void {
  zero(Assembler::x86_64::Reg::RDX);
  gen_rex_byte(code, Assembler::x86_64::Reg::None, divisor);
  code.append(0xF7);
  code.append(gen_modrm_byte(Assembler::x86_64::Reg(0x06), divisor));
}

auto Assembler::x86_64::signed_divide(Assembler::x86_64::Reg divisor) -> void {
  code.append(Unsigned_8(RexExt::W));
  code.append(Unsigned_8(0x99));
  gen_rex_byte(code, Assembler::x86_64::Reg::None, divisor);
  code.append(Unsigned_8(0xF7));
  code.append(gen_modrm_byte(Assembler::x86_64::Reg(0x07), divisor));
}

auto Assembler::x86_64::one(Assembler::x86_64::Reg reg) -> void {
  zero(reg);

  // See if we can save an extra REX byte by using only the 32bit reg.
  if (reg > Assembler::x86_64::Reg::RDI || reg < Assembler::x86_64::Reg::RAX) {
    gen_rex_byte(code, Assembler::x86_64::Reg::None, reg);
  }

  // Inc calls 0xFF with reg=0 in gen_modrm_byte.
  code.append(0xFF);
  code.append(gen_modrm_byte(Assembler::x86_64::Reg(0x00), reg));
}

auto Assembler::x86_64::neg_one(Assembler::x86_64::Reg reg) -> void {
  zero(reg);
  // Can't save the REX byte on dec since we need the wrap.
  dec(reg);
}

auto Assembler::x86_64::lea(
    Assembler::x86_64::Reg base,
    Signed_32 displacement,
    Assembler::x86_64::Reg destination) -> void {
  gen_rex_byte(code, destination, base);
  code.append(0x8D);
  gen_memory_operand(code, destination, base, displacement);
}

auto Assembler::x86_64::read_only(Assembler::x86_64::Reg destination) -> void {
  code.append(Unsigned_8(RexExt::W));
  code.append(0x8D);
  // LEA has a special case when mod=00 (Memory) and RM=101 (RBP / R13)
  // This uses RIP with a 32bit offset.
  code.append(gen_modrm_byte(
      AddressMode::Memory, destination, Assembler::x86_64::Reg::RBP));
  // Emit placeholder disp32 for the PC32 relocation to patch.
  write_const(code, Unsigned_32(0));
}

auto Assembler::x86_64::call() -> void {
  // Generate the hex for a PC32 call.
  code.append(0xE8);
  // Emit placeholder disp32 for the PC32 relocation to patch.
  write_const(code, Unsigned_32(0));
}

auto Assembler::x86_64::ret() -> void {
  code.append(0xC3);
}
