// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/args.hpp"

namespace Puffer {

// Application turns one archived App selection into the native entry object
// requested by the host. Dependency Contract Archives and the root Complete
// Archive rebuild the semantic graph for that request, then the finished entry
// object leaves the transaction as its product.
class Application {
 public:
  constexpr Application(const Perimortem::System::Args::Values& arguments)
      : arguments(arguments) {}

  auto run() const -> S32;

 private:
  const Perimortem::System::Args::Values& arguments;
};

}  // namespace Puffer
