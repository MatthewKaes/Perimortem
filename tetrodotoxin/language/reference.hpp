// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/bootstrap/model/alias.hpp"

namespace Tetrodotoxin::Language {

// Reference is one delayed authored query. It retains its stable host and asks
// that host for the same name whenever the represented identity is requested.
class Reference : public Ttx::Model::Alias {
 public:
  TTX_CONTRACT(Reference, Ttx::Model::Alias);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes name) -> Reference&;

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  constexpr auto get_host() const -> const Ttx::Concept::Abstract& {
    return *host;
  }

 private:
  constexpr Reference(
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes name)
      : Ttx::Model::Alias(name, Ttx::Concept::Documentation::get_empty()),
        host(&host),
        name(name) {}

  const Ttx::Concept::Abstract* host;
  Perimortem::Core::View::Bytes name;
};

}  // namespace Tetrodotoxin::Language
