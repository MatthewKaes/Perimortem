// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/toolchain.hpp"

#include "tetrodotoxin/isa/library/library.hpp"
#include "tetrodotoxin/isa/package/package.hpp"
#include "tetrodotoxin/isa/render/render.hpp"
#include "tetrodotoxin/isa/shader/shader.hpp"

using namespace Tetrodotoxin;

auto Toolchain::standard() -> Toolchain {
  Toolchain toolchain;
  toolchain.install_standard_isas();
  return toolchain;
}

auto Toolchain::install_standard_isas() -> void {
  isa_registry.install(Isa::Library::get_name(), Isa::Library::evaluate);
  isa_registry.install(Isa::Package::get_name(), Isa::Package::evaluate);
  isa_registry.install(Isa::Render::get_name(), Isa::Render::evaluate);
  isa_registry.install(Isa::Shader::get_name(), Isa::Shader::evaluate);
}
