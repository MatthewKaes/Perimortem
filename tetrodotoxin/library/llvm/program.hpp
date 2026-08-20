// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/result.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/library/llvm/carriers.hpp"
#include "tetrodotoxin/library/llvm/debug.hpp"
#include "tetrodotoxin/library/llvm/export.hpp"
#include "tetrodotoxin/library/llvm/failure.hpp"
#include "tetrodotoxin/library/llvm/functions.hpp"
#include "tetrodotoxin/library/llvm/globals.hpp"
#include "tetrodotoxin/library/llvm/products.hpp"
#include "tetrodotoxin/library/llvm/publication.hpp"
#include "tetrodotoxin/library/llvm/target.hpp"
#include "tetrodotoxin/library/llvm/unit.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Library::Llvm {

// Program owns one LLVM module transaction and its direct target Builder.
// Library lowering contributes through that Builder before compile publishes
// any Terminal bytes.
class Program : public Ttx::Concept::Abstract {
 public:
  TTX_CONTRACT(Program, Ttx::Concept::Abstract);

  Program(
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source_text,
      Target target,
      Debug::Level debug_level,
      Unit unit);

  ~Program();

  auto get_name() const -> Perimortem::Core::View::Bytes override;

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

  auto initialize() -> Bool;

  // Creates one hosted process entry that invokes an already published native
  // `[] -> []` symbol exactly once and then returns success to the host.
  auto create_process_entry(Perimortem::Core::View::Bytes symbol) -> Bool;

  auto compile() -> Perimortem::Utility::Result<Products, Failure>;

  auto resolve_context(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract& override;

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

  constexpr auto get_unit() const -> const Unit& { return unit; }

  auto add_export(Export value) -> void;

  auto add_publication(Publication value) -> void;

  auto fail_backend(Perimortem::Core::View::Bytes message) -> Bool;

  auto fail_source(
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor,
      Perimortem::Core::View::Bytes message,
      Perimortem::Core::View::Bytes hint = {}) -> Bool;

  constexpr auto has_source_failure() const -> Bool { return source_failed; }

  constexpr auto has_tool_failure() const -> Bool { return tool_failed; }

 private:
  Perimortem::Memory::Allocator::Arena& arena;
  Ttx::Lexical::Errors& errors;
  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Bytes source_text;
  Target target;
  Unit unit;
  Debug debug;
  LLVMOpaqueContext& context;
  LLVMOpaqueModule& module;
  Perimortem::Core::Option<Unsigned_8&> target_machine;
  Carriers carriers;
  Functions functions;
  Globals globals;
  Perimortem::Memory::Managed::Vector<Export> exports;
  Perimortem::Memory::Managed::Vector<Publication> publications;
  Bool source_failed = False;
  Bool tool_failed = False;
};

}  // namespace Tetrodotoxin::Library::Llvm
