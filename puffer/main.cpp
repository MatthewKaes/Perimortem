// Perimortem Engine
// Copyright © Matt Kaes

#include "puffer/command.hpp"

S32 main(S32 argc, char** argv) {
  return Puffer::Command(argc, argv).run();
}
