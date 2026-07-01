// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/abi/argument.hpp"
#include "tetrodotoxin/abi/call.hpp"
#include "tetrodotoxin/abi/type.hpp"
#include "tetrodotoxin/compiler/assembler/x86_64.hpp"
#include "tetrodotoxin/compiler/context/function.hpp"
#include "tetrodotoxin/compiler/context/names.hpp"
#include "tetrodotoxin/compiler/context/relocation.hpp"
#include "tetrodotoxin/compiler/context/strings.hpp"
#include "tetrodotoxin/compiler/context/symbol.hpp"

namespace Tetrodotoxin::Compiler {

// Compiles TTX functions to x86-64 machine code and tracks the resulting
// symbol and relocation tables. The TTX linker is responsible for packaging the
// output into an object file or archive.
class Compiler {
 public:
  Compiler() : arena(), names(arena) {}

  // Compile a TTX function and record it in the internal symbol table.
  // Returns false when the function body uses a shape this backend does not
  // honestly lower yet.
  auto compile_func(
      Perimortem::Core::View::Bytes module_name,
      Perimortem::Core::View::Bytes function_name,
      Abi::Type return_type,
      Perimortem::Core::View::Vector<Abi::Argument> arguments,
      Perimortem::Core::View::Vector<Abi::ForeignCall> foreign_calls)
      -> Bool;

  // Record an immutable data blob as an externally visible object symbol.
  // Shader programs use this for SPIR-V words and metadata, and other generated
  // backends can use the same path for read-only payloads.
  auto add_read_only_data(
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Bytes data,
      Count alignment = 8,
      Context::Symbol::Visability visability =
          Context::Symbol::Visability::Global) -> Count;

  auto add_object_definition(const Abi::ObjectDefinition& definition) -> void;

  // Emit a C header declaring all compiled functions.
  auto generate_cpp_header(Perimortem::Memory::Dynamic::Bytes& header_out)
      -> void;

  auto get_code() const -> Perimortem::Core::View::Bytes {
    return machine_code;
  }

  auto get_strings() const -> Perimortem::Core::View::Bytes {
    return strings.get_view();
  }

  auto get_read_only() const -> Perimortem::Core::View::Bytes {
    return read_only;
  }

  auto get_symbols() const -> Perimortem::Core::View::Vector<Context::Symbol> {
    return symbols;
  }

  auto get_relocs() const
      -> Perimortem::Core::View::Vector<Context::Relocation> {
    return relocs;
  }

 private:
  auto load_string(
      Assembler::x86_64& assembler,
      Assembler::x86_64::Reg destination,
      Perimortem::Core::View::Bytes string) -> void;

  auto store_object_value(
      Assembler::x86_64& assembler,
      const Abi::ObjectField& field,
      const Abi::ObjectValue& value,
      Signed_32 displacement) -> void;

  auto store_zeroed_bytes(
      Assembler::x86_64& assembler,
      Count byte_size,
      Signed_32 displacement) -> void;

  auto store_object(
      Assembler::x86_64& assembler,
      const Abi::Object& object,
      Signed_32 displacement) -> void;

  auto call_extern(
      Assembler::x86_64& assembler,
      Perimortem::Core::View::Bytes function_name) -> void;

  auto ref_extern(
      Perimortem::Core::View::Bytes name,
      Context::Symbol::Type type) -> Count;

  auto ref_string(Perimortem::Core::View::Bytes string_value) -> Count;

  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Dynamic::Bytes machine_code;
  Perimortem::Memory::Dynamic::Bytes read_only;
  Context::Names names;
  Context::Strings strings;
  Perimortem::Memory::Dynamic::Vector<Abi::ObjectDefinition>
      object_definitions;
  Perimortem::Memory::Dynamic::Vector<Context::Function> functions;
  Perimortem::Memory::Dynamic::Vector<Context::Relocation> relocs;
  Perimortem::Memory::Dynamic::Vector<Context::Symbol> symbols;
};

}  // namespace Tetrodotoxin::Compiler
