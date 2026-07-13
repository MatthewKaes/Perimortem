// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/puffer/resolution/context.hpp"
#include "tetrodotoxin/puffer/resolution/resolver.hpp"
#include "tetrodotoxin/puffer/toolchain.hpp"
#include "tetrodotoxin/terminal/plan.hpp"
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Puffer {

// Coordinates source resolution, terminal lowering, and package serialization.
// Source records are transaction inputs; build returns every derived artifact
// to the caller.
class Compiler {
 public:
  enum class Mode {
    Library,
    Package,
  };

  Compiler(Mode mode, Perimortem::Core::View::Bytes package_name);

  auto add_dependency(Perimortem::Core::View::Bytes path) -> Bool;
  auto add_source(Perimortem::Core::View::Bytes path) -> Bool;
  auto build(
      Perimortem::Core::View::Bytes object_name,
      Perimortem::Memory::Dynamic::Bytes& archive,
      Perimortem::Memory::Dynamic::Bytes& header,
      Perimortem::Memory::Dynamic::Bytes& puffer_buffer) -> Bool;
  constexpr auto get_errors() const
      -> Perimortem::Core::View::Vector<Ttx::Lexical::Errors::Error> {
    auto resolution_errors = resolution.get_errors();
    return resolution_errors.is_empty() ? errors.get_view() : resolution_errors;
  }

 private:
  using Record = Resolution::Source::Record;

  static auto is_package_root(Perimortem::Core::View::Bytes path) -> Bool;
  static auto is_puffer_buffer(Perimortem::Core::View::Bytes path) -> Bool;

  auto add_record(Record& record) -> void;
  auto report(
      Perimortem::Core::View::Bytes path,
      Perimortem::Core::View::Bytes message) -> Bool;

  Perimortem::Memory::Allocator::Arena arena;
  Ttx::Lexical::Errors errors;
  Tetrodotoxin::Isa::Registry isa_registry;
  Resolution::Resolver resolver;
  Resolution::Context resolution;
  Tetrodotoxin::Terminal::Plan terminals;
  Perimortem::Memory::Dynamic::Vector<Record*> records;
  Record* package_root = nullptr;
  Mode mode;
};

}  // namespace Tetrodotoxin::Puffer
