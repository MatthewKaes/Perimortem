// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/package/archive/archive.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"

namespace Tetrodotoxin::Package::Archive {

// Emits one canonical Package Archive from a validated value. Resource free
// values preserve Format 2, while a Package that selected Resources uses
// Format 3 and carries each route and byte value once. Writer measures the
// complete envelope before allocation and preserves every supplied list order.
class Writer {
 public:
  class GraphMember {
   public:
    constexpr GraphMember(
        Perimortem::Core::View::Bytes name,
        const Tetrodotoxin::Language::Monograph& monograph)
        : name(name), monograph(monograph) {}

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }
    constexpr auto get_monograph() const
        -> const Tetrodotoxin::Language::Monograph& {
      return monograph;
    }

   private:
    Perimortem::Core::View::Bytes name;
    const Tetrodotoxin::Language::Monograph& monograph;
  };

  Writer() = delete;

  // Writes every required section in canonical order. A body that exceeds the
  // unsigned 32 bit envelope limit logs a warning. Failure to reach the
  // measured boundary logs an error.
  static auto write(const Archive& archive)
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>;

  static auto write(
      const Package::Language::Monograph& package,
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version,
      Tetrodotoxin::Language::Persistence::Profile profile,
      Perimortem::Core::View::Vector<GraphMember> members,
      Perimortem::Core::View::Vector<GraphImport> imports,
      Perimortem::Core::View::Vector<Artifact> artifacts = {},
      Perimortem::Core::View::Vector<Export> exports = {})
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes>;
};

}  // namespace Tetrodotoxin::Package::Archive
