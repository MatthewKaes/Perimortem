// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/system/args.hpp"

namespace Puffer {

// Application owns one source-free App entry compilation request. Dependency
// Interfaces and the root Complete Archive reconstruct the semantic graph;
// only the resulting host entry object leaves the transaction.
class Application {
 public:
  constexpr Application(const Perimortem::System::Args::Values& arguments)
      : arguments(arguments) {}

  auto run() const -> Signed_32;

 private:
  const Perimortem::System::Args::Values& arguments;
};

}  // namespace Puffer
