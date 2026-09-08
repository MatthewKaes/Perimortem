// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/dialect.hpp"
#include "ttx/concept/reference.hpp"

namespace Tetrodotoxin::Environment {

// Toolchain owns the installed family of Tetrodotoxin languages. Every
// Workspace can borrow the same immutable Dialect identities, while the
// semantic objects produced through process stay with the Toolchain. Workspace
// can still own its independent source transactions. Installed Dialects outlive
// every source processed here.
class Toolchain {
 public:
  Toolchain();
  ~Toolchain();

  Toolchain(const Toolchain&) = delete;
  Toolchain(Toolchain&&) = delete;
  auto operator=(const Toolchain&) -> Toolchain& = delete;
  auto operator=(Toolchain&&) -> Toolchain& = delete;

  // Adds the Dialect to be useable by the toolchain under it's provided name.
  // Returns false if a Dialect is already registered with that name.
  auto install(Language::Dialect& dialect) -> Bool {
    BAIL_IF(contains_name(dialect.get_name()));

    dialects.insert(dialect);
    return true;
  }

  auto find(Perimortem::Core::View::Bytes name) const
      -> Perimortem::Core::Option<Language::Dialect&>;

  // Reads one source and dispatches its body to the named Dialect if installed.
  // The toolchain manages the the life time of the returned Monograph and its
  // source bytes from disk.
  //
  // While `process` is how most Dialects drive their actual side effects, only
  // the side effects reachable from Monograph are exposed to invokers of the
  // toolchain. Communicating via memory side channels or other means is allowed
  // but it's considered undefined behavior as far as Tetrodotoxin is concerned.
  auto process(
      Perimortem::Core::View::Bytes source,
      Ttx::Lexical::Errors& errors)
      -> Perimortem::Core::Option<Language::Monograph&>;

  constexpr auto get_dialects() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<Language::Dialect>> {
    return dialects;
  }

 private:
  auto contains_name(Perimortem::Core::View::Bytes name) const -> Bool;

  Perimortem::Memory::Allocator::Arena arena;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Language::Dialect>>
      dialects;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Language::Monograph>>
      sources;
};

}  // namespace Tetrodotoxin::Environment
