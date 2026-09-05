// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <vector>

#include "tetrodotoxin/language/production.h"

namespace Tetrodotoxin::Language {

// A native producer can retain private owners beside a source production by
// placing them behind this lifetime boundary. The object never crosses the C
// ABI; its only observable consequence is keeping every borrowed request and
// operation table valid until the production handle is released.
class ProductionLifetime {
 public:
  virtual ~ProductionLifetime() = default;
};

// Production is the C++ owner behind one retained source-production handle.
// It takes ownership of the initial ProductRequest references supplied to its
// constructor and releases them before releasing the optional private lifetime.
class Production final {
 public:
  static auto create(
      std::vector<uint8_t> publication_root,
      std::vector<tetrodotoxin_product_request> requests,
      std::unique_ptr<ProductionLifetime> lifetime = {})
      -> std::optional<tetrodotoxin_source_production>;

  Production(const Production&) = delete;
  Production(Production&&) = delete;
  auto operator=(const Production&) -> Production& = delete;
  auto operator=(Production&&) -> Production& = delete;

 private:
  Production(
      std::vector<uint8_t> publication_root,
      std::vector<tetrodotoxin_product_request> requests,
      std::unique_ptr<ProductionLifetime> lifetime);
  ~Production();

  auto handle() -> tetrodotoxin_source_production;
  static auto select(tetrodotoxin_source_production_self* self) -> Production&;
  static void TTX_CALL retain(tetrodotoxin_source_production_self* self);
  static void TTX_CALL release(tetrodotoxin_source_production_self* self);
  static auto TTX_CALL publication_root(
      tetrodotoxin_source_production_self* self) -> ttx_borrowed_bytes;
  static void TTX_CALL visit_requests(
      tetrodotoxin_source_production_self* self,
      tetrodotoxin_product_request_sink result);

  static const tetrodotoxin_source_production_ops operations;

  std::atomic<uint64_t> references = 1;
  std::vector<uint8_t> output_root;
  std::unique_ptr<ProductionLifetime> retained_lifetime;
  std::vector<tetrodotoxin_product_request> product_requests;
};

}  // namespace Tetrodotoxin::Language
