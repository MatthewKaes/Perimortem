// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <optional>
#include <vector>

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/language/artifact_file.h"
#include "ttx/concept/constant.hpp"
#include "ttx/model/layouts/value.hpp"

namespace Tetrodotoxin::Language {

struct ArtifactObservation {
  std::vector<uint8_t> route;
  std::vector<uint8_t> bytes;
  bool executable;
};

// Consumers use the public Interface to copy one complete immutable artifact.
// Keeping this protocol check beside ArtifactFile gives Environment and Puffer
// the same admission path without teaching either consumer about a C++ product
// class or a provider's private buffer.
auto observe_artifact(ttx_abstract candidate)
    -> std::optional<ArtifactObservation>;

// An ArtifactFile is one immutable product that a Publisher can materialize
// without learning which Terminal produced it. The logical route stays inside
// the product Pack until publication, and byte visitation never grants the
// Publisher access to a provider's buffer or filesystem.
class ArtifactFile : public Ttx::Concept::Constant {
 public:
  ArtifactFile();

  virtual constexpr auto get_artifact_route() const
      -> Perimortem::Core::View::Bytes = 0;
  virtual constexpr auto get_artifact_bytes() const
      -> Perimortem::Core::View::Bytes = 0;
  virtual constexpr auto is_executable() const -> Bool = 0;

  void interface(
      ttx_abstract self,
      ttx_abstract requirement,
      ttx_interface_sink result) const override;
  void route(ttx_abstract self, ttx_route_result result) const override;
  void bytes(ttx_abstract self, ttx_bytes_result result) const override;
  void domain(ttx_abstract self, ttx_domain_result result) const override;

 protected:
  auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation override;

 private:
  struct InterfaceBinding : ttx_interface_capability {
    InterfaceBinding(
        const tetrodotoxin_artifact_file_ops* operations,
        const ArtifactFile* owner,
        ttx_abstract requirement,
        ttx_abstract candidate)
        : ttx_interface_capability{&operations->interface},
          owner(owner),
          requirement(requirement),
          candidate(candidate) {}
    const ArtifactFile* owner;
    ttx_abstract requirement;
    ttx_abstract candidate;
  };

  struct RouteBinding {
    ttx_route_ops operations;
  };

  struct BytesBinding {
    ttx_bytes_ops operations;
  };

  static auto select(ttx_interface self) -> const InterfaceBinding&;
  static auto select(ttx_route self) -> const ArtifactFile&;
  static auto select(ttx_bytes self) -> const ArtifactFile&;

  static auto TTX_CALL interface_requirement(ttx_interface self)
      -> ttx_abstract;
  static auto TTX_CALL interface_candidate(ttx_interface self) -> ttx_abstract;
  static auto TTX_CALL interface_relation(ttx_interface self)
      -> ttx_interface_relation;
  static void TTX_CALL interface_invoke(
      ttx_interface self,
      ttx_abstract operation,
      ttx_pack input,
      ttx_context context,
      ttx_pack_result result);
  static auto TTX_CALL artifact_route(ttx_interface self) -> ttx_borrowed_bytes;
  static auto TTX_CALL artifact_size(ttx_interface self) -> uint64_t;
  static void TTX_CALL
      artifact_visit(ttx_interface self, ttx_bytes_sink result);
  static auto TTX_CALL artifact_executable(ttx_interface self) -> uint8_t;

  static auto TTX_CALL route_candidate(ttx_route self) -> ttx_abstract;
  static auto TTX_CALL route_bytes(ttx_route self) -> ttx_borrowed_bytes;
  static auto TTX_CALL bytes_candidate(ttx_bytes self) -> ttx_abstract;
  static auto TTX_CALL bytes_size(ttx_bytes self) -> uint64_t;
  static void TTX_CALL bytes_visit(ttx_bytes self, ttx_bytes_sink result);

  RouteBinding route_binding;
  BytesBinding bytes_binding;
  Ttx::Layouts::Value domain_layout;
};

}  // namespace Tetrodotoxin::Language
