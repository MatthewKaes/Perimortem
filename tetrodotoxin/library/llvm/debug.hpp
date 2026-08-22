// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/language/definition.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Llvm {

// Debug owns authored source correlation for one Program transaction. Level
// selects policy while every retained LLVM metadata identity remains private to
// this owner.
class Debug {
 public:
  enum class Level : U8 {
    None,
    Line,
    Full,
  };

  constexpr Debug(Level level) : level(level) {}

  Debug(const Debug&) = delete;
  Debug(Debug&&) = delete;
  auto operator=(const Debug&) -> Debug& = delete;
  auto operator=(Debug&&) -> Debug& = delete;

  constexpr auto get_level() const -> Level { return level; }

  constexpr auto get_builder() const
      -> Perimortem::Core::Option<LLVMOpaqueDIBuilder&> {
    return builder;
  }

  constexpr auto get_file() const
      -> Perimortem::Core::Option<LLVMOpaqueMetadata&> {
    return file;
  }

  auto initialize(
      Ttx::Concept::Abstract& program,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source_text) -> Bool;

  auto release() -> void;

  auto finalize(Ttx::Concept::Abstract& program) -> Bool;

  auto find_type(const Ttx::Model::Type& type) const
      -> Perimortem::Core::Option<LLVMMetadataRef>;

  auto publish_type(const Ttx::Model::Type& type, LLVMMetadataRef metadata)
      -> Bool;

  auto replace_type(const Ttx::Model::Type& type, LLVMMetadataRef metadata)
      -> Bool;

  auto find_payload(const Ttx::Model::Type& type) const
      -> Perimortem::Core::Option<LLVMMetadataRef>;

  auto publish_payload(const Ttx::Model::Type& type, LLVMMetadataRef metadata)
      -> Bool;

  auto replace_payload(const Ttx::Model::Type& type, LLVMMetadataRef metadata)
      -> Bool;

  auto publish_enumerator(
      const Ttx::Model::Type& type,
      LLVMMetadataRef metadata) -> Bool;

  auto get_enumerators(const Ttx::Model::Type& type) const
      -> Perimortem::Core::View::Vector<LLVMMetadataRef>;

  auto find_scope(const Ttx::Model::Type& type) const
      -> Perimortem::Core::Option<LLVMMetadataRef>;

  auto publish_scope(const Ttx::Model::Type& type, LLVMMetadataRef metadata)
      -> Bool;

  auto replace_scope(const Ttx::Model::Type& type, LLVMMetadataRef metadata)
      -> Bool;

  auto get_scope_types() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Type>>;

  auto publish_member(const Ttx::Model::Type& type, LLVMMetadataRef metadata)
      -> Bool;

  auto get_members(const Ttx::Model::Type& type) const
      -> Perimortem::Core::View::Vector<LLVMMetadataRef>;

  auto type(
      const Ttx::Model::Type& type,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor) -> Bool;

  auto field(const Ttx::Model::Addressable& field, Ttx::Lexical::Anchor anchor)
      -> Bool;

  auto signed_enumerator(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Type& type,
      const Ttx::Concept::Abstract& enumerator,
      S64 value) -> Bool;

  auto unsigned_enumerator(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Type& type,
      const Ttx::Concept::Abstract& enumerator,
      U64 value) -> Bool;

  auto global(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Addressable& addressable,
      const Tetrodotoxin::Language::Definition& definition,
      Bool local,
      Bool defined) -> Bool;

  auto begin_function(
      Ttx::Concept::Abstract& body,
      const Ttx::Model::Callable& callable,
      const Tetrodotoxin::Language::Definition& definition) -> Bool;

  auto parameter(
      Ttx::Concept::Abstract& body,
      const Ttx::Model::Addressable& parameter,
      Ttx::Lexical::Anchor anchor,
      Count index) -> Bool;

  auto end_function(Ttx::Concept::Abstract& body) -> Bool;

  auto begin_block(
      Ttx::Concept::Abstract& body,
      const Ttx::Concept::Abstract& block,
      Ttx::Lexical::Anchor anchor) -> Bool;

  auto statement(Ttx::Concept::Abstract& body, Ttx::Lexical::Anchor anchor)
      -> Bool;

  auto local(
      Ttx::Concept::Abstract& body,
      const Ttx::Model::Addressable& local,
      Ttx::Lexical::Anchor anchor) -> Bool;

  auto value(
      Ttx::Concept::Abstract& body,
      const Ttx::Model::Addressable& local,
      Ttx::Lexical::Anchor anchor,
      LLVMValueRef value) -> Bool;

 private:
  Level level;
  Perimortem::Core::Option<LLVMOpaqueDIBuilder&> builder;
  Perimortem::Core::Option<LLVMOpaqueMetadata&> file;
  Perimortem::Memory::Dynamic::Map<const Ttx::Model::Type*, LLVMMetadataRef>
      types;
  Perimortem::Memory::Dynamic::Map<const Ttx::Model::Type*, LLVMMetadataRef>
      payloads;
  Perimortem::Memory::Dynamic::Map<
      const Ttx::Model::Type*,
      Perimortem::Memory::Dynamic::Vector<LLVMMetadataRef>>
      enumerators;
  Perimortem::Memory::Dynamic::Map<const Ttx::Model::Type*, LLVMMetadataRef>
      scopes;
  Perimortem::Memory::Dynamic::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Type>>
      scope_types;
  Perimortem::Memory::Dynamic::Map<
      const Ttx::Model::Type*,
      Perimortem::Memory::Dynamic::Vector<LLVMMetadataRef>>
      members;
  Bool released = False;
};

}  // namespace Tetrodotoxin::Library::Llvm
