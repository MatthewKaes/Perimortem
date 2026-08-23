// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/command.hpp"

S32 main(S32 argc, char** argv) {
  return Puffer::Command(argc, argv).run();
}
