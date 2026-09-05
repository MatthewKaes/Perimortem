// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <memory>

#include "tetrodotoxin/language/error.hpp"
#include "tetrodotoxin/plugin/dialect_library.hpp"
#include "tetrodotoxin/plugin/requirements.hpp"
#include "tetrodotoxin/shader/dialect.hpp"

using namespace Perimortem::Core;

class ShaderPluginError final : public Tetrodotoxin::Language::Error {
 public:
  TTX_NAME("Shader plugin error"_view);

  void describe(Ttx::Lexical::Errors::Report& report) const override {
    report
        << "Shader plugin could not acquire its Library and Render dependencies."_view;
  }
};

static constexpr uint8_t package_name[] = "Tetrodotoxin.Shader";
static constexpr uint8_t export_name[] = "Dialect";
static constexpr uint8_t export_version[] = "1.0";
static constexpr uint8_t library_package[] = "Tetrodotoxin.Library";
static constexpr uint8_t render_package[] = "Tetrodotoxin.Render";
static constexpr uint8_t cpp_package[] = "Tetrodotoxin.Cpp";
static constexpr uint8_t cpp_route[] = "Owner";

static const ttx_provider_export_descriptor descriptor = {
  .header =
      {
        .size = sizeof(ttx_provider_export_descriptor),
        .abi_major = TTX_PLUGIN_ABI_MAJOR,
        .abi_minor = TTX_PLUGIN_ABI_MINOR,
      },
  .package_coordinate = {package_name, sizeof(package_name) - 1},
  .exported_route = {export_name, sizeof(export_name) - 1},
  .export_version = {export_version, sizeof(export_version) - 1},
  .content_sha256 = {0x48},
};

static const ttx_requirement_descriptor dependencies[] = {
  {
    .header =
        {
          .size = sizeof(ttx_requirement_descriptor),
          .abi_major = TTX_PLUGIN_ABI_MAJOR,
          .abi_minor = TTX_PLUGIN_ABI_MINOR,
        },
    .package_coordinate = {library_package, sizeof(library_package) - 1},
    .exported_route = {export_name, sizeof(export_name) - 1},
    .contract_version = {export_version, sizeof(export_version) - 1},
    .content_sha256 = {0x4c},
  },
  {
    .header =
        {
          .size = sizeof(ttx_requirement_descriptor),
          .abi_major = TTX_PLUGIN_ABI_MAJOR,
          .abi_minor = TTX_PLUGIN_ABI_MINOR,
        },
    .package_coordinate = {render_package, sizeof(render_package) - 1},
    .exported_route = {export_name, sizeof(export_name) - 1},
    .contract_version = {export_version, sizeof(export_version) - 1},
    .content_sha256 = {0x52},
  },
  {
    .header =
        {
          .size = sizeof(ttx_requirement_descriptor),
          .abi_major = TTX_PLUGIN_ABI_MAJOR,
          .abi_minor = TTX_PLUGIN_ABI_MINOR,
        },
    .package_coordinate = {cpp_package, sizeof(cpp_package) - 1},
    .exported_route = {cpp_route, sizeof(cpp_route) - 1},
    .contract_version = {export_version, sizeof(export_version) - 1},
    .content_sha256 = {0x43},
  },
};

static ShaderPluginError error;
static std::unique_ptr<Tetrodotoxin::Shader::Dialect> dialect;
static std::unique_ptr<Tetrodotoxin::Plugin::DialectLibrary> plugin;

extern "C" TTX_EXPORT void TTX_CALL
    ttx_plugin_entry(ttx_host_requirements host, ttx_plugin_entry_sink result) {
  auto library =
      Tetrodotoxin::Plugin::resolve_requirement(host, dependencies[0]);
  auto render =
      Tetrodotoxin::Plugin::resolve_requirement(host, dependencies[1]);
  auto cpp = Tetrodotoxin::Plugin::resolve_requirement(host, dependencies[2]);
  if (!library || !render || !cpp) {
    result.operations->failed(result.self, error.get_abi());
    return;
  }
  Ttx::Abstract* local_library = Ttx::Abstract::local_owner(*library, *cpp);
  Ttx::Abstract* local_render = Ttx::Abstract::local_owner(*render, *cpp);
  if (local_library == nullptr || local_render == nullptr) {
    result.operations->failed(result.self, error.get_abi());
    return;
  }
  auto& library_semantic = static_cast<Ttx::Concept::Abstract&>(*local_library);
  auto& render_semantic = static_cast<Ttx::Concept::Abstract&>(*local_render);
  auto& library_dependency =
      static_cast<Tetrodotoxin::Library::Dialect&>(library_semantic);
  auto& render_dependency =
      static_cast<Tetrodotoxin::Render::Dialect&>(render_semantic);
  dialect = std::make_unique<Tetrodotoxin::Shader::Dialect>(
      library_dependency, render_dependency);
  plugin = std::make_unique<Tetrodotoxin::Plugin::DialectLibrary>(
      *dialect, descriptor, dependencies, std::size(dependencies));
  plugin->open(result);
}
