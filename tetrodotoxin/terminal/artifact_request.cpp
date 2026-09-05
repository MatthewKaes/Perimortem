// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/artifact_request.hpp"

#include <new>
#include <utility>

#include "perimortem/core/view/bytes.hpp"

#include "ttx/query.hpp"

using namespace Tetrodotoxin;

static auto view(const std::vector<uint8_t>& bytes)
    -> Perimortem::Core::View::Bytes {
  return bytes.empty()
             ? Perimortem::Core::View::Bytes()
             : Perimortem::Core::View::Bytes(bytes.data(), bytes.size());
}

const tetrodotoxin_product_request_ops
    Terminal::ArtifactRequest::request_operations = {
      .header =
          {
            .size = TETRODOTOXIN_PRODUCT_REQUEST_PREFIX_SIZE,
            .abi_major = TTX_ABI_MAJOR,
            .abi_minor = TTX_ABI_MINOR,
          },
      .retain = retain,
      .release = release,
      .observe = observe,
      .cancel = cancel,
      .prepare_reobserve = nullptr,
};

const tetrodotoxin_production_closure_ops
    Terminal::ArtifactRequest::closure_operations = {
      .header =
          {
            .size = sizeof(tetrodotoxin_production_closure_ops),
            .abi_major = TTX_ABI_MAJOR,
            .abi_minor = TTX_ABI_MINOR,
          },
      .visit_constants = visit_constants,
      .visit_authorities = visit_authorities,
};

Terminal::ArtifactRequest::ArtifactRequest(
    std::vector<uint8_t> selected_route,
    std::vector<uint8_t> bytes,
    bool executable,
    tetrodotoxin_workspace_view selected_workspace)
    : artifact(
          Language::Product::create(
              arena,
              view(selected_route),
              view(bytes),
              Bool(executable))),
      route(std::move(selected_route)),
      artifact_layout(artifact.get_handle()),
      route_layout(route.get_abi()),
      named_layout(artifact_layout.get_abi(), route_layout.get_abi()),
      context(ttx_context_create()),
      products(),
      workspace(selected_workspace) {
  if (context.operations == nullptr) {
    return;
  }
  const Ttx::PackObservation retained =
      Ttx::pack(context, named_layout.get_abi());
  if (retained.state == Ttx::PackObservationState::Packed) {
    products = retained.pack;
  }
}

Terminal::ArtifactRequest::~ArtifactRequest() {
  if (context.operations != nullptr) {
    context.operations->release(context);
  }
}

auto Terminal::ArtifactRequest::valid() const -> bool {
  return context.operations != nullptr && products.operations != nullptr;
}

auto Terminal::ArtifactRequest::create(
    std::vector<uint8_t> route,
    std::vector<uint8_t> bytes,
    bool executable,
    tetrodotoxin_workspace_view workspace)
    -> std::optional<tetrodotoxin_product_request> {
  auto* created = new (std::nothrow) ArtifactRequest(
      std::move(route), std::move(bytes), executable, workspace);
  if (created == nullptr || !created->valid()) {
    delete created;
    return std::nullopt;
  }
  return created->request();
}

auto Terminal::ArtifactRequest::request() -> tetrodotoxin_product_request {
  return {
    .operations = &request_operations,
    .self = reinterpret_cast<tetrodotoxin_product_request_self*>(this),
  };
}

auto Terminal::ArtifactRequest::closure() -> tetrodotoxin_production_closure {
  return {
    .operations = &closure_operations,
    .self = reinterpret_cast<tetrodotoxin_production_closure_self*>(this),
  };
}

auto Terminal::ArtifactRequest::select(tetrodotoxin_product_request_self* self)
    -> ArtifactRequest& {
  return *reinterpret_cast<ArtifactRequest*>(self);
}

auto Terminal::ArtifactRequest::select(
    tetrodotoxin_production_closure_self* self) -> ArtifactRequest& {
  return *reinterpret_cast<ArtifactRequest*>(self);
}

void Terminal::ArtifactRequest::retain(
    tetrodotoxin_product_request_self* self) {
  select(self).references.fetch_add(1, std::memory_order_relaxed);
}

void Terminal::ArtifactRequest::release(
    tetrodotoxin_product_request_self* self) {
  ArtifactRequest& request = select(self);
  if (request.references.fetch_sub(1, std::memory_order_acq_rel) == 1) {
    delete &request;
  }
}

void Terminal::ArtifactRequest::observe(
    tetrodotoxin_product_request_self* self,
    tetrodotoxin_product_observation_result result) {
  ArtifactRequest& request = select(self);
  result.operations->produced(result.self, request.products, request.closure());
}

void Terminal::ArtifactRequest::cancel(
    tetrodotoxin_product_request_self*,
    tetrodotoxin_product_cancel_result result) {
  // Construction receives completed immutable bytes, so there is no pending
  // effect for cancellation to stop by the time a caller owns this request.
  result.operations->already_settled(result.self);
}

void Terminal::ArtifactRequest::visit_constants(
    tetrodotoxin_production_closure_self* self,
    tetrodotoxin_closure_constant_sink result) {
  ArtifactRequest& request = select(self);
  result.operations->constant(result.self, request.artifact.get_handle());
  result.operations->completed(result.self);
}

void Terminal::ArtifactRequest::visit_authorities(
    tetrodotoxin_production_closure_self* self,
    tetrodotoxin_closure_authority_sink result) {
  ArtifactRequest& request = select(self);
  if (request.workspace.operations != nullptr &&
      request.workspace.self != nullptr &&
      request.workspace.operations->header.abi_major == TTX_ABI_MAJOR &&
      request.workspace.operations->header.size >=
          sizeof(tetrodotoxin_workspace_view_ops) &&
      request.workspace.operations->visit_revisions != nullptr) {
    request.workspace.operations->visit_revisions(
        request.workspace.self, result);
    return;
  }
  result.operations->completed(result.self);
}
