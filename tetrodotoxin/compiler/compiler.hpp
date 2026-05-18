// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/compiler/assembler/x86_64.hpp"
#include "tetrodotoxin/compiler/context/function.hpp"
#include "tetrodotoxin/compiler/context/names.hpp"
#include "tetrodotoxin/compiler/context/relocation.hpp"
#include "tetrodotoxin/compiler/context/strings.hpp"
#include "tetrodotoxin/compiler/context/symbol.hpp"
#include "tetrodotoxin/compiler/intermediate/argument.hpp"
#include "tetrodotoxin/compiler/intermediate/type.hpp"

namespace Tetrodotoxin::Compiler {

// Takes in a TTX file and breaks it down to it's binary representation.
// Machine code generated is for x86-64.
//
// TODO: Fix all of the hacky codegen.
class Compiler {
 public:
  Compiler() : arena(), names(arena) {}

  // Stub for compiling functions
  auto compile_function(
      Perimortem::Core::View::Bytes module,
      Perimortem::Core::View::Bytes name,
      Intermediate::Type return_type,
      Perimortem::Core::View::Vector<Intermediate::Argument> arguments)
      -> Count;

  // Emit the C ABI header declaring the exported symbols into header_out.
  auto generate_c_header() -> Perimortem::Memory::Dynamic::Bytes;

 private:
  auto load_string(
      Assembler::x86_64 assembler,
      Assembler::x86_64::Reg dst,
      Perimortem::Core::View::Bytes string) -> void;

  auto call_extern(
      Assembler::x86_64 assembler,
      Perimortem::Core::View::Bytes function) -> void;

  auto ref_local(Perimortem::Core::View::Bytes string_value) -> Count;
  auto ref_string(Perimortem::Core::View::Bytes string_value) -> Count;
  auto ref_extern(
      Perimortem::Core::View::Bytes string_value,
      Context::Symbol::Type type) -> Count;

  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Dynamic::Bytes machine_code;
  Context::Names names;
  Context::Strings strings;
  Perimortem::Memory::Dynamic::Vector<Context::Function> functions;
  Perimortem::Memory::Dynamic::Vector<Context::Relocation> relocs;
  Perimortem::Memory::Dynamic::Map<Perimortem::Core::View::Bytes, Count>
      sym_table;
  Perimortem::Memory::Dynamic::Vector<Context::Symbol> symbols;
};

}  // namespace Tetrodotoxin::Compiler
