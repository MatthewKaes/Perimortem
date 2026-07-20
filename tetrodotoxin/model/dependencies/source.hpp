// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/model/dependency.hpp"
#include "tetrodotoxin/model/source.hpp"

namespace Tetrodotoxin::Model::Dependencies {

// Source is a dependency found by a source selector such as a relative path.
// The resolved Source remains available for formatting, inspection, and
// incremental compilation, while the inherited binding names the Exports
// product created by interpreting that Source.
class Source final : public Model::Dependency {
 public:
  using ContractOwner = Source;
  static constexpr Perimortem::System::Uuid contract_id{
    0x5122d30e1fd14b97,
    0xa09a66f20e674ef8,
  };

  constexpr Source(
      const Model::Dialect& root_dialect,
      Perimortem::Core::View::Bytes path,
      const Model::Source& source,
      Perimortem::Core::View::Bytes local_name,
      const Ttx::Model::Exports& target,
      const Ttx::Concept::Documentation& documentation)
      : Dependency(root_dialect, local_name, target, documentation),
        path(path),
        source(source) {}

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Dependency::implements(requested);
  }

  constexpr auto get_path() const -> Perimortem::Core::View::Bytes {
    return path;
  }

  constexpr auto get_source() const -> const Model::Source& { return source; }

 private:
  Perimortem::Core::View::Bytes path;
  const Model::Source& source;
};

}  // namespace Tetrodotoxin::Model::Dependencies
