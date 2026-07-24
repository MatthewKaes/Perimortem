// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/concept/package/resolution.hpp"
#include "ttx/concept/documentation.hpp"

namespace Tetrodotoxin::Ttx::Model::Package {

// Source is the model resolved from one root package.ttx document. It retains
// only the authored package facts needed by construction; Cursor state,
// filesystem capabilities, opened paths, and partially built semantic owners
// remain on the parser transaction.
class Source {
 public:
  constexpr Source(
      const Ttx::Concept::Documentation& documentation,
      Perimortem::Core::View::Vector<Concept::Package::Resolution> resolutions,
      Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> members)
      : documentation(documentation),
        resolutions(resolutions),
        members(members) {}

  constexpr auto get_documentation() const
      -> const Ttx::Concept::Documentation& {
    return documentation;
  }

  constexpr auto get_resolutions() const
      -> Perimortem::Core::View::Vector<Concept::Package::Resolution> {
    return resolutions;
  }

  // A member is already its normalized package-relative route. Another wrapper
  // would add neither identity nor behavior to this model.
  constexpr auto get_members() const
      -> Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> {
    return members;
  }

 private:
  const Ttx::Concept::Documentation& documentation;
  Perimortem::Core::View::Vector<Concept::Package::Resolution> resolutions;
  Perimortem::Core::View::Vector<Perimortem::Core::View::Bytes> members;
};

}  // namespace Tetrodotoxin::Ttx::Model::Package
