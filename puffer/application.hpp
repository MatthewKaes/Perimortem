// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/args.hpp"

#include "tetrodotoxin/package/repository/repository.hpp"

namespace Puffer {

// Application turns one archived App selection into the native entry source
// requested by the host. Dependency Contract Archives and the root Complete
// Archive rebuild the semantic graph for that request, then the finished entry
// source leaves the transaction as its product.
class Application {
 public:
  constexpr Application(
      const Perimortem::System::Args::Values& arguments,
      Tetrodotoxin::Package::Repository::Repository& repository)
      : arguments(arguments), repository(repository) {}

  auto run() const -> S32;

 private:
  const Perimortem::System::Args::Values& arguments;
  Tetrodotoxin::Package::Repository::Repository& repository;
};

}  // namespace Puffer
