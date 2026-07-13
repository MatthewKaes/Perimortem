// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/lowering/context.hpp"
#include "tetrodotoxin/isa/lowering/input.hpp"
#include "tetrodotoxin/isa/shader/compiler.hpp"
#include "tetrodotoxin/isa/shader/virtual_machine.hpp"

using namespace Tetrodotoxin;

auto Isa::Shader::VirtualMachine::lower(
    Isa::Lowering::Context& context,
    const Isa::Lowering::Input& input) -> Bool {
  Isa::Shader::Compiler compiler;
  if (!compiler.lower(
          context.get_arena(), context.get_errors(), input.source, input.module,
          input.type, input.implementation)) {
    return False;
  }

  return context.publish_read_only(
      compiler.get_read_only(), compiler.get_stages());
}
