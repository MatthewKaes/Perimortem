// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/system/args.hpp"

namespace Puffer {

// Package owns one command-line Package compilation transaction. It restores
// dependency Interfaces, imports the root source Package once, compiles each
// declared member independently, and publishes the two semantic Archives only
// after every member product succeeds.
class Package {
 public:
  constexpr Package(const Perimortem::System::Args::Values& arguments)
      : arguments(arguments) {}

  auto run() const -> Signed_32;

 private:
  const Perimortem::System::Args::Values& arguments;
};

}  // namespace Puffer
