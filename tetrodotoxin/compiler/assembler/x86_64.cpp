// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/assembler/x86_64.hpp"

#include "perimortem/core/data.hpp"

#include "perimortem/utility/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Compiler;
using namespace Tetrodotoxin::Compiler::Assembler;

using Bytes = Perimortem::Memory::Dynamic::Bytes;

using Reg = x86_64::Reg;

constexpr auto reg_code(Reg reg) -> Byte {
  return Byte(reg) & Byte(0x7);
}

// REX extensions.
// Can be bit or'ed together to add multiple extensions.
enum class RexExt : Byte {
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
enum class AddressMode : Byte {
  Memory = 0b00000000,     // Memory with no displacement
  Memory_8 = 0b01000000,   // Memory with 1 Byte of displacement (signed)
  Memory_32 = 0b10000000,  // Memory with 4 Byte of displacement (signed)
  RegToReg = 0b11000000,   // Operand size expanded to 64 bits.
};

template <typename bit_type>
constexpr auto write_const(Bytes& code, bit_type data) -> void {
  // Ensure data is written in little endian.
  data = Data::ensure_endian<Data::ByteOrder::Native, Data::ByteOrder::Little>(
      data);

  code.resize(code.get_size() + sizeof(bit_type));
  Data::copy(
      code.get_access().get_data() + code.get_size() - sizeof(bit_type), &data,
      1);
}

constexpr auto rex_short(Bytes& code, Reg reg) -> void {
  if (reg > Reg::RDI) {
    code.append(Byte(RexExt::B));
  }
}

constexpr auto gen_rex_byte(Bytes& code, Reg reg, Reg rm) -> void {
  Byte rex_code = 0;

  // REX.W only for 64-bit operand size (upper nibble == 0x1)
  if ((reg != Reg::None && (Byte(reg) & Byte(0xF0)) == Byte(0x10)) ||
      (rm != Reg::None && (Byte(rm) & Byte(0xF0)) == Byte(0x10))) {
    rex_code = Byte(RexExt::W);
  }

  // Register extension to 4 bit address
  if (reg != Reg::None && (Byte(reg) & Byte(0x0F)) > 0x07) {
    rex_code |= Byte(RexExt::R);
  }

  // Reg / Mem extension to 4 bit address
  if (rm != Reg::None && (Byte(rm) & Byte(0x0F)) > 0x07) {
    rex_code |= Byte(RexExt::B);
  }

  if (rex_code) {
    code.append(rex_code);
  }
}

// REX for 16-bit register operands (0x66 prefix handles operand size, no
// REX.W).
constexpr auto gen_rex_byte_16(Bytes& code, Reg reg, Reg rm) -> void {
  Byte rex_code = 0;
  if (reg != Reg::None && (Byte(reg) & Byte(0x0F)) > 0x07) {
    rex_code |= Byte(RexExt::R);
  }
  if (rm != Reg::None && (Byte(rm) & Byte(0x0F)) > 0x07) {
    rex_code |= Byte(RexExt::B);
  }
  if (rex_code) {
    code.append(rex_code);
  }
}

// REX for 8-bit register operands.
// SPL/BPL/SIL/DIL (codes 4-7) require a bare REX prefix to distinguish them
// from the legacy AH/CH/DH/BH registers which share those encoding codes.
constexpr auto gen_rex_byte_8(Bytes& code, Reg reg, Reg rm) -> void {
  Byte rex_code = 0;
  if (reg != Reg::None && (Byte(reg) & Byte(0x0F)) > 0x07) {
    rex_code |= Byte(RexExt::R);
  }
  if (rm != Reg::None && (Byte(rm) & Byte(0x0F)) > 0x07) {
    rex_code |= Byte(RexExt::B);
  }

  // SPL/BPL/SIL/DIL special case due to encoding aliasing.
  auto anti_alias_rex = [](Reg r) -> bool {
    auto c = Byte(r) & Byte(0x0F);
    return c >= 0x04 && c <= 0x07;
  };

  if (anti_alias_rex(reg) || anti_alias_rex(rm)) {
    rex_code |= Byte(RexExt::Bare);
  }
  if (rex_code) {
    code.append(rex_code);
  }
}

constexpr auto gen_modrm_byte(AddressMode mode, Reg reg, Reg rm) -> Byte {
  return Byte(mode) | (reg_code(reg) << 3) | reg_code(rm);
}

constexpr auto gen_modrm_byte(Reg reg, Reg rm) -> Byte {
  return Byte(AddressMode::RegToReg) | (reg_code(reg) << 3) | reg_code(rm);
}

// Writes ModRM and optional SIB/displacement bytes for a [base+disp] memory
// operand.
static auto
    gen_memory_operand(Bytes& code, Reg reg, Reg base, SignedBits_32 disp)
        -> void {
  const bool rip_override =
      (Byte(base) & Byte(0x7)) == 5;  // RBP/R13: mod=00 means RIP-relative
  const bool sib_override =
      (Byte(base) & Byte(0x7)) == 4;  // RSP/R12: rm=4 means SIB byte

  AddressMode mod;
  if (disp == 0 && !rip_override) {
    mod = AddressMode::Memory;
  } else if (disp >= -128 && disp <= 127) {
    mod = AddressMode::Memory_8;
  } else {
    mod = AddressMode::Memory_32;
  }

  if (sib_override) {
    // rm=4 signals SIB present
    code.append(gen_modrm_byte(mod, reg, Reg(0x4)));
    code.append(Byte(0x24));  // SIB: scale=0, no index, RSP/R12 base
  } else {
    code.append(gen_modrm_byte(mod, reg, base));
  }

  if (mod == AddressMode::Memory_8) {
    code.append(Byte(disp));
  } else if (mod == AddressMode::Memory_32) {
    write_const(code, Bits_32(disp));
  }
}

auto x86_64::mov(Reg src, Reg dst) -> void {
  // 8-bit: opcode 0x88 (MR encoding: r/m8 = r8)
  if ((Byte(src) & Byte(0xF0)) == Byte(0x20)) {
    gen_rex_byte_8(code, src, dst);
    code.append(0x88);
    code.append(gen_modrm_byte(src, dst));
    return;
  }

  // 16-bit: 0x66 operand size prefix + opcode 0x89
  if ((Byte(src) & Byte(0xF0)) == Byte(0x30)) {
    code.append(Byte(0x66));
    gen_rex_byte_16(code, src, dst);
    code.append(0x89);
    code.append(gen_modrm_byte(src, dst));
    return;
  }

  // 32-bit and 64-bit: opcode 0x89 (MR encoding: r/m = r)
  gen_rex_byte(code, src, dst);
  code.append(0x89);
  code.append(gen_modrm_byte(src, dst));
}

auto x86_64::mov(Byte r8, Reg dst) -> void {
  // B0+rd encoding for 8-bit immediate.
  // SPL/BPL/SIL/DIL (codes 4-7) need a bare REX to avoid selecting AH/CH/DH/BH.
  auto code_low = Byte(dst) & Byte(0x0F);
  if (code_low > 0x7) {
    code.append(Byte(RexExt::B));  // REX.B for R8B-R15B
  } else if (code_low >= 4) {
    code.append(Byte(0x40));  // Bare REX for SPL/BPL/SIL/DIL
  }
  code.append(0xB0 + reg_code(dst));
  code.append(r8);
}

auto x86_64::mov(Bits_16 r16, Reg dst) -> void {
  // 0x66 operand size prefix + B8 + rd encoding.
  code.append(Byte(0x66));
  if ((Byte(dst) & Byte(0x0F)) > 0x7) {
    code.append(Byte(RexExt::B));
  }
  code.append(0xB8 + reg_code(dst));
  write_const(code, r16);
}

auto x86_64::mov(Bits_32 r32, Reg dst) -> void {
  // B8 + rd encoding: zero-extends to 64 bits implicitly.
  if ((Byte(dst) & Byte(0x0F)) > 0x7) {
    code.append(Byte(RexExt::B));
  }
  code.append(0xB8 + reg_code(dst));
  write_const(code, r32);
}

auto x86_64::mov(Bits_64 r64, Reg dst) -> void {
  switch (r64) {
  case 0:
    zero(dst);
    return;
  case 1:
    one(dst);
    return;
  case 0xFFFFFFFFFFFFFFFF:
    neg_one(dst);
    return;
  }

  // Zero-extending 32-bit move: MOV r32, imm32 (5-6 bytes, zero-extends to 64 bits).
  if (r64 <= Bits_64(0xFFFFFFFF)) {
    if ((Byte(dst) & Byte(0x0F)) > 0x7) {
      code.append(Byte(RexExt::B));
    }
    code.append(0xB8 + reg_code(dst));
    write_const(code, Bits_32(r64));
    return;
  }

  // Sign-extending 32-bit move: MOV r/m64, imm32 (7-8 bytes).
  // Handles [INT32_MIN, -2] more compactly than a 10-byte MOVABS.
  if (r64 >= Bits_64(0xFFFFFFFF80000000ULL)) {
    gen_rex_byte(code, Reg::None, dst);
    code.append(0xC7);
    code.append(gen_modrm_byte(Reg(0x00), dst));
    write_const(code, Bits_32(r64));
    return;
  }

  // 10-byte absolute move: MOVABS r64, imm64.
  gen_rex_byte(code, Reg::None, dst);
  code.append(0xB8 + reg_code(dst));
  write_const(code, r64);
}

auto x86_64::mov(Reg src, Reg base, SignedBits_32 disp) -> void {
  // Store: mov src, [base+disp]
  gen_rex_byte(code, src, base);
  code.append(0x89);
  gen_memory_operand(code, src, base, disp);
}

auto x86_64::mov(Reg base, SignedBits_32 disp, Reg dst) -> void {
  // Load: mov [base+disp], dst
  gen_rex_byte(code, dst, base);
  code.append(0x8B);
  gen_memory_operand(code, dst, base, disp);
}

auto x86_64::push(Reg reg) -> void {
  // Short form push reg encoding.
  rex_short(code, reg);
  code.append(0x50 + reg_code(reg));
}

auto x86_64::pop(Reg reg) -> void {
  // Short form push reg encoding.
  rex_short(code, reg);
  code.append(0x58 + reg_code(reg));
}

auto x86_64::zero(Reg reg) -> void {
  // Save the REX byte if we can alias through the 32 bit reg.
  if (reg > Reg::RDI || reg < Reg::RAX) {
    gen_rex_byte(code, reg, reg);
  }

  // XOR
  code.append(0x31);
  code.append(gen_modrm_byte(reg, reg));
}

auto x86_64::inc(Reg reg) -> void {
  gen_rex_byte(code, Reg::None, reg);

  // Inc calls 0xFF with reg=0 in gen_modrm_byte.
  code.append(0xFF);
  code.append(gen_modrm_byte(Reg(0x00), reg));
}

auto x86_64::dec(Reg reg) -> void {
  gen_rex_byte(code, Reg::None, reg);

  // Inc calls 0xFF with reg=1 in gen_modrm_byte.
  code.append(0xFF);
  code.append(gen_modrm_byte(Reg(0x01), reg));
}

auto x86_64::one(Reg reg) -> void {
  zero(reg);

  // See if we can save an extra REX byte by using only the 32bit reg.
  if (reg > Reg::RDI || reg < Reg::RAX) {
    gen_rex_byte(code, Reg::None, reg);
  }
  // Inc calls 0xFF with reg=0 in gen_modrm_byte.
  code.append(0xFF);
  code.append(gen_modrm_byte(Reg(0x00), reg));
}

auto x86_64::neg_one(Reg reg) -> void {
  zero(reg);
  // Can't save the REX byte on dec since we need the wrap.
  dec(reg);
}

auto x86_64::read_only(Reg dst) -> void {
  code.append(Byte(RexExt::W));
  code.append(0x8D);
  // LEA has a special case when mod=00 (Memory) and RM=101 (RBP / R13)
  // This uses RIP with a 32bit offset.
  code.append(gen_modrm_byte(AddressMode::Memory, dst, Reg::RBP));
  // Emit placeholder disp32 for the PC32 relocation to patch.
  write_const(code, Bits_32(0));
}

auto x86_64::prologue(
    Perimortem::Core::View::Vector<Intermediate::Argument> arguments) -> void {
  // TODO: generate SSA so we can do mem_to_reg analysis
  push(Reg::RBP);
}

// returns and clears the stack.
auto x86_64::epilogue(Intermediate::Type type) -> void {
  // Pop stored registers in reverse order.
  pop(Reg::RBP);
  code.append(0xC3);
}

auto x86_64::call() -> void {
  // Generate the hex for a PC32 call.
  code.append(0xE8);
  // Emit placeholder disp32 for the PC32 relocation to patch.
  write_const(code, Bits_32(0));
}
