// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "tetrodotoxin/linker/linker.hpp"
#include "tetrodotoxin/linker/object/symbol.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Compiler {

class Shader {
 public:
  explicit Shader(Perimortem::Memory::Allocator::Arena& arena)
      : arena(arena), error(arena) {}

  auto lower(Perimortem::Core::View::Bytes module, const Ttx::Type& root)
      -> Bool;
  auto add_to(Tetrodotoxin::Linker::Linker& linker) const -> void;

  constexpr auto get_error() const -> Perimortem::Core::View::Bytes {
    return error.get_view();
  }

 private:
  auto lower_stage(
      Perimortem::Core::View::Bytes module,
      const Ttx::Type& shader,
      const Ttx::Type& contract,
      const Ttx::Type::Function& stage) -> Bool;
  auto register_render_contracts(const Ttx::Type& root) -> void;
  auto register_render_contract(const Ttx::Type& render) -> void;
  auto find_contract(const Ttx::Type& shader) const -> const Ttx::Type*;
  auto symbol_name(
      Perimortem::Core::View::Bytes module,
      Perimortem::Core::View::Bytes shader,
      Perimortem::Core::View::Bytes stage) -> Perimortem::Core::View::Bytes;
  auto append_symbol_segment(
      Perimortem::Memory::Managed::Bytes& output,
      Perimortem::Core::View::Bytes value) -> void;
  auto set_error(Perimortem::Core::View::Bytes message) -> Bool;

  static auto type_attribute_equals(
      const Ttx::Type& type,
      Perimortem::Core::View::Bytes key,
      Perimortem::Core::View::Bytes value) -> Bool;

  Perimortem::Memory::Allocator::Arena& arena;
  Perimortem::Memory::Dynamic::Bytes read_only;
  Perimortem::Memory::Dynamic::Vector<Tetrodotoxin::Linker::Object::Symbol>
      symbols;
  Perimortem::Memory::Dynamic::
      Map<Perimortem::Core::View::Bytes, const Ttx::Type*>
          contracts;
  Perimortem::Memory::Managed::Bytes error;
};

}  // namespace Tetrodotoxin::Compiler
