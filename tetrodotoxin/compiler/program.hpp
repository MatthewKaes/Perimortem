// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/compiler/execution/function.hpp"

namespace Tetrodotoxin::Compiler {

// Program is the complete immutable input presented to a compiler backend.
// It joins executable bodies to their internal symbols and carries the public
// ABI projections emitted beside the native artifact. Execution still owns
// function bodies and operations. Program owns the boundary where those bodies
// become one target product.
//
// Ordered vectors define publication order for backends and generated headers.
// The adjacent indices accelerate construction without making map iteration an
// output contract.
class Program {
 public:
  auto define(
      Ttx::Lexical::Source source,
      Perimortem::Core::View::Bytes symbol,
      const Ttx::Function& function,
      const Execution::Body& body) -> Bool;
  auto expose(Tetrodotoxin::Abi::Type type) -> Bool;
  auto expose(const Tetrodotoxin::Abi::Export& export_) -> Bool;

  auto find(const Ttx::Function& function) const -> const Execution::Function*;
  auto find_export(const Ttx::Function& function) const
      -> const Tetrodotoxin::Abi::Export*;
  auto resolve_symbol(Perimortem::Core::View::Bytes symbol) const
      -> Perimortem::Core::View::Bytes;

  constexpr auto get_functions() const
      -> Perimortem::Core::View::Vector<Execution::Function> {
    return functions;
  }

  constexpr auto get_types() const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Abi::Type> {
    return types;
  }

  constexpr auto get_exports() const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Abi::Export> {
    return exports;
  }

 private:
  Perimortem::Memory::Dynamic::Vector<Execution::Function> functions;
  Perimortem::Memory::Dynamic::Map<const Ttx::Function*, Count>
      function_indices;
  Perimortem::Memory::Dynamic::Map<Perimortem::Core::View::Bytes, Count>
      function_symbols;
  Perimortem::Memory::Dynamic::Vector<Tetrodotoxin::Abi::Type> types;
  Perimortem::Memory::Dynamic::Map<Perimortem::Core::View::Bytes, Count>
      type_paths;
  Perimortem::Memory::Dynamic::Vector<Tetrodotoxin::Abi::Export> exports;
  Perimortem::Memory::Dynamic::Map<const Ttx::Function*, Count> export_indices;
  Perimortem::Memory::Dynamic::
      Map<Perimortem::Core::View::Bytes, Perimortem::Core::View::Bytes>
          exported_symbols;
};

}  // namespace Tetrodotoxin::Compiler
