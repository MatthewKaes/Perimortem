// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/compiler/context.hpp"
#include "tetrodotoxin/compiler/symbol/stage.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Compiler {

// Shader lowers Render/Shader type facts into read-only SPIR-V stage modules.
//
// Render records register pipeline contracts; Shader records then emit stage
// blobs that satisfy those contracts. The linker later packages the emitted
// blobs as ordinary read-only data.
class Shader {
 public:
  Shader() = default;

  auto lower(
      Context& context,
      Perimortem::Core::View::Bytes module,
      const Ttx::Type& root) -> Bool;

  constexpr auto get_read_only() const -> Perimortem::Core::View::Bytes {
    return read_only;
  }
  constexpr auto get_stages() const
      -> Perimortem::Core::View::Vector<Symbol::Stage> {
    return stages;
  }

 private:
  auto lower_stage(
      Context& context,
      Perimortem::Core::View::Bytes module,
      const Ttx::Type& shader,
      const Ttx::Type& contract,
      const Ttx::Type::Function& stage) -> Bool;
  auto register_render_contracts(const Ttx::Type& root) -> void;
  auto register_render_contract(const Ttx::Type& render) -> void;
  auto find_contract(const Ttx::Type& shader) const -> const Ttx::Type*;
  auto stage_name(
      Context& context,
      Perimortem::Core::View::Bytes module,
      Perimortem::Core::View::Bytes shader,
      Perimortem::Core::View::Bytes stage) -> Perimortem::Core::View::Bytes;
  auto append_name_segment(
      Perimortem::Memory::Managed::Bytes& output,
      Perimortem::Core::View::Bytes value) -> void;

  Perimortem::Memory::Dynamic::Bytes read_only;
  Perimortem::Memory::Dynamic::Vector<Symbol::Stage> stages;
  Perimortem::Memory::Dynamic::
      Map<Perimortem::Core::View::Bytes, const Ttx::Type*>
          contracts;
};

}  // namespace Tetrodotoxin::Compiler
