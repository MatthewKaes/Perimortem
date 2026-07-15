// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/archiver/terminal.hpp"
#include "tetrodotoxin/compiler/engine.hpp"
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Isa::Lowering {

// Borrows the services available to one selected ISA lowerer. Puffer owns the
// arena, diagnostics, execution product, native publisher, and terminal list
// for the whole compilation transaction. Context exposes the transactions an
// ISA may perform without exposing Program as a mutable container or giving
// the lowerer a place to cache derived products.
class Context {
 public:
  Context(
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Lexical::Errors& errors,
      Tetrodotoxin::Compiler::Program& program,
      Tetrodotoxin::Compiler::Engine& engine,
      Perimortem::Memory::Managed::Vector<Tetrodotoxin::Archiver::Terminal>&
          terminals)
      : arena(arena),
        errors(errors),
        program(program),
        engine(engine),
        terminals(terminals) {}

  // Copies an opaque terminal product into transaction storage because the
  // lowerer's scratch storage may be released before package serialization.
  auto publish(
      Perimortem::Core::View::Bytes group,
      Perimortem::Core::View::Bytes path,
      Perimortem::Core::View::Bytes content) -> void {
    terminals.insert(
        Tetrodotoxin::Archiver::Terminal(
            retain(group), retain(path), retain(content)));
  }

  constexpr auto get_arena() const -> Perimortem::Memory::Allocator::Arena& {
    return arena;
  }

  constexpr auto get_errors() const -> Ttx::Lexical::Errors& { return errors; }

  auto publish_function(
      Ttx::Lexical::Source source,
      Perimortem::Core::View::Bytes symbol,
      const Ttx::Function& function,
      const Tetrodotoxin::Compiler::Execution::Body& body) -> Bool {
    return program.define(source, symbol, function, body);
  }

  auto publish_read_only(
      Perimortem::Core::View::Bytes content,
      Perimortem::Core::View::Vector<Tetrodotoxin::Compiler::Symbol> symbols)
      -> Bool {
    return engine.publish_read_only(content, symbols);
  }

 private:
  auto retain(Perimortem::Core::View::Bytes source)
      -> Perimortem::Core::View::Bytes {
    if (source.is_empty()) {
      return Perimortem::Core::View::Bytes();
    }

    Bits_8* copy = arena.allocate(source.get_size());
    Perimortem::Core::Data::copy(copy, source.get_data(), source.get_size());
    return Perimortem::Core::View::Bytes(copy, source.get_size());
  }

  Perimortem::Memory::Allocator::Arena& arena;
  Ttx::Lexical::Errors& errors;
  Tetrodotoxin::Compiler::Program& program;
  Tetrodotoxin::Compiler::Engine& engine;
  Perimortem::Memory::Managed::Vector<Tetrodotoxin::Archiver::Terminal>&
      terminals;
};

}  // namespace Tetrodotoxin::Isa::Lowering
