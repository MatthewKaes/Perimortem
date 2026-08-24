// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/args.hpp"

namespace Puffer {

// Package owns one command line Package compilation transaction. It restores
// dependency Contracts, imports the root source Package once, and keeps that
// Workspace alive while the selected Terminals produce every member artifact.
// The semantic Archives become visible only after the complete product agrees.
class Package {
 public:
  constexpr Package(const Perimortem::System::Args::Values& arguments)
      : arguments(arguments) {}

  auto run() const -> S32;

 private:
  const Perimortem::System::Args::Values& arguments;
};

}  // namespace Puffer
