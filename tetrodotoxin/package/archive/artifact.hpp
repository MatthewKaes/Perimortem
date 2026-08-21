// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/linker/fingerprint.hpp"
#include "tetrodotoxin/linker/import.hpp"

namespace Tetrodotoxin::Package::Archive {

// Artifact connects one logical native product to its target ABI fingerprint
// and selected native imports. Package retains these locators while Linker owns
// the manifest and native bytes that must reproduce the same agreement.
class Artifact {
 public:
  constexpr Artifact(
      Perimortem::Core::View::Bytes id,
      Perimortem::Core::View::Bytes target,
      Tetrodotoxin::Linker::Fingerprint fingerprint,
      Perimortem::Core::View::Vector<Tetrodotoxin::Linker::Import> imports = {})
      : id(id), target(target), fingerprint(fingerprint), imports(imports) {}

  constexpr auto get_id() const -> Perimortem::Core::View::Bytes { return id; }

  constexpr auto get_target() const -> Perimortem::Core::View::Bytes {
    return target;
  }

  constexpr auto get_fingerprint() const -> Tetrodotoxin::Linker::Fingerprint {
    return fingerprint;
  }

  constexpr auto get_imports() const
      -> Perimortem::Core::View::Vector<Tetrodotoxin::Linker::Import> {
    return imports;
  }

 private:
  Perimortem::Core::View::Bytes id;
  Perimortem::Core::View::Bytes target;
  Tetrodotoxin::Linker::Fingerprint fingerprint;
  Perimortem::Core::View::Vector<Tetrodotoxin::Linker::Import> imports;
};

}  // namespace Tetrodotoxin::Package::Archive
