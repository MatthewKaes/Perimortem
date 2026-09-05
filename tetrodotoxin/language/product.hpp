// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/artifact_file.hpp"

namespace Tetrodotoxin::Language {

// Product is one immutable named byte result produced after graph completion.
// Its relative name is publication intent rather than semantic identity inside
// the source graph, and its bytes remain valid for the result Arena lifetime.
class Product : public ArtifactFile {
 public:

  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Bytes value,
      Bool executable = False) -> Product&;

  TTX_NAME(name);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto get_value() const -> Perimortem::Core::View::Bytes {
    return value;
  }

  constexpr auto get_artifact_route() const
      -> Perimortem::Core::View::Bytes override {
    return name;
  }

  constexpr auto get_artifact_bytes() const
      -> Perimortem::Core::View::Bytes override {
    return value;
  }

  constexpr auto is_executable() const -> Bool override { return executable; }

 private:
  Product(
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::View::Bytes value,
      Bool executable);

  Perimortem::Core::View::Bytes name;
  Perimortem::Core::View::Bytes value;
  Bool executable;
};

}  // namespace Tetrodotoxin::Language
