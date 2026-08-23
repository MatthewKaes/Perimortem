// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/result.hpp"

#include "backend/llvm/abi/export.hpp"
#include "backend/llvm/abi/publication.hpp"
#include "backend/llvm/abi/unit.hpp"
#include "backend/llvm/failure.hpp"
#include "backend/llvm/products.hpp"
#include "backend/llvm/representation/carriers.hpp"
#include "backend/llvm/representation/debug.hpp"
#include "backend/llvm/representation/emission.hpp"
#include "backend/llvm/representation/functions.hpp"
#include "backend/llvm/representation/globals.hpp"
#include "backend/llvm/target.hpp"
#include "llvm-c/Types.h"
#include "tetrodotoxin/linker/import.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Backend::Llvm::Representation {

// Program owns one LLVM module transaction and publishes Terminal bytes only
// after backend traversal and verification complete.
class Program : public Emission {
 public:
  Program(
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source_text,
      Target target,
      Debug::Level debug_level,
      Abi::Unit unit);

  ~Program();

  auto initialize() -> Bool;

  // Creates one hosted process entry that invokes an already published native
  // symbol with empty parameter and result shapes exactly once and then returns
  // success to the host.
  auto create_process_entry(Perimortem::Core::View::Bytes symbol) -> Bool;

  auto compile() -> Perimortem::Utility::Result<Products, Failure>;

  constexpr auto get_arena() const -> Perimortem::Memory::Allocator::Arena& {
    return arena;
  }

  constexpr auto get_target() const -> Target { return target; }

  constexpr auto get_debug() -> Debug& { return debug; }

  constexpr auto get_debug() const -> const Debug& { return debug; }

  constexpr auto get_context() -> LLVMOpaqueContext& { return context; }

  constexpr auto get_module() -> LLVMOpaqueModule& { return module; }

  constexpr auto get_carriers() const -> const Carriers& { return carriers; }

  constexpr auto get_functions() const -> const Functions& { return functions; }

  constexpr auto get_globals() const -> const Globals& { return globals; }

  constexpr auto get_unit() const -> const Abi::Unit& { return unit; }

  auto add_export(Abi::Export value) -> void;

  auto add_publication(Abi::Publication value) -> void;

  auto add_import(Tetrodotoxin::Linker::Import value) -> Bool;

  auto fail_backend(Perimortem::Core::View::Bytes message) -> Bool;

  auto fail_source(
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) -> Bool;

  constexpr auto has_source_failure() const -> Bool {
    return Bool(failures & U8(FailureFlag::Source));
  }

  constexpr auto has_tool_failure() const -> Bool {
    return Bool(failures & U8(FailureFlag::Tool));
  }

 private:
  enum class FailureFlag : U8 {
    Source = 1,
    Tool = 2,
  };

  Perimortem::Memory::Allocator::Arena& arena;
  Ttx::Lexical::Errors& errors;
  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Bytes source_text;
  Target target;
  Abi::Unit unit;
  Debug debug;
  LLVMOpaqueContext& context;
  LLVMOpaqueModule& module;
  Perimortem::Core::Option<U8&> target_machine;
  Carriers carriers;
  Functions functions;
  Globals globals;
  Perimortem::Memory::Managed::Vector<Abi::Export> exports;
  Perimortem::Memory::Managed::Vector<Abi::Publication> publications;
  Perimortem::Memory::Managed::Vector<Tetrodotoxin::Linker::Import> imports;
  U8 failures = 0;
};

}  // namespace Tetrodotoxin::Backend::Llvm::Representation
