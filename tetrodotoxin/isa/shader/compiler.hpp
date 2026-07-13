// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/compiler/symbol.hpp"
#include "tetrodotoxin/isa/base/implementation.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Shader {

// Compiler is Shader's terminal state machine for SPIR-V stage modules.
//
// It lives with the Shader ISA because Shader owns stage legality and body
// meaning. Its direct SPIR-V assembler dependency is temporary: Shader should
// emit a typed graphics execution program and let the toolchain select SPIR-V
// or another backend, just as Library now does for host execution.
// The resolved render contract remains a nested
// `Contract` alias, so lowering consumes one resolved type tree without relying
// on Render records being lowered first.
class Compiler {
 public:
  Compiler() = default;

  auto lower(
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Lexical::Errors& errors,
      Ttx::Lexical::Source source,
      Perimortem::Core::View::Bytes module,
      const Ttx::Type& root,
      const Tetrodotoxin::Isa::Base::Implementation& implementation) -> Bool;

  constexpr auto get_read_only() const -> Perimortem::Core::View::Bytes {
    return read_only;
  }

  constexpr auto get_stages() const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Compiler::Symbol> {
    return stages;
  }

 private:
  auto lower_stage(
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Lexical::Errors& errors,
      Ttx::Lexical::Source source,
      Perimortem::Core::View::Bytes module,
      const Ttx::Type& shader,
      const Ttx::Type& contract,
      const Ttx::Function& stage,
      const Tetrodotoxin::Isa::Base::Implementation& implementation) -> Bool;
  auto stage_name(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes module,
      Perimortem::Core::View::Bytes shader,
      Perimortem::Core::View::Bytes stage) -> Perimortem::Core::View::Bytes;
  auto append_name_segment(
      Perimortem::Memory::Managed::Bytes& output,
      Perimortem::Core::View::Bytes value) -> void;

  Perimortem::Memory::Dynamic::Bytes read_only;
  Perimortem::Memory::Dynamic::Vector<Tetrodotoxin::Compiler::Symbol> stages;
};

}  // namespace Tetrodotoxin::Isa::Shader
