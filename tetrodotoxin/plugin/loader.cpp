// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/plugin/loader.hpp"

#include <cstring>
#include <dlfcn.h>
#include <limits>
#include <utility>

using namespace Tetrodotoxin::Plugin;

template <typename Operations>
static auto supports(const Operations* operations, uint32_t size) -> bool {
  return operations != nullptr &&
         operations->header.abi_major == TTX_PLUGIN_ABI_MAJOR &&
         operations->header.size >= size;
}

template <typename Operations>
static auto supports_ttx(const Operations* operations, uint32_t size) -> bool {
  return operations != nullptr &&
         operations->header.abi_major == TTX_ABI_MAJOR &&
         operations->header.size >= size;
}

static auto valid_bytes(ttx_borrowed_bytes value) -> bool {
  return value.size <= std::numeric_limits<size_t>::max() &&
         (value.size == 0 || value.data != nullptr);
}

static auto copy(ttx_borrowed_bytes value) -> std::string {
  if (value.size == 0) {
    return {};
  }
  return std::string(
      reinterpret_cast<const char*>(value.data),
      static_cast<size_t>(value.size));
}

static auto valid(const ttx_requirement_descriptor* descriptor) -> bool {
  return descriptor != nullptr &&
         descriptor->header.abi_major == TTX_PLUGIN_ABI_MAJOR &&
         descriptor->header.size >= sizeof(ttx_requirement_descriptor) &&
         valid_bytes(descriptor->package_coordinate) &&
         valid_bytes(descriptor->exported_route) &&
         valid_bytes(descriptor->contract_version);
}

static auto valid(const ttx_provider_export_descriptor* descriptor) -> bool {
  return descriptor != nullptr &&
         descriptor->header.abi_major == TTX_PLUGIN_ABI_MAJOR &&
         descriptor->header.size >= sizeof(ttx_provider_export_descriptor) &&
         valid_bytes(descriptor->package_coordinate) &&
         valid_bytes(descriptor->exported_route) &&
         valid_bytes(descriptor->export_version);
}

struct EntryCapture {
  bool answered = false;
  bool opened = false;
  bool incompatible = false;
  bool invalid = false;
  ttx_plugin plugin = {};
};

static auto entry_capture(ttx_plugin_entry_sink_self* self) -> EntryCapture& {
  return *reinterpret_cast<EntryCapture*>(self);
}

static void TTX_CALL
    entry_opened(ttx_plugin_entry_sink_self* self, ttx_plugin plugin) {
  EntryCapture& capture = entry_capture(self);
  if (capture.answered) {
    capture.invalid = true;
    return;
  }
  capture.answered = true;
  capture.opened = true;
  capture.plugin = plugin;
}

static void TTX_CALL
    entry_incompatible(ttx_plugin_entry_sink_self* self, uint16_t, uint16_t) {
  EntryCapture& capture = entry_capture(self);
  if (capture.answered) {
    capture.invalid = true;
    return;
  }
  capture.answered = true;
  capture.incompatible = true;
}

static void TTX_CALL
    entry_failed(ttx_plugin_entry_sink_self* self, ttx_abstract) {
  EntryCapture& capture = entry_capture(self);
  if (capture.answered) {
    capture.invalid = true;
    return;
  }
  capture.answered = true;
}

auto Loaded::open(
    const std::string& path,
    ttx_host_requirements host,
    LoadFailure& failure) -> std::optional<Loaded> {
  void* library = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (library == nullptr) {
    failure = LoadFailure::LibraryUnavailable;
    return std::nullopt;
  }

  void* symbol = dlsym(library, "ttx_plugin_entry");
  ttx_plugin_entry_function entry = nullptr;
  static_assert(sizeof(entry) == sizeof(symbol));
  std::memcpy(&entry, &symbol, sizeof(entry));
  if (entry == nullptr) {
    dlclose(library);
    failure = LoadFailure::EntryUnavailable;
    return std::nullopt;
  }

  static const ttx_plugin_entry_sink_ops operations = {
    .header =
        {
          .size = sizeof(ttx_plugin_entry_sink_ops),
          .abi_major = TTX_PLUGIN_ABI_MAJOR,
          .abi_minor = TTX_PLUGIN_ABI_MINOR,
        },
    .opened = entry_opened,
    .incompatible = entry_incompatible,
    .failed = entry_failed,
  };
  EntryCapture capture;
  entry(
      host, {
              .operations = &operations,
              .self = reinterpret_cast<ttx_plugin_entry_sink_self*>(&capture),
            });
  if (capture.invalid) {
    // Once a plugin has transferred a handle, duplicate result arms leave the
    // host unable to know which state its release operation expects. Preserve
    // the mapping until process exit instead of guessing at cleanup.
    failure = LoadFailure::InvalidProtocol;
    return std::nullopt;
  }
  if (!capture.answered || !capture.opened) {
    dlclose(library);
    failure = capture.incompatible ? LoadFailure::IncompatibleAbi
                                   : LoadFailure::EntryRejected;
    return std::nullopt;
  }
  if (!supports(capture.plugin.operations, sizeof(ttx_plugin_ops)) ||
      capture.plugin.self == nullptr ||
      capture.plugin.operations->retain == nullptr ||
      capture.plugin.operations->release == nullptr ||
      capture.plugin.operations->visit_providers == nullptr ||
      capture.plugin.operations->visit_dependencies == nullptr ||
      capture.plugin.operations->close == nullptr) {
    // A malformed plugin has already transferred an opaque retained handle,
    // but has not supplied enough protocol for the host to release it safely.
    // Keep its code mapped until process exit rather than calling an unproved
    // operation or unmapping callbacks that may still belong to the handle.
    failure = LoadFailure::InvalidProtocol;
    return std::nullopt;
  }

  return Loaded(library, capture.plugin);
}

Loaded::Loaded(Loaded&& source) noexcept
    : library(std::exchange(source.library, nullptr)),
      plugin(std::exchange(source.plugin, {})) {}

Loaded::~Loaded() {
  close();
}

struct ProviderCapture {
  std::vector<ProviderDescriptor>* output;
  bool ended = false;
  bool failed = false;
};

static auto provider_capture(ttx_provider_sink_self* self) -> ProviderCapture& {
  return *reinterpret_cast<ProviderCapture*>(self);
}

static void TTX_CALL provider_item(
    ttx_provider_sink_self* self,
    const ttx_provider_export_descriptor* descriptor,
    ttx_abstract candidate) {
  ProviderCapture& capture = provider_capture(self);
  if (capture.ended || !valid(descriptor) || candidate.operations == nullptr ||
      candidate.operations->header.abi_major != TTX_ABI_MAJOR ||
      candidate.operations->header.size < TTX_ABSTRACT_INTERFACE_PREFIX_SIZE ||
      candidate.operations->name == nullptr ||
      candidate.operations->documentation == nullptr ||
      candidate.operations->resolve == nullptr ||
      candidate.operations->resolve_concept == nullptr ||
      candidate.operations->visit_concepts == nullptr ||
      candidate.operations->interface == nullptr) {
    capture.failed = true;
    return;
  }
  ProviderDescriptor copied = {
    .package_coordinate = copy(descriptor->package_coordinate),
    .exported_route = copy(descriptor->exported_route),
    .export_version = copy(descriptor->export_version),
    .content_sha256 = {},
    .candidate = candidate,
    .terminal = {},
    .dialect = {},
  };
  std::memcpy(
      copied.content_sha256.data(), descriptor->content_sha256,
      copied.content_sha256.size());
  capture.output->push_back(std::move(copied));
}

static void TTX_CALL terminal_item(
    ttx_provider_sink_self* self,
    const ttx_provider_export_descriptor* descriptor,
    tetrodotoxin_terminal_provider terminal) {
  ProviderCapture& capture = provider_capture(self);
  if (capture.ended || !valid(descriptor) ||
      !supports_ttx(
          terminal.operations, sizeof(tetrodotoxin_terminal_provider_ops)) ||
      terminal.self == nullptr || terminal.operations->retain == nullptr ||
      terminal.operations->release == nullptr ||
      terminal.operations->candidate == nullptr ||
      terminal.operations->begin == nullptr) {
    capture.failed = true;
    return;
  }
  const ttx_abstract candidate = terminal.operations->candidate(terminal.self);
  if (candidate.operations == nullptr ||
      candidate.operations->header.abi_major != TTX_ABI_MAJOR ||
      candidate.operations->header.size < TTX_ABSTRACT_INTERFACE_PREFIX_SIZE) {
    capture.failed = true;
    return;
  }
  ProviderDescriptor copied = {
    .package_coordinate = copy(descriptor->package_coordinate),
    .exported_route = copy(descriptor->exported_route),
    .export_version = copy(descriptor->export_version),
    .content_sha256 = {},
    .candidate = candidate,
    .terminal = terminal,
    .dialect = {},
  };
  std::memcpy(
      copied.content_sha256.data(), descriptor->content_sha256,
      copied.content_sha256.size());
  capture.output->push_back(std::move(copied));
}

static void TTX_CALL dialect_item(
    ttx_provider_sink_self* self,
    const ttx_provider_export_descriptor* descriptor,
    tetrodotoxin_dialect_provider dialect) {
  ProviderCapture& capture = provider_capture(self);
  if (capture.ended || !valid(descriptor) ||
      !supports_ttx(
          dialect.operations, sizeof(tetrodotoxin_dialect_provider_ops)) ||
      dialect.self == nullptr || dialect.operations->retain == nullptr ||
      dialect.operations->release == nullptr ||
      dialect.operations->candidate == nullptr ||
      dialect.operations->name == nullptr ||
      dialect.operations->interpret == nullptr) {
    capture.failed = true;
    return;
  }
  const ttx_abstract candidate = dialect.operations->candidate(dialect.self);
  const ttx_borrowed_bytes name = dialect.operations->name(dialect.self);
  if (candidate.operations == nullptr ||
      candidate.operations->header.abi_major != TTX_ABI_MAJOR ||
      candidate.operations->header.size < TTX_ABSTRACT_INTERFACE_PREFIX_SIZE ||
      !valid_bytes(name)) {
    capture.failed = true;
    return;
  }
  ProviderDescriptor copied = {
    .package_coordinate = copy(descriptor->package_coordinate),
    .exported_route = copy(descriptor->exported_route),
    .export_version = copy(descriptor->export_version),
    .content_sha256 = {},
    .candidate = candidate,
    .terminal = {},
    .dialect = dialect,
  };
  std::memcpy(
      copied.content_sha256.data(), descriptor->content_sha256,
      copied.content_sha256.size());
  capture.output->push_back(std::move(copied));
}

static void TTX_CALL provider_completed(ttx_provider_sink_self* self) {
  ProviderCapture& capture = provider_capture(self);
  if (capture.ended) {
    capture.failed = true;
    return;
  }
  capture.ended = true;
}

static void TTX_CALL
    provider_failed(ttx_provider_sink_self* self, ttx_abstract) {
  ProviderCapture& capture = provider_capture(self);
  if (capture.ended) {
    capture.failed = true;
    return;
  }
  capture.ended = true;
  capture.failed = true;
}

auto Loaded::providers(std::vector<ProviderDescriptor>& output) const -> bool {
  if (plugin.operations == nullptr) {
    return false;
  }
  static const ttx_provider_sink_ops operations = {
    .header =
        {
          .size = sizeof(ttx_provider_sink_ops),
          .abi_major = TTX_PLUGIN_ABI_MAJOR,
          .abi_minor = TTX_PLUGIN_ABI_MINOR,
        },
    .provider = provider_item,
    .terminal = terminal_item,
    .dialect = dialect_item,
    .completed = provider_completed,
    .failed = provider_failed,
  };
  const size_t beginning = output.size();
  ProviderCapture capture = {.output = &output};
  plugin.operations->visit_providers(
      plugin.self,
      {
        .operations = &operations,
        .self = reinterpret_cast<ttx_provider_sink_self*>(&capture),
      });
  if (!capture.ended || capture.failed) {
    output.resize(beginning);
    return false;
  }
  return true;
}

struct RequirementCapture {
  std::vector<RequirementDescriptor>* output;
  bool ended = false;
  bool failed = false;
};

static auto requirement_capture(ttx_requirement_sink_self* self)
    -> RequirementCapture& {
  return *reinterpret_cast<RequirementCapture*>(self);
}

static void TTX_CALL requirement_item(
    ttx_requirement_sink_self* self,
    const ttx_requirement_descriptor* descriptor) {
  RequirementCapture& capture = requirement_capture(self);
  if (capture.ended || !valid(descriptor)) {
    capture.failed = true;
    return;
  }
  RequirementDescriptor copied = {
    .package_coordinate = copy(descriptor->package_coordinate),
    .exported_route = copy(descriptor->exported_route),
    .contract_version = copy(descriptor->contract_version),
    .content_sha256 = {},
  };
  std::memcpy(
      copied.content_sha256.data(), descriptor->content_sha256,
      copied.content_sha256.size());
  capture.output->push_back(std::move(copied));
}

static void TTX_CALL requirement_completed(ttx_requirement_sink_self* self) {
  RequirementCapture& capture = requirement_capture(self);
  if (capture.ended) {
    capture.failed = true;
    return;
  }
  capture.ended = true;
}

static void TTX_CALL
    requirement_failed(ttx_requirement_sink_self* self, ttx_abstract) {
  RequirementCapture& capture = requirement_capture(self);
  if (capture.ended) {
    capture.failed = true;
    return;
  }
  capture.ended = true;
  capture.failed = true;
}

auto Loaded::dependencies(
    ttx_abstract candidate,
    std::vector<RequirementDescriptor>& output) const -> bool {
  if (plugin.operations == nullptr || candidate.operations == nullptr) {
    return false;
  }
  static const ttx_requirement_sink_ops operations = {
    .header =
        {
          .size = sizeof(ttx_requirement_sink_ops),
          .abi_major = TTX_PLUGIN_ABI_MAJOR,
          .abi_minor = TTX_PLUGIN_ABI_MINOR,
        },
    .requirement = requirement_item,
    .completed = requirement_completed,
    .failed = requirement_failed,
  };
  const size_t beginning = output.size();
  RequirementCapture capture = {.output = &output};
  plugin.operations->visit_dependencies(
      plugin.self, candidate,
      {
        .operations = &operations,
        .self = reinterpret_cast<ttx_requirement_sink_self*>(&capture),
      });
  if (!capture.ended || capture.failed) {
    output.resize(beginning);
    return false;
  }
  return true;
}

struct CloseCapture {
  bool answered = false;
  bool closed = false;
};

static auto close_capture(ttx_plugin_close_sink_self* self) -> CloseCapture& {
  return *reinterpret_cast<CloseCapture*>(self);
}

static void TTX_CALL close_completed(ttx_plugin_close_sink_self* self) {
  CloseCapture& capture = close_capture(self);
  if (capture.answered) {
    capture.closed = false;
    return;
  }
  capture.answered = true;
  capture.closed = true;
}

static void TTX_CALL
    close_failed(ttx_plugin_close_sink_self* self, ttx_abstract) {
  CloseCapture& capture = close_capture(self);
  if (capture.answered) {
    capture.closed = false;
    return;
  }
  capture.answered = true;
}

auto Loaded::close() -> bool {
  if (library == nullptr) {
    return true;
  }
  static const ttx_plugin_close_sink_ops operations = {
    .header =
        {
          .size = sizeof(ttx_plugin_close_sink_ops),
          .abi_major = TTX_PLUGIN_ABI_MAJOR,
          .abi_minor = TTX_PLUGIN_ABI_MINOR,
        },
    .closed = close_completed,
    .failed = close_failed,
  };
  if (plugin.operations != nullptr) {
    CloseCapture capture;
    plugin.operations->close(
        plugin.self,
        {
          .operations = &operations,
          .self = reinterpret_cast<ttx_plugin_close_sink_self*>(&capture),
        });
    if (!capture.answered || !capture.closed) {
      return false;
    }
    plugin.operations->release(plugin.self);
    plugin = {};
  }
  const bool unloaded = dlclose(library) == 0;
  if (unloaded) {
    library = nullptr;
  }
  return unloaded;
}
