// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/compiler/symbol/external.hpp"
#include "tetrodotoxin/compiler/symbol/function.hpp"
#include "tetrodotoxin/compiler/symbol/relocation.hpp"
#include "tetrodotoxin/compiler/symbol/string.hpp"
#include "tetrodotoxin/isa/library/block.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Compiler {

// Library lowers Library-owned function bodies into host machine code facts.
//
// The compiler consumes real Ttx::Type functions and Library blocks produced by
// the ISA. It publishes code bytes, strings, externals, and relocation facts
// for the linker instead of emitting linker symbols directly.
class Library {
 public:
  Library() = default;

  auto lower(
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Lexical::Errors& errors,
      Ttx::Lexical::Source source,
      Perimortem::Core::View::Bytes module,
      const Ttx::Type& root) -> Bool;

  constexpr auto get_machine_code() const -> Perimortem::Core::View::Bytes {
    return machine_code;
  }
  constexpr auto get_string_data() const -> Perimortem::Core::View::Bytes {
    return string_data;
  }
  constexpr auto get_functions() const
      -> Perimortem::Core::View::Vector<Symbol::Function> {
    return functions;
  }
  constexpr auto get_strings() const
      -> Perimortem::Core::View::Vector<Symbol::String> {
    return strings;
  }
  constexpr auto get_externals() const
      -> Perimortem::Core::View::Vector<Symbol::External> {
    return externals;
  }
  constexpr auto get_relocations() const
      -> Perimortem::Core::View::Vector<Symbol::Relocation> {
    return relocations;
  }

 private:
  auto lower_function(
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Lexical::Errors& errors,
      Ttx::Lexical::Source source,
      Perimortem::Core::View::Bytes module,
      const Ttx::Type::Function& function) -> Bool;
  auto lower_block(
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Lexical::Errors& errors,
      Ttx::Lexical::Source source,
      const Ttx::Type::Function& function,
      Ttx::Type::Function::Block block) -> Bool;
  auto lower_statement(
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Lexical::Errors& errors,
      Ttx::Lexical::Source source,
      const Ttx::Type::Function& function,
      const Tetrodotoxin::Isa::Library::Statement& statement) -> Bool;
  auto lower_call(
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Lexical::Errors& errors,
      Ttx::Lexical::Source source,
      const Ttx::Type::Function& function,
      const Tetrodotoxin::Isa::Library::Call& call) -> Bool;
  auto lower_pack_value(
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Lexical::Errors& errors,
      Ttx::Lexical::Source source,
      const Ttx::Type::Function& function,
      const Tetrodotoxin::Isa::Expression::Value& value,
      const Ttx::Type::Function& callee) -> Bool;

  auto emit_string_argument(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes value) -> Bool;
  auto emit_parameter_argument(
      Ttx::Lexical::Errors& errors,
      Ttx::Lexical::Source source,
      const Ttx::Type::Function& function,
      Perimortem::Core::View::Bytes name) -> Bool;
  auto call_external(Perimortem::Core::View::Bytes name) -> void;
  auto string_index(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes value) -> Count;
  auto external_index(Perimortem::Core::View::Bytes name) -> Count;
  auto function_name(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes module,
      Perimortem::Core::View::Bytes function) -> Perimortem::Core::View::Bytes;
  auto local_string_name(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes value) -> Perimortem::Core::View::Bytes;
  auto append_name_segment(
      Perimortem::Memory::Managed::Bytes& output,
      Perimortem::Core::View::Bytes value) -> void;
  auto append_hex(Perimortem::Memory::Managed::Bytes& output, Bits_64 value)
      -> void;

  static auto is_view_bytes(const Ttx::Type* type) -> Bool;
  static auto is_void_result(
      Perimortem::Core::View::Vector<Ttx::Type::Member> result) -> Bool;

  Perimortem::Memory::Dynamic::Bytes machine_code;
  Perimortem::Memory::Dynamic::Bytes string_data;
  Perimortem::Memory::Dynamic::Vector<Symbol::Function> functions;
  Perimortem::Memory::Dynamic::Vector<Symbol::String> strings;
  Perimortem::Memory::Dynamic::Vector<Symbol::External> externals;
  Perimortem::Memory::Dynamic::Vector<Symbol::Relocation> relocations;
};

}  // namespace Tetrodotoxin::Compiler
