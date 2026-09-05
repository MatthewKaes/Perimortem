// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/artifact_file.hpp"

#include <cstddef>
#include <cstdlib>
#include <limits>
#include <utility>

#include "ttx/query.hpp"
#include "ttx/model/requirement.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

struct ObservedBytes {
  ttx_bytes_sink_ops operations;
  std::vector<uint8_t>* output;
  bool completed;
};

struct ObservedArtifact {
  ttx_interface_sink_ops operations;
  ttx_abstract candidate;
  std::optional<Language::ArtifactObservation> observation;
  bool answered;
};

template <typename Owner, typename Self>
static auto observed_owner(Self* self) -> Owner& {
  return *reinterpret_cast<Owner*>(self);
}

static void TTX_CALL
    observe_bytes(ttx_bytes_sink self, ttx_borrowed_bytes bytes) {
  auto& capture = observed_owner<ObservedBytes>(self.self);
  if ((bytes.size != 0 && bytes.data == nullptr) || capture.completed ||
      bytes.size >
          std::numeric_limits<size_t>::max() - capture.output->size()) {
    capture.completed = false;
    capture.output = nullptr;
    return;
  }
  if (bytes.size == 0) {
    return;
  }
  capture.output->insert(
      capture.output->end(), bytes.data, bytes.data + bytes.size);
}

static void TTX_CALL observe_bytes_completed(ttx_bytes_sink self) {
  auto& capture = observed_owner<ObservedBytes>(self.self);
  capture.completed = capture.output != nullptr;
}

static void TTX_CALL observe_artifact_interface(
    ttx_interface_sink self,
    ttx_interface interface) {
  auto& capture = observed_owner<ObservedArtifact>(self.self);
  if (capture.answered || interface.operations == nullptr ||
      interface.operations->header.abi_major != TTX_ABI_MAJOR ||
      interface.operations->header.size <
          sizeof(tetrodotoxin_artifact_file_ops)) {
    capture.answered = true;
    capture.observation.reset();
    return;
  }
  capture.answered = true;
  const auto* operations =
      reinterpret_cast<const tetrodotoxin_artifact_file_ops*>(
          interface.operations);
  if (!ttx_abstract_same(
          operations->interface.requirement(interface),
          tetrodotoxin_artifact_file_requirement()) ||
      !ttx_abstract_same(
          operations->interface.candidate(interface), capture.candidate) ||
      operations->interface.negotiate(interface) != TTX_INTERFACE_SATISFIED) {
    return;
  }

  const ttx_borrowed_bytes route = operations->route(interface);
  if (route.size == 0 || route.data == nullptr ||
      route.size > std::numeric_limits<size_t>::max()) {
    return;
  }
  Language::ArtifactObservation observation = {
    .route = std::vector<uint8_t>(route.data, route.data + route.size),
    .bytes = {},
    .executable = false,
  };
  ObservedBytes bytes = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_bytes_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .bytes = observe_bytes,
          .completed = observe_bytes_completed,
        },
    .output = &observation.bytes,
    .completed = false,
  };
  operations->visit_bytes(
      interface, {
                   .operations = &bytes.operations,
                   .self = reinterpret_cast<ttx_bytes_sink_self*>(&bytes),
                 });
  const uint8_t executable = operations->executable(interface);
  if (!bytes.completed || executable > 1 ||
      observation.bytes.size() != operations->size(interface)) {
    return;
  }
  observation.executable = executable == 1;
  capture.observation = std::move(observation);
}

auto Tetrodotoxin::Language::observe_artifact(ttx_abstract candidate)
    -> std::optional<ArtifactObservation> {
  const ttx_interface_relation constant =
      Ttx::relation(candidate, ttx_constant_requirement());
  if (candidate.operations == nullptr ||
      (constant != TTX_INTERFACE_SATISFIED &&
       constant != TTX_INTERFACE_EQUIVALENT)) {
    return std::nullopt;
  }
  ObservedArtifact capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_interface_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .answer = observe_artifact_interface,
        },
    .candidate = candidate,
    .observation = std::nullopt,
    .answered = false,
  };
  candidate.operations->interface(
      candidate, tetrodotoxin_artifact_file_requirement(),
      {
        .operations = &capture.operations,
        .self = reinterpret_cast<ttx_interface_sink_self*>(&capture),
      });
  return capture.answered ? std::move(capture.observation) : std::nullopt;
}

static auto artifact_requirement() -> ttx_abstract {
  static constinit Ttx::Requirement requirement(
      "Tetrodotoxin.Language.ArtifactFile"_bytes);
  return requirement.get_abi();
}

extern "C" TTX_EXPORT ttx_abstract TTX_CALL
    tetrodotoxin_artifact_file_requirement(void) {
  return artifact_requirement();
}

Language::ArtifactFile::ArtifactFile()
    : route_binding({
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_route_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .candidate = route_candidate,
              .bytes = route_bytes,
            },
      }),
      bytes_binding({
        .operations =
            {
              .header =
                  {
                    .size = sizeof(ttx_bytes_ops),
                    .abi_major = TTX_ABI_MAJOR,
                    .abi_minor = TTX_ABI_MINOR,
                  },
              .candidate = bytes_candidate,
              .size = bytes_size,
              .visit = bytes_visit,
            },
      }),
      domain_layout(get_handle()) {}

auto Language::ArtifactFile::negotiate(ttx_abstract requirement) const
    -> ttx_interface_relation {
  return ttx_abstract_same(requirement, artifact_requirement()) ||
                 ttx_abstract_same(requirement, ttx_route_requirement()) ||
                 ttx_abstract_same(requirement, ttx_bytes_requirement())
             ? TTX_INTERFACE_SATISFIED
             : Ttx::Concept::Constant::negotiate(requirement);
}

void Language::ArtifactFile::interface(
    ttx_abstract self,
    ttx_abstract requirement,
    ttx_interface_sink result) const {
  if (!ttx_abstract_same(requirement, artifact_requirement())) {
    Ttx::Abstract::interface(self, requirement, result);
    return;
  }

  const InterfaceBinding binding = {
    .operations =
        {
          .interface =
              {
                .header =
                    {
                      .size = sizeof(tetrodotoxin_artifact_file_ops),
                      .abi_major = TTX_ABI_MAJOR,
                      .abi_minor = TTX_ABI_MINOR,
                    },
                .requirement = interface_requirement,
                .candidate = interface_candidate,
                .negotiate = interface_relation,
                .invoke = interface_invoke,
              },
          .route = artifact_route,
          .size = artifact_size,
          .visit_bytes = artifact_visit,
          .executable = artifact_executable,
        },
    .owner = this,
    .requirement = requirement,
    .candidate = self,
  };
  result.operations->answer(
      result, {
                .operations = &binding.operations.interface,
                .self = reinterpret_cast<ttx_interface_self*>(
                    const_cast<InterfaceBinding*>(&binding)),
              });
}

void Language::ArtifactFile::route(ttx_abstract self, ttx_route_result result)
    const {
  (void)self;
  result.operations->resolved(
      result, {
                .operations = &route_binding.operations,
                .self = reinterpret_cast<ttx_route_self*>(
                    const_cast<ArtifactFile*>(this)),
              });
}

void Language::ArtifactFile::bytes(ttx_abstract self, ttx_bytes_result result)
    const {
  (void)self;
  result.operations->resolved(
      result, {
                .operations = &bytes_binding.operations,
                .self = reinterpret_cast<ttx_bytes_self*>(
                    const_cast<ArtifactFile*>(this)),
              });
}

void Language::ArtifactFile::domain(ttx_abstract self, ttx_domain_result result)
    const {
  result.operations->resolved(result, self, domain_layout.get_abi());
}

auto Language::ArtifactFile::select(ttx_interface self)
    -> const InterfaceBinding& {
  if (self.operations == nullptr || self.self == nullptr) {
    std::abort();
  }
  const auto& binding = *reinterpret_cast<const InterfaceBinding*>(self.self);
  if (binding.owner == nullptr ||
      !ttx_abstract_same(binding.candidate, binding.owner->get_handle())) {
    std::abort();
  }
  return binding;
}

auto Language::ArtifactFile::select(ttx_route self) -> const ArtifactFile& {
  if (self.operations == nullptr || self.self == nullptr) {
    std::abort();
  }
  return *reinterpret_cast<const ArtifactFile*>(self.self);
}

auto Language::ArtifactFile::select(ttx_bytes self) -> const ArtifactFile& {
  if (self.operations == nullptr || self.self == nullptr) {
    std::abort();
  }
  return *reinterpret_cast<const ArtifactFile*>(self.self);
}

auto TTX_CALL Language::ArtifactFile::interface_requirement(ttx_interface self)
    -> ttx_abstract {
  return select(self).requirement;
}

auto TTX_CALL Language::ArtifactFile::interface_candidate(ttx_interface self)
    -> ttx_abstract {
  return select(self).candidate;
}

auto TTX_CALL Language::ArtifactFile::interface_relation(ttx_interface)
    -> ttx_interface_relation {
  return TTX_INTERFACE_SATISFIED;
}

void TTX_CALL Language::ArtifactFile::interface_invoke(
    ttx_interface,
    ttx_abstract,
    ttx_pack,
    ttx_context,
    ttx_pack_result result) {
  result.operations->none(result);
}

auto TTX_CALL Language::ArtifactFile::artifact_route(ttx_interface self)
    -> ttx_borrowed_bytes {
  const Core::View::Bytes route = select(self).owner->get_artifact_route();
  return {.data = route.get_data(), .size = route.get_size()};
}

auto TTX_CALL Language::ArtifactFile::artifact_size(ttx_interface self)
    -> uint64_t {
  return select(self).owner->get_artifact_bytes().get_size();
}

void TTX_CALL Language::ArtifactFile::artifact_visit(
    ttx_interface self,
    ttx_bytes_sink result) {
  const Core::View::Bytes bytes = select(self).owner->get_artifact_bytes();
  if (!bytes.is_empty()) {
    result.operations->bytes(
        result, {.data = bytes.get_data(), .size = bytes.get_size()});
  }
  result.operations->completed(result);
}

auto TTX_CALL Language::ArtifactFile::artifact_executable(ttx_interface self)
    -> uint8_t {
  return select(self).owner->is_executable() ? 1 : 0;
}

auto TTX_CALL Language::ArtifactFile::route_candidate(ttx_route self)
    -> ttx_abstract {
  return select(self).get_handle();
}

auto TTX_CALL Language::ArtifactFile::route_bytes(ttx_route self)
    -> ttx_borrowed_bytes {
  const Core::View::Bytes route = select(self).get_artifact_route();
  return {.data = route.get_data(), .size = route.get_size()};
}

auto TTX_CALL Language::ArtifactFile::bytes_candidate(ttx_bytes self)
    -> ttx_abstract {
  return select(self).get_handle();
}

auto TTX_CALL Language::ArtifactFile::bytes_size(ttx_bytes self) -> uint64_t {
  return select(self).get_artifact_bytes().get_size();
}

void TTX_CALL
    Language::ArtifactFile::bytes_visit(ttx_bytes self, ttx_bytes_sink result) {
  const Core::View::Bytes bytes = select(self).get_artifact_bytes();
  if (!bytes.is_empty()) {
    result.operations->bytes(
        result, {.data = bytes.get_data(), .size = bytes.get_size()});
  }
  result.operations->completed(result);
}
