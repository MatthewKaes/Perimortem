// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/compiler/intermediate/argument.hpp"

namespace Tetrodotoxin::Compiler::Assembler {

// Assembler for the x86_64 ISA
// TODO: Generalize to an ISA interface at some point.
// Also SSA needs to be pulled out at some point for optimization passes.
class x86_64 {
 public:
  enum class Reg {
    None = -1,
    EAX = 0x0,
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
    R15
  };

  x86_64(Perimortem::Memory::Dynamic::Bytes& machine_code)
      : code(machine_code) {}

  // Sets up a function for use.
  auto prologue(
      Perimortem::Core::View::Vector<Intermediate::Argument> arguments) -> void;

  // returns and clears the stack.
  auto epilogue(Intermediate::Type type) -> void;

  // Exits the current scope.
  auto close() -> void {};

  auto mov(Reg src, Reg dst) -> void;
  auto mov(Bits_64 r64, Reg dst) -> void;
  auto push(Reg reg) -> void;
  auto pop(Reg reg) -> void;
  auto zero(Reg reg) -> void;
  auto inc(Reg reg) -> void;
  auto dec(Reg reg) -> void;
  auto one(Reg reg) -> void;
  auto neg_one(Reg reg) -> void;

  // Loads the address of read only data into the specified reg.
  // Creates null padding to support a PC32 offset relocate by the linker.
  auto read_only(Reg dst) -> void;

  // Creates a call with null padding to support a Program Counter +
  // 32bit offset relocate by the linker.
  //
  // It's up to the compiler to create the correct symbol for the linker.
  auto call() -> void;

 private:
  Perimortem::Memory::Dynamic::Bytes& code;
};

}  // namespace Tetrodotoxin::Compiler::Assembler
