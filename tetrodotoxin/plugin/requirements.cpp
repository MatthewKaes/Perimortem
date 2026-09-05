// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/plugin/requirements.hpp"

using namespace Tetrodotoxin;

struct RequirementCapture {
  bool answered;
  ttx_abstract requirement;
};

static void TTX_CALL
    resolved(ttx_requirement_result_self* self, ttx_abstract requirement) {
  auto& capture = *reinterpret_cast<RequirementCapture*>(self);
  capture.answered = true;
  capture.requirement = requirement;
}

static void TTX_CALL missing(ttx_requirement_result_self* self) {
  reinterpret_cast<RequirementCapture*>(self)->answered = true;
}

static void TTX_CALL incompatible(
    ttx_requirement_result_self* self,
    const ttx_requirement_descriptor*) {
  missing(self);
}

static void TTX_CALL failed(ttx_requirement_result_self* self, ttx_abstract) {
  missing(self);
}

auto Plugin::resolve_requirement(
    ttx_host_requirements host,
    const ttx_requirement_descriptor& descriptor)
    -> std::optional<ttx_abstract> {
  if (host.operations == nullptr || host.self == nullptr ||
      host.operations->resolve == nullptr) {
    return std::nullopt;
  }
  static const ttx_requirement_result_ops operations = {
    .header =
        {
          .size = sizeof(ttx_requirement_result_ops),
          .abi_major = TTX_PLUGIN_ABI_MAJOR,
          .abi_minor = TTX_PLUGIN_ABI_MINOR,
        },
    .resolved = resolved,
    .missing = missing,
    .incompatible = incompatible,
    .failed = failed,
  };
  RequirementCapture capture = {};
  host.operations->resolve(
      host.self, &descriptor,
      {
        .operations = &operations,
        .self = reinterpret_cast<ttx_requirement_result_self*>(&capture),
      });
  return capture.answered && capture.requirement.operations != nullptr
             ? std::optional<ttx_abstract>(capture.requirement)
             : std::nullopt;
}
