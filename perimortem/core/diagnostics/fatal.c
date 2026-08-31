// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/diagnostics/fatal.h"

#include <stdio.h>
#include <stdlib.h>

void perimortem_fatal(const char* message) {
  fputs(message, stderr);
  fputc('\n', stderr);
  fflush(stderr);
  abort();
}
