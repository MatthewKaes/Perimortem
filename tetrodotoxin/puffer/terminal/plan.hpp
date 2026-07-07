// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/set.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/utility/pair.hpp"

#include "tetrodotoxin/compiler/library.hpp"
#include "tetrodotoxin/compiler/shader.hpp"
#include "tetrodotoxin/puffer/resolution/resolver.hpp"
#include "tetrodotoxin/puffer/resolution/source/record.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Puffer::Terminal {

// Plan is the terminal lowering transaction for one Puffer invocation.
//
// Resolution owns source loading and type graph construction. Plan starts after
// that point: it selects the resolved records that should produce terminal
// artifacts, orders them so Render contracts reach Shader lowering first,
// feeds the compiler owners, and records the Puffer Buffer facts for the same
// source set. The linker consumes the compiler outputs after the plan lowers.
//
// TODO: Puffer Buffer emission needs to become a real Tetrodotoxin snapshot
// format, not a text fact dump. The next resolver slice should let package deps
// inject `.puffer` buffers so Resolver can hydrate their public TTX type trees
// without re-reading or re-evaluating the package source files.
class Plan {
 public:
  explicit Plan(Perimortem::Memory::Allocator::Arena& arena)
      : arena(arena), compiler_errors(arena) {}

  auto add_record(Resolution::Source::Record& record) -> void;
  auto add_package(
      Resolution::Resolver& resolver,
      Resolution::Source::Record& root) -> void;
  auto lower() -> Bool;

  constexpr auto is_empty() const -> Bool { return records.get_size() == 0; }
  constexpr auto get_library() const
      -> const Tetrodotoxin::Compiler::Library& {
    return library_compiler;
  }
  constexpr auto get_shader() const -> const Tetrodotoxin::Compiler::Shader& {
    return shader_compiler;
  }
  constexpr auto get_puffer_buffer() const -> Perimortem::Core::View::Bytes {
    return puffer_buffer;
  }
  constexpr auto get_errors() const
      -> Perimortem::Core::View::Vector<Ttx::Lexical::Errors::Error> {
    return compiler_errors;
  }

 private:
  using Record = Tetrodotoxin::Puffer::Resolution::Source::Record;

  enum class RecordKind {
    Package,
    Library,
    Render,
    Shader,
    Other,
  };

  static constexpr Perimortem::Core::Static::Vector<
      Perimortem::Utility::Pair<Perimortem::Core::View::Bytes, RecordKind>,
      4>
      record_kinds = {{
        {"Package"_view, RecordKind::Package},
        {"Library"_view, RecordKind::Library},
        {"Render"_view, RecordKind::Render},
        {"Shader"_view, RecordKind::Shader},
      }};

  auto lower_kind(RecordKind kind) -> Bool;
  auto lower_record(Record& record, RecordKind kind) -> Bool;
  auto build_module_name(Perimortem::Core::View::Bytes source_path)
      -> Perimortem::Core::View::Bytes;
  auto append_terminal(Record& record) -> void;
  auto append_field(
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Bytes value) -> void;
  auto append_type_facts(
      const Ttx::Type& type,
      Perimortem::Core::View::Bytes path) -> void;
  auto append_member_fact(
      Perimortem::Core::View::Bytes owner,
      const Ttx::Type::Member& member) -> void;
  auto append_function_fact(
      Perimortem::Core::View::Bytes owner,
      const Ttx::Type::Function& function) -> void;

  static auto classify(const Record& record) -> RecordKind;
  static auto type_name(const Ttx::Type* type) -> Perimortem::Core::View::Bytes;
  static auto append_decimal(
      Perimortem::Memory::Dynamic::Bytes& output,
      Count value) -> void;

  Perimortem::Memory::Allocator::Arena& arena;
  Ttx::Lexical::Errors compiler_errors;
  Tetrodotoxin::Compiler::Library library_compiler;
  Tetrodotoxin::Compiler::Shader shader_compiler;
  Perimortem::Memory::Dynamic::Set<Record*> record_set;
  Perimortem::Memory::Dynamic::Vector<Record*> records;
  Perimortem::Memory::Dynamic::Bytes puffer_buffer;
};

}  // namespace Tetrodotoxin::Puffer::Terminal
