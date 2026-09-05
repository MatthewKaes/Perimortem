// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Puffer {

// Command recognizes only the transport-level split between one source and one
// LSP pipe. Once a source is selected, every remaining byte belongs to that
// source's Dialect and enters the immutable Puffer invocation unchanged.
class Command {
 public:
  constexpr Command(S32 argument_count, char** argument_values)
      : argument_count(argument_count), argument_values(argument_values) {}

  auto run() const -> S32;

 private:
  S32 argument_count;
  char** argument_values;
};

}  // namespace Puffer
