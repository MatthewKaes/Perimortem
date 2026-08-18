// Perimortem Engine
// Copyright © Matt Kaes

#include "puffer/command.hpp"

Signed_32 main(Signed_32 argc, char** argv) {
  return Puffer::Command(argc, argv).run();
}
