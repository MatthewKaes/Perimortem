// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Tetrodotoxin::Compiler::Assembler {

// x86_64 is the private machine-code writer selected by the System V backend.
// It receives physical operands only, with SSA identity and allocation being
// part of the target-independent compiler layer.
//
// The emitter still performs at least some micro optimization by attempting to
// emit the shortest encodings possible for most common idioms. Higher order op
// fusion and microcode optimizations are not currently performed.
class x86_64 {
 public:
  enum class Reg {
    None = -1,
    // 32-bit registers
    EAX = 0x00,
    ECX,
    EDX,
    EBX,
    ESP,
    EBP,
    ESI,
    EDI,
    R8D,
    R9D,
    R10D,
    R11D,
    R12D,
    R13D,
    R14D,
    R15D,
    // 64-bit registers
    RAX = 0x10,
    RCX,
    RDX,
    RBX,
    RSP,
    RBP,
    RSI,
    RDI,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,
    // 8-bit registers (REX required for SPL/BPL/SIL/DIL)
    AL = 0x20,
    CL,
    DL,
    BL,
    SPL,
    BPL,
    SIL,
    DIL,
    R8B,
    R9B,
    R10B,
    R11B,
    R12B,
    R13B,
    R14B,
    R15B,
    // 16-bit registers
    AX = 0x30,
    CX,
    DX,
    BX,
    SP,
    BP,
    SI,
    DI,
    R8W,
    R9W,
    R10W,
    R11W,
    R12W,
    R13W,
    R14W,
    R15W,
  };

  enum class Xmm : Unsigned_8 {
    XMM0,
    XMM1,
    XMM2,
    XMM3,
    XMM4,
    XMM5,
    XMM6,
    XMM7,
  };

  x86_64(Perimortem::Memory::Dynamic::Bytes& machine_code)
      : code(machine_code) {}

  // Exits the current scope.
  auto close() -> void {};

  auto mov(Reg source, Reg destination) -> void;
  auto mov(Unsigned_8 immediate, Reg destination) -> void;
  auto mov(Unsigned_16 immediate, Reg destination) -> void;
  auto mov(Unsigned_32 immediate, Reg destination) -> void;
  // Encoding optimized 64 bit mov.
  auto mov(Unsigned_64 immediate, Reg destination) -> void;
  // Store to memory: mov source, [base + displacement]
  auto mov(Reg source, Reg base, Signed_32 displacement) -> void;
  // Load from memory: mov [base + displacement], destination
  auto mov(Reg base, Signed_32 displacement, Reg destination) -> void;
  auto mov_bits(Reg source, Xmm destination) -> void;
  auto mov_bits(Xmm source, Reg destination) -> void;
  auto push(Reg reg) -> void;
  auto pop(Reg reg) -> void;
  auto zero(Reg reg) -> void;
  auto inc(Reg reg) -> void;
  auto dec(Reg reg) -> void;
  auto add(Reg source, Reg destination) -> void;
  auto add(Unsigned_32 immediate, Reg destination) -> void;
  auto sub(Reg source, Reg destination) -> void;
  auto sub(Unsigned_32 immediate, Reg destination) -> void;
  auto multiply(Reg source, Reg destination) -> void;
  auto compare(Reg source, Reg destination) -> void;
  auto set_equal(Reg destination) -> void;
  // Unsigned division consumes RDX:RAX and leaves the quotient in RAX.
  auto divide(Reg divisor) -> void;
  // Signed division consumes RDX:RAX and leaves quotient and remainder in
  // RAX and RDX.
  auto signed_divide(Reg divisor) -> void;
  auto one(Reg reg) -> void;
  auto neg_one(Reg reg) -> void;
  auto lea(Reg base, Signed_32 displacement, Reg destination) -> void;

  // Loads the address of read only data into the specified register.
  // Creates null padding for a PC32 relocation target.
  auto read_only(Reg destination) -> void;

  // Creates a call with null padding for a Program Counter + 32 bit
  // relocation target.
  auto call() -> void;
  auto ret() -> void;

 private:
  Perimortem::Memory::Dynamic::Bytes& code;
};

}  // namespace Tetrodotoxin::Compiler::Assembler
