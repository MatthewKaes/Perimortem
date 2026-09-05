// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <cstring>

#include "tetrodotoxin/plugin/provider_library.hpp"

static auto borrowed(const char* text) -> ttx_borrowed_bytes {
  return {
    .data = reinterpret_cast<const uint8_t*>(text),
    .size = std::strlen(text),
  };
}

class SystemRuntime final : public Ttx::Abstract {
 public:
  auto name() const -> ttx_borrowed_bytes override {
    return borrowed("Perimortem System runtime");
  }
};

static constexpr uint8_t package_name[] = "Perimortem.System";
static constexpr uint8_t export_name[] = "NativeRuntime";
static constexpr uint8_t export_version[] = "1.0";

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
  .content_sha256 = {0x53},
};

static SystemRuntime runtime;
static Tetrodotoxin::Plugin::ProviderLibrary plugin(runtime, descriptor);

extern "C" TTX_EXPORT void TTX_CALL
    ttx_plugin_entry(ttx_host_requirements, ttx_plugin_entry_sink result) {
  plugin.open(result);
}
