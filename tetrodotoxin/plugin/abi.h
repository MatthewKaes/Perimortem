// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_PLUGIN_ABI_H
#define TETRODOTOXIN_PLUGIN_ABI_H

#include "tetrodotoxin/language/provider.h"
#include "tetrodotoxin/terminal/provider.h"
#include "ttx/abi.h"

#define TTX_PLUGIN_ABI_MAJOR 1u
#define TTX_PLUGIN_ABI_MINOR 0u

// Requirement descriptors let independently built plugins ask the host for an
// exact shared contract without comparing a C++ type, address, or local enum.
// The digest belongs to that versioned contract artifact. Environment resolves
// the descriptor to the canonical Abstract already owned by this process.
typedef struct {
  ttx_abi_header header;
  ttx_borrowed_bytes package_coordinate;
  ttx_borrowed_bytes exported_route;
  ttx_borrowed_bytes contract_version;
  uint8_t content_sha256[32];
} ttx_requirement_descriptor;

// An export descriptor identifies one provider owned by this plugin. It stays
// separate from the requirement the provider may satisfy, leaving Environment
// responsible for proving that relationship before the candidate enters a
// child Toolchain.
typedef struct {
  ttx_abi_header header;
  ttx_borrowed_bytes package_coordinate;
  ttx_borrowed_bytes exported_route;
  ttx_borrowed_bytes export_version;
  uint8_t content_sha256[32];
} ttx_provider_export_descriptor;

typedef struct ttx_plugin_self ttx_plugin_self;
typedef struct ttx_plugin_ops ttx_plugin_ops;
typedef struct ttx_provider_sink_self ttx_provider_sink_self;
typedef struct ttx_provider_sink_ops ttx_provider_sink_ops;
typedef struct ttx_requirement_sink_self ttx_requirement_sink_self;
typedef struct ttx_requirement_sink_ops ttx_requirement_sink_ops;
typedef struct ttx_plugin_close_sink_self ttx_plugin_close_sink_self;
typedef struct ttx_plugin_close_sink_ops ttx_plugin_close_sink_ops;
typedef struct ttx_host_requirements_self ttx_host_requirements_self;
typedef struct ttx_host_requirements_ops ttx_host_requirements_ops;
typedef struct ttx_requirement_result_self ttx_requirement_result_self;
typedef struct ttx_requirement_result_ops ttx_requirement_result_ops;
typedef struct ttx_plugin_entry_sink_self ttx_plugin_entry_sink_self;
typedef struct ttx_plugin_entry_sink_ops ttx_plugin_entry_sink_ops;

typedef struct {
  const ttx_plugin_ops* operations;
  ttx_plugin_self* self;
} ttx_plugin;

typedef struct {
  const ttx_provider_sink_ops* operations;
  ttx_provider_sink_self* self;
} ttx_provider_sink;

typedef struct {
  const ttx_requirement_sink_ops* operations;
  ttx_requirement_sink_self* self;
} ttx_requirement_sink;

typedef struct {
  const ttx_plugin_close_sink_ops* operations;
  ttx_plugin_close_sink_self* self;
} ttx_plugin_close_sink;

typedef struct {
  const ttx_host_requirements_ops* operations;
  ttx_host_requirements_self* self;
} ttx_host_requirements;

typedef struct {
  const ttx_requirement_result_ops* operations;
  ttx_requirement_result_self* self;
} ttx_requirement_result;

typedef struct {
  const ttx_plugin_entry_sink_ops* operations;
  ttx_plugin_entry_sink_self* self;
} ttx_plugin_entry_sink;

// Every sink is borrowed for one synchronous operation. A provider calls any
// number of item arms followed by exactly one completed or failed arm, and
// retains neither the sink nor descriptor byte views after returning.
struct ttx_provider_sink_ops {
  ttx_abi_header header;
  void(TTX_CALL* provider)(
      ttx_provider_sink_self*,
      const ttx_provider_export_descriptor*,
      ttx_abstract candidate);
  void(TTX_CALL* terminal)(
      ttx_provider_sink_self*,
      const ttx_provider_export_descriptor*,
      tetrodotoxin_terminal_provider provider);
  void(TTX_CALL* dialect)(
      ttx_provider_sink_self*,
      const ttx_provider_export_descriptor*,
      tetrodotoxin_dialect_provider provider);
  void(TTX_CALL* completed)(ttx_provider_sink_self*);
  void(TTX_CALL* failed)(ttx_provider_sink_self*, ttx_abstract error);
};

struct ttx_requirement_sink_ops {
  ttx_abi_header header;
  void(TTX_CALL* requirement)(
      ttx_requirement_sink_self*,
      const ttx_requirement_descriptor*);
  void(TTX_CALL* completed)(ttx_requirement_sink_self*);
  void(TTX_CALL* failed)(ttx_requirement_sink_self*, ttx_abstract error);
};

struct ttx_plugin_close_sink_ops {
  ttx_abi_header header;
  void(TTX_CALL* closed)(ttx_plugin_close_sink_self*);
  void(TTX_CALL* failed)(ttx_plugin_close_sink_self*, ttx_abstract error);
};

struct ttx_requirement_result_ops {
  ttx_abi_header header;
  void(TTX_CALL* resolved)(
      ttx_requirement_result_self*,
      ttx_abstract requirement);
  void(TTX_CALL* missing)(ttx_requirement_result_self*);
  void(TTX_CALL* incompatible)(
      ttx_requirement_result_self*,
      const ttx_requirement_descriptor* installed);
  void(TTX_CALL* failed)(ttx_requirement_result_self*, ttx_abstract error);
};

struct ttx_host_requirements_ops {
  ttx_abi_header header;
  void(TTX_CALL* resolve)(
      ttx_host_requirements_self*,
      const ttx_requirement_descriptor*,
      ttx_requirement_result);
};

struct ttx_plugin_entry_sink_ops {
  ttx_abi_header header;
  void(TTX_CALL* opened)(ttx_plugin_entry_sink_self*, ttx_plugin);
  void(TTX_CALL* incompatible)(
      ttx_plugin_entry_sink_self*,
      uint16_t required_major,
      uint16_t minimum_minor);
  void(TTX_CALL* failed)(ttx_plugin_entry_sink_self*, ttx_abstract error);
};

// Environment retains the Plugin before storing any borrowed provider
// identity. close runs after the child Toolchain and every provider request
// have ended; only then may the final release unload the library containing
// these operation tables.
struct ttx_plugin_ops {
  ttx_abi_header header;
  void(TTX_CALL* retain)(ttx_plugin_self*);
  void(TTX_CALL* release)(ttx_plugin_self*);
  void(TTX_CALL* visit_providers)(ttx_plugin_self*, ttx_provider_sink);
  void(TTX_CALL* visit_dependencies)(
      ttx_plugin_self*,
      ttx_abstract candidate,
      ttx_requirement_sink);
  void(TTX_CALL* close)(ttx_plugin_self*, ttx_plugin_close_sink);
};

typedef void(TTX_CALL* ttx_plugin_entry_function)(
    ttx_host_requirements host,
    ttx_plugin_entry_sink result);

// Every provider library exports this one symbol. The host requirements and
// result sink remain valid only until the call returns; opened transfers one
// retained Plugin reference to Environment.
TTX_EXTERN_C TTX_EXPORT void TTX_CALL
    ttx_plugin_entry(ttx_host_requirements host, ttx_plugin_entry_sink result);

#endif
