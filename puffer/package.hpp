// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/args.hpp"

#include "tetrodotoxin/package/repository/repository.hpp"

namespace Puffer {

// Package owns one command line Package compilation transaction. It restores
// dependency Contracts, imports the root source Package once, and keeps that
// Workspace alive while the selected Terminals produce every member artifact.
// The semantic Archives become visible only after the complete product agrees.
class Package {
 public:
  constexpr Package(
      const Perimortem::System::Args::Values& arguments,
      Tetrodotoxin::Package::Repository::Repository& repository)
      : arguments(arguments), repository(repository) {}

  auto run() const -> S32;

 private:
  const Perimortem::System::Args::Values& arguments;
  Tetrodotoxin::Package::Repository::Repository& repository;
};

}  // namespace Puffer
