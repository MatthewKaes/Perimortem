// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/model/dependency.hpp"
#include "tetrodotoxin/model/package.hpp"

namespace Tetrodotoxin::Model::Dependencies {

// Package is a dependency found by durable package name and exact Major.Minor
// Version. Its binding names the same anonymous Model::Package contract whether
// the resolver supplied an interpreted package or restored a precompiled one.
// Environment owns this edge once and exposes its Alias to every Source in
// the transaction.
class Package final : public Model::Dependency {
 public:
  using ContractOwner = Package;
  static constexpr Perimortem::System::Uuid contract_id{
    0x35ab79042c69423a,
    0xa16217ec792f0121,
  };

  constexpr Package(
      const Model::Dialect& root_dialect,
      Perimortem::Core::View::Bytes package_name,
      Perimortem::System::Version version,
      Perimortem::Core::View::Bytes local_name,
      const Model::Package& package,
      const Ttx::Concept::Documentation& documentation)
      : Dependency(root_dialect, local_name, package, documentation),
        package_name(package_name),
        version(version),
        package(package) {}

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id || Dependency::implements(requested);
  }

  constexpr auto get_package_name() const -> Perimortem::Core::View::Bytes {
    return package_name;
  }

  constexpr auto get_version() const -> Perimortem::System::Version {
    return version;
  }

  constexpr auto get_package() const -> const Model::Package& {
    return package;
  }

 private:
  Perimortem::Core::View::Bytes package_name;
  Perimortem::System::Version version;
  const Model::Package& package;
};

}  // namespace Tetrodotoxin::Model::Dependencies
