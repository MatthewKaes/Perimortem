// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/bootstrap/model/addressable.hpp"

namespace Ttx::Model::Layouts {

// Addressable is one named Layout slot with an exact value Type. The Layout
// remains the owner of source order, attributes, and authored location; this
// identity exists only so graph consumers can refer to that real slot.
class Addressable : public Ttx::Model::Addressable {
 public:
  TTX_CONTRACT(Addressable, Ttx::Model::Addressable);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& type) -> Perimortem::Core::Option<Addressable&>;

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& type) -> Addressable&;

  Addressable(const Addressable&) = delete;
  Addressable(Addressable&&) = delete;
  auto operator=(const Addressable&) -> Addressable& = delete;
  auto operator=(Addressable&&) -> Addressable& = delete;

  TTX_NAME(name);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto get_type() const -> const Ttx::Model::Type& override {
    return type;
  }

 private:
  constexpr Addressable(
      Perimortem::Core::View::Bytes name,
      const Ttx::Model::Type& type)
      : name(name), type(type) {}

  Perimortem::Core::View::Bytes name;
  const Ttx::Model::Type& type;
};

}  // namespace Ttx::Model::Layouts
