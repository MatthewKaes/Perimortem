// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/linker/linker.hpp"
#include "tetrodotoxin/linker/object/relocation.hpp"
#include "tetrodotoxin/linker/object/symbol.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Compiler {

class Library {
 public:
  explicit Library(Perimortem::Memory::Allocator::Arena& arena)
      : arena(arena), error(arena) {}

  auto lower(
      Perimortem::Core::View::Bytes module,
      const Ttx::Type& root) -> Bool;
  auto add_to(Tetrodotoxin::Linker::Linker& linker) const -> void;
  auto append_header(Perimortem::Memory::Dynamic::Bytes& header) const -> void;

  constexpr auto get_error() const -> Perimortem::Core::View::Bytes {
    return error.get_view();
  }

 private:
  class Declaration {
   public:
    Perimortem::Core::View::Bytes symbol_name;
    Ttx::Type::Function function;
  };

  auto lower_function(
      Perimortem::Core::View::Bytes module,
      const Ttx::Type& root,
      const Ttx::Type::Function& function) -> Bool;
  auto lower_block(
      const Ttx::Type& root,
      const Ttx::Type::Function& function,
      Ttx::Type::Function::Block block) -> Bool;
  auto lower_statement(
      const Ttx::Type& root,
      const Ttx::Type::Function& function,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> tokens,
      Count& index) -> Bool;
  auto lower_foreign_call(
      const Ttx::Type& root,
      const Ttx::Type::Function& function,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> tokens,
      Count& index) -> Bool;
  auto lower_argument(
      const Ttx::Type::Function& function,
      Perimortem::Core::View::Vector<Ttx::Lexical::Token> tokens,
      Count& index,
      const Ttx::Type::Function& callee) -> Bool;

  auto emit_string_argument(Perimortem::Core::View::Bytes token_text) -> Bool;
  auto emit_parameter_argument(
      const Ttx::Type::Function& function,
      Perimortem::Core::View::Bytes name) -> Bool;
  auto call_external(Perimortem::Core::View::Bytes name) -> void;
  auto string_symbol(Perimortem::Core::View::Bytes value) -> Count;
  auto external_symbol(
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Linker::Object::Symbol::Type type) -> Count;
  auto symbol_name(
      Perimortem::Core::View::Bytes module,
      Perimortem::Core::View::Bytes function) -> Perimortem::Core::View::Bytes;
  auto local_symbol_name(Perimortem::Core::View::Bytes value)
      -> Perimortem::Core::View::Bytes;
  auto append_symbol_segment(
      Perimortem::Memory::Managed::Bytes& output,
      Perimortem::Core::View::Bytes value) -> void;
  auto append_hex(
      Perimortem::Memory::Managed::Bytes& output,
      Bits_64 value) -> void;
  auto keep(Perimortem::Core::View::Bytes bytes)
      -> Perimortem::Core::View::Bytes;
  auto set_error(Perimortem::Core::View::Bytes message) -> Bool;

  static auto cpp_type(const Ttx::Type* type) -> Perimortem::Core::View::Bytes;
  static auto has_attribute(
      const Ttx::Type& type,
      Perimortem::Core::View::Bytes key,
      Perimortem::Core::View::Bytes value) -> Bool;
  static auto is_view_bytes(const Ttx::Type* type) -> Bool;
  static auto is_void_result(
      Perimortem::Core::View::Vector<Ttx::Type::Member> result) -> Bool;

  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Memory::Dynamic::Bytes machine_code;
  Perimortem::Memory::Dynamic::Bytes string_data;
  Perimortem::Memory::Dynamic::Vector<Declaration> declarations;
  Perimortem::Memory::Dynamic::Vector<Tetrodotoxin::Linker::Object::Symbol>
      symbols;
  Perimortem::Memory::Dynamic::Vector<Tetrodotoxin::Linker::Object::Relocation>
      relocations;
  Perimortem::Memory::Managed::Bytes error;
};

}  // namespace Tetrodotoxin::Compiler
