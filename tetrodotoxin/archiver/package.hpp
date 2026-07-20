// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/abi/linkage.hpp"
#include "tetrodotoxin/archiver/manifest.hpp"
#include "tetrodotoxin/archiver/terminal.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Archiver {

// Legacy Puffer Buffer view retained while the old format is replaced.
//
// This is not the canonical package contract. New consumers use
// Tetrodotoxin::Model::Package, and the replacement reader reconstructs
// Model::Packages::Precompiled. Do not adapt Source or Model::Package into this
// root-Type representation merely to preserve the old format.
//
// Resolution owns how source files become a type tree. Package owns how that
// tree, function linkage, and terminal byte artifacts appear after a
// Puffer Buffer is restored. Its Manifest is required package identity and
// dependency data. External packages are restore inputs only and are not
// retained by Package.
//
// Package does not own an arena. Reader constructs it from caller-owned
// storage, leaving the cache or resolver to decide how long the snapshot lives.
class Package {
 public:
  constexpr Package(
      Manifest manifest,
      const Ttx::Type& root_type,
      Perimortem::Core::View::Vector<const Ttx::Type*> types,
      Perimortem::Core::View::Vector<Terminal> terminals,
      Perimortem::Core::View::Vector<Tetrodotoxin::Abi::Linkage> linkages = {})
      : manifest(manifest),
        root_type(root_type),
        types(types),
        terminals(terminals),
        linkages(linkages) {}

  constexpr auto get_manifest() const -> const Manifest& { return manifest; }
  constexpr auto find_terminal(
      Perimortem::Core::View::Bytes group,
      Perimortem::Core::View::Bytes path) const
      -> Perimortem::Core::View::Bytes {
    for (Count i = 0; i < terminals.get_size(); i++) {
      if (terminals[i].get_group() == group &&
          terminals[i].get_path() == path) {
        return terminals[i].get_content();
      }
    }

    return Perimortem::Core::View::Bytes();
  }

  constexpr auto get_type() const -> const Ttx::Type& { return root_type; }
  constexpr auto get_types() const
      -> Perimortem::Core::View::Vector<const Ttx::Type*> {
    return types;
  }

  constexpr auto get_terminals() const
      -> Perimortem::Core::View::Vector<Terminal> {
    return terminals;
  }

  constexpr auto get_linkages() const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Abi::Linkage> {
    return linkages;
  }

 private:
  Manifest manifest;
  const Ttx::Type& root_type;
  Perimortem::Core::View::Vector<const Ttx::Type*> types;
  Perimortem::Core::View::Vector<Terminal> terminals;
  Perimortem::Core::View::Vector<Tetrodotoxin::Abi::Linkage> linkages;
};

}  // namespace Tetrodotoxin::Archiver
