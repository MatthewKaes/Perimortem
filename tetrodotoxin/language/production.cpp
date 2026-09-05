// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/production.hpp"

#include <new>
#include <utility>

using namespace Tetrodotoxin;

static void release_requests(
    std::vector<tetrodotoxin_product_request>& requests) {
  for (tetrodotoxin_product_request request : requests) {
    if (request.operations != nullptr && request.self != nullptr &&
        request.operations->release != nullptr) {
      request.operations->release(request.self);
    }
  }
  requests.clear();
}

const tetrodotoxin_source_production_ops Language::Production::operations = {
  .header =
      {
        .size = sizeof(tetrodotoxin_source_production_ops),
        .abi_major = TTX_ABI_MAJOR,
        .abi_minor = TTX_ABI_MINOR,
      },
  .retain = retain,
  .release = release,
  .publication_root = publication_root,
  .visit_requests = visit_requests,
};

Language::Production::Production(
    std::vector<uint8_t> publication_root,
    std::vector<tetrodotoxin_product_request> requests,
    std::unique_ptr<ProductionLifetime> lifetime)
    : output_root(std::move(publication_root)),
      retained_lifetime(std::move(lifetime)),
      product_requests(std::move(requests)) {}

Language::Production::~Production() {
  for (tetrodotoxin_product_request request : product_requests) {
    request.operations->release(request.self);
  }
}

auto Language::Production::create(
    std::vector<uint8_t> publication_root,
    std::vector<tetrodotoxin_product_request> requests,
    std::unique_ptr<ProductionLifetime> lifetime)
    -> std::optional<tetrodotoxin_source_production> {
  if (publication_root.empty() || requests.empty()) {
    release_requests(requests);
    return std::nullopt;
  }
  for (tetrodotoxin_product_request request : requests) {
    if (request.operations == nullptr || request.self == nullptr ||
        request.operations->header.abi_major != TTX_ABI_MAJOR ||
        request.operations->header.size <
            TETRODOTOXIN_PRODUCT_REQUEST_PREFIX_SIZE ||
        request.operations->release == nullptr) {
      release_requests(requests);
      return std::nullopt;
    }
  }
  auto* created = new (std::nothrow) Production(
      std::move(publication_root), std::move(requests), std::move(lifetime));
  if (created == nullptr) {
    release_requests(requests);
    return std::nullopt;
  }
  return created->handle();
}

auto Language::Production::handle() -> tetrodotoxin_source_production {
  return {
    .operations = &operations,
    .self = reinterpret_cast<tetrodotoxin_source_production_self*>(this),
  };
}

auto Language::Production::select(tetrodotoxin_source_production_self* self)
    -> Production& {
  return *reinterpret_cast<Production*>(self);
}

void Language::Production::retain(tetrodotoxin_source_production_self* self) {
  select(self).references.fetch_add(1, std::memory_order_relaxed);
}

void Language::Production::release(tetrodotoxin_source_production_self* self) {
  Production& production = select(self);
  if (production.references.fetch_sub(1, std::memory_order_acq_rel) == 1) {
    delete &production;
  }
}

auto Language::Production::publication_root(
    tetrodotoxin_source_production_self* self) -> ttx_borrowed_bytes {
  const std::vector<uint8_t>& root = select(self).output_root;
  return {.data = root.data(), .size = root.size()};
}

void Language::Production::visit_requests(
    tetrodotoxin_source_production_self* self,
    tetrodotoxin_product_request_sink result) {
  for (tetrodotoxin_product_request request : select(self).product_requests) {
    result.operations->request(result.self, request);
  }
  result.operations->completed(result.self);
}
