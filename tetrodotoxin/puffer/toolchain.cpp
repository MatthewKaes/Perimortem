// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/toolchain.hpp"

#include "tetrodotoxin/compiler/target/system_v.hpp"
#include "tetrodotoxin/isa/app/virtual_machine.hpp"
#include "tetrodotoxin/isa/library/virtual_machine.hpp"
#include "tetrodotoxin/isa/package/virtual_machine.hpp"
#include "tetrodotoxin/isa/render/virtual_machine.hpp"
#include "tetrodotoxin/isa/scene/virtual_machine.hpp"
#include "tetrodotoxin/isa/shader/virtual_machine.hpp"

using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Puffer;

auto Toolchain::standard_registry() -> Tetrodotoxin::Isa::Registry {
  Tetrodotoxin::Isa::Registry registry;
  registry.install(
      Tetrodotoxin::Isa::App::VirtualMachine::get_name(),
      Tetrodotoxin::Isa::App::VirtualMachine::evaluate);
  registry.install(
      Tetrodotoxin::Isa::Library::VirtualMachine::get_name(),
      Tetrodotoxin::Isa::Library::VirtualMachine::evaluate,
      Tetrodotoxin::Isa::Library::VirtualMachine::lower, True);
  registry.install(
      Tetrodotoxin::Isa::Package::VirtualMachine::get_name(),
      Tetrodotoxin::Isa::Package::VirtualMachine::evaluate);
  registry.install(
      Tetrodotoxin::Isa::Render::VirtualMachine::get_name(),
      Tetrodotoxin::Isa::Render::VirtualMachine::evaluate,
      Tetrodotoxin::Isa::Render::VirtualMachine::lower, True);
  registry.install(
      Tetrodotoxin::Isa::Scene::VirtualMachine::get_name(),
      Tetrodotoxin::Isa::Scene::VirtualMachine::evaluate);
  registry.install(
      Tetrodotoxin::Isa::Shader::VirtualMachine::get_name(),
      Tetrodotoxin::Isa::Shader::VirtualMachine::evaluate,
      Tetrodotoxin::Isa::Shader::VirtualMachine::lower);
  return registry;
}

auto Toolchain::standard_backend() -> Tetrodotoxin::Compiler::Backend {
  return Tetrodotoxin::Compiler::Target::SystemV::backend();
}
