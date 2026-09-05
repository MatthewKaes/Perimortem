// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <atomic>
#include <optional>
#include <vector>

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/environment/provider.h"
#include "tetrodotoxin/language/product.hpp"
#include "tetrodotoxin/terminal/provider.h"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/route.hpp"
#include "ttx/model/layouts/value.hpp"

namespace Tetrodotoxin::Terminal {

// ArtifactRequest closes one already completed byte product over the ordinary
// TTX support model. The request owns its ArtifactFile identity and Context,
// allowing the returned Pack to borrow exact route and byte producers until
// publication releases the request. Terminal implementations supply only the
// bytes they derived instead of rebuilding Pack and Layout operation tables.
class ArtifactRequest {
 public:
  static auto create(
      std::vector<uint8_t> route,
      std::vector<uint8_t> bytes,
      bool executable,
      tetrodotoxin_workspace_view workspace = {})
      -> std::optional<tetrodotoxin_product_request>;

  ArtifactRequest(const ArtifactRequest&) = delete;
  ArtifactRequest(ArtifactRequest&&) = delete;
  auto operator=(const ArtifactRequest&) -> ArtifactRequest& = delete;
  auto operator=(ArtifactRequest&&) -> ArtifactRequest& = delete;

 private:
  ArtifactRequest(
      std::vector<uint8_t> route,
      std::vector<uint8_t> bytes,
      bool executable,
      tetrodotoxin_workspace_view workspace);
  ~ArtifactRequest();

  auto valid() const -> bool;
  auto request() -> tetrodotoxin_product_request;
  auto closure() -> tetrodotoxin_production_closure;

  static auto select(tetrodotoxin_product_request_self* self)
      -> ArtifactRequest&;
  static auto select(tetrodotoxin_production_closure_self* self)
      -> ArtifactRequest&;
  static void TTX_CALL retain(tetrodotoxin_product_request_self* self);
  static void TTX_CALL release(tetrodotoxin_product_request_self* self);
  static void TTX_CALL observe(
      tetrodotoxin_product_request_self* self,
      tetrodotoxin_product_observation_result result);
  static void TTX_CALL cancel(
      tetrodotoxin_product_request_self* self,
      tetrodotoxin_product_cancel_result result);
  static void TTX_CALL visit_constants(
      tetrodotoxin_production_closure_self* self,
      tetrodotoxin_closure_constant_sink result);
  static void TTX_CALL visit_authorities(
      tetrodotoxin_production_closure_self* self,
      tetrodotoxin_closure_authority_sink result);

  static const tetrodotoxin_product_request_ops request_operations;
  static const tetrodotoxin_production_closure_ops closure_operations;

  std::atomic<uint64_t> references = 1;
  Perimortem::Memory::Allocator::Arena arena;
  Tetrodotoxin::Language::Product& artifact;
  Ttx::Route route;
  Ttx::Layouts::Value artifact_layout;
  Ttx::Layouts::Value route_layout;
  Ttx::Layouts::Named named_layout;
  ttx_context context;
  ttx_pack products;
  tetrodotoxin_workspace_view workspace;
};

}  // namespace Tetrodotoxin::Terminal
