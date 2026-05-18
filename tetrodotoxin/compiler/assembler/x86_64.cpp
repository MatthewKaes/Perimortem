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
  // TODO: Only 64 bits so this is pointless for W
  // Also it would be a size mismatch so we should only have to check one.
  if (reg >= Reg::RAX || rm >= Reg::RAX) {
    Byte rex_code = Byte(RexExt::W);

    // Register extension
    if (reg > Reg::RDI) {
      rex_code |= Byte(RexExt::R);
    }

    // Reg / Mem extension
    if (rm > Reg::RDI) {
      rex_code |= Byte(RexExt::B);
    }

    code.append(rex_code);
  }
}

constexpr auto gen_modrm_byte(AddressMode mode, Reg reg, Reg rm) -> Byte {
  return Byte(mode) | (reg_code(reg) << 3) | reg_code(rm);
}

constexpr auto gen_modrm_byte(Reg reg, Reg rm) -> Byte {
  return Byte(AddressMode::RegToReg) | (reg_code(reg) << 3) | reg_code(rm);
}

auto x86_64::mov(Reg src, Reg dst) -> void {
  // Encoding for mov (0x89): dst = reg, src = r/m
  gen_rex_byte(code, dst, src);

  // mov to dest is standard for reg to reg, 0x8B for mem to reg.
  code.append(0x89);

  // gen_modrm_byte
  code.append(gen_modrm_byte(dst, src));
}

// TODO: support the signed variant with 7+ byte encoding.
auto x86_64::mov(Bits_64 r64, Reg dst) -> void {
  // Easy to optimize mov conversions.
  switch (r64) {
  case 1:
    one(dst);
    return;
  case 0:
    zero(dst);
    return;
  case 0xFFFFFFFFFFFFFFFF:
    neg_one(dst);
    return;
  }
  if (r64 == 0) {
    zero(dst);
    return;
  }

  const auto upper_32 = r64 & 0xFFFFFFFF;
  const auto sign_extendable =
      upper_32 == 0 ? true : upper_32 == 0xFFFFFFFF00000000;

  // Optimize for 32 bit literal moves.
  if (r64 < 0xFFFFFFFF) {
    if (dst <= Reg::RDI) {
      // 5 byte zero extended move
      code.append(0xB8 + reg_code(dst));
      write_const(code, Bits_32(r64));
      return;
    } else {
      // 7 byte imm32 move into R8+
      code.append(Byte(RexExt::B) | Byte(RexExt::W));
      code.append(0xB8 + reg_code(dst));
      code.append(gen_modrm_byte(Reg(0x00), dst));
    }
  }

  // 10 byte encoding to moveabs the value into the register.
  gen_rex_byte(code, Reg::None, dst);
  code.append(0xB8 + reg_code(dst));
  write_const(code, Bits_64(r64));
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
  if (reg > Reg::RDI) {
    code.append(Byte(RexExt::B) | Byte(RexExt::R));
  }

  // XOR
  code.append(0x31);
  code.append(gen_modrm_byte(reg, reg));
}

auto x86_64::inc(Reg reg) -> void {
  rex_short(code, reg);

  // Inc calls 0xFF with reg=0 in gen_modrm_byte.
  code.append(0xFF);
  code.append(gen_modrm_byte(Reg(0x00), reg));
}

auto x86_64::dec(Reg reg) -> void {
  rex_short(code, reg);

  // Inc calls 0xFF with reg=1 in gen_modrm_byte.
  code.append(0xFF);
  code.append(gen_modrm_byte(Reg(0x01), reg));
}

auto x86_64::one(Reg reg) -> void {
  zero(reg);

  // See if we can save an extra REX byte by using only the 32bit reg.
  if (reg >= Reg::RAX && reg <= Reg::RDI) {
    reg = Reg(Byte(reg) & 0x7);
  }
  inc(reg);
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
  code.concat("\xE8\x00\x00\x00\x00"_view);
}
