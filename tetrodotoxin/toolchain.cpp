// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/toolchain.hpp"

#include "tetrodotoxin/isa/app/virtual_machine.hpp"
#include "tetrodotoxin/isa/library/virtual_machine.hpp"
#include "tetrodotoxin/isa/package/virtual_machine.hpp"
#include "tetrodotoxin/isa/render/virtual_machine.hpp"
#include "tetrodotoxin/isa/scene/virtual_machine.hpp"
#include "tetrodotoxin/isa/shader/virtual_machine.hpp"

using namespace Tetrodotoxin;

auto Toolchain::standard() -> Toolchain {
  Toolchain toolchain;
  toolchain.install_standard_isas();
  return toolchain;
}

auto Toolchain::install_standard_isas() -> void {
  isa_registry.install(
      Isa::App::VirtualMachine::get_name(),
      Isa::App::VirtualMachine::evaluate);
  isa_registry.install(
      Isa::Library::VirtualMachine::get_name(),
      Isa::Library::VirtualMachine::evaluate);
  isa_registry.install(
      Isa::Package::VirtualMachine::get_name(),
      Isa::Package::VirtualMachine::evaluate);
  isa_registry.install(
      Isa::Render::VirtualMachine::get_name(),
      Isa::Render::VirtualMachine::evaluate);
  isa_registry.install(
      Isa::Scene::VirtualMachine::get_name(),
      Isa::Scene::VirtualMachine::evaluate);
  isa_registry.install(
      Isa::Shader::VirtualMachine::get_name(),
      Isa::Shader::VirtualMachine::evaluate);
}
