// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "backend/llvm/representation/program.hpp"
#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"

namespace Tetrodotoxin::Backend::Llvm::Lowering {

// Types owns target carrier preparation for exact Library identities. The
// semantic graph supplies category, width, Layout, and declaration facts while
// this backend owns every physical representation and completion record.
class Types {
 public:
  static auto prepare(
      Representation::Program& program,
      const Tetrodotoxin::Library::Language::Model::Type& type) -> Bool;

  static auto prepare(
      Representation::Program& program,
      const Tetrodotoxin::Library::Language::Model::Addressable& addressable)
      -> Bool;

  static auto prepare(
      Representation::Program& program,
      const Tetrodotoxin::Library::Language::Model::Callable& callable) -> Bool;

  static auto reserve(
      Representation::Program& program,
      const Tetrodotoxin::Library::Language::Model::Addressable& addressable)
      -> Bool;

  static auto complete(
      Representation::Program& program,
      const Tetrodotoxin::Library::Language::Model::Addressable& addressable)
      -> Bool;

  static auto reserve(
      Representation::Program& program,
      const Tetrodotoxin::Library::Language::Model::Callable& callable) -> Bool;

  static auto complete(
      Representation::Program& program,
      const Tetrodotoxin::Library::Language::Model::Callable& callable) -> Bool;

  static auto reserve_declaration(
      Representation::Program& program,
      const Tetrodotoxin::Library::Language::Model::Type& type) -> Bool;

  static auto complete_declaration(
      Representation::Program& program,
      const Tetrodotoxin::Library::Language::Model::Type& type) -> Bool;
};

}  // namespace Tetrodotoxin::Backend::Llvm::Lowering
