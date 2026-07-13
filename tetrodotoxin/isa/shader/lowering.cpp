// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/shader/compiler.hpp"
#include "tetrodotoxin/isa/shader/virtual_machine.hpp"
#include "tetrodotoxin/terminal/context.hpp"
#include "tetrodotoxin/terminal/input.hpp"

using namespace Tetrodotoxin;

auto Isa::Shader::VirtualMachine::lower(
    Terminal::Context& context,
    const Terminal::Input& input) -> Bool {
  Isa::Shader::Compiler compiler;
  if (!compiler.lower(
          context.get_arena(), context.get_errors(), input.get_source(),
          input.get_module(), input.get_type(), input.get_implementation())) {
    return False;
  }

  return context.publish_read_only(
      compiler.get_read_only(), compiler.get_stages());
}
