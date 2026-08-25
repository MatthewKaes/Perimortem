// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/language/persistence/profile.hpp"
#include "tetrodotoxin/scene/language/monograph.hpp"

namespace Tetrodotoxin::Scene::Archive {

// Writer keeps Scene relationships around one opaque Library payload. Library
// owns the source context and Scene instance declarations, while Scene records
// the Signals and lifecycle edges that give those Functions application
// meaning.
class Writer {
 public:
  static auto encode(
      const Tetrodotoxin::Scene::Language::Monograph& monograph,
      Tetrodotoxin::Language::Persistence::Profile profile)
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>;

 private:
  Writer(Tetrodotoxin::Language::Persistence::Profile profile);

  auto write(U8 value) -> void;
  auto write(U16 value) -> void;
  auto write(U32 value) -> void;
  auto write(Perimortem::Core::View::Bytes value) -> Bool;
  auto write(const Ttx::Concept::Documentation& documentation) -> Bool;

  Perimortem::Memory::Dynamic::Bytes bytes;
};

}  // namespace Tetrodotoxin::Scene::Archive
