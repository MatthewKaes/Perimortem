// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <vector>

#include "tetrodotoxin/language/provider.h"

namespace Tetrodotoxin::Language {

enum class InterpretationState { Constructed, Failed, Invalid };

struct InterpretationObservation {
  InterpretationState state;
  tetrodotoxin_source_graph graph;
  ttx_abstract error;
};

enum class ValidationState { Accepted, Incomplete, Failed, Invalid };

struct ValidationObservation {
  ValidationState state;
  ttx_abstract error;
};

enum class ProductionState { Unknown, None, Failed, Planned, Invalid };

struct ProductionObservation {
  ProductionState state;
  tetrodotoxin_source_production production;
  ttx_abstract error;
};

struct ProductionPlanObservation {
  bool valid;
  std::vector<uint8_t> publication_root;
  std::vector<tetrodotoxin_product_request> requests;
  ttx_abstract error;
};

// ProductionSink enforces one synchronous outcome without turning that
// operation result into a graph identity. Language, Build, and Puffer can share
// the carrier while each concrete producer still owns the meaning of failure
// and of the Pack it returns.
class ProductionSink {
 public:
  ProductionSink();

  auto get_handle() -> tetrodotoxin_production_result;
  constexpr auto get_observation() const -> ProductionObservation {
    return observation;
  }

 private:
  static auto select(tetrodotoxin_production_result_self* self)
      -> ProductionSink&;
  auto answer(ProductionState state) -> ProductionObservation&;
  static void TTX_CALL unknown(tetrodotoxin_production_result_self* self);
  static void TTX_CALL none(tetrodotoxin_production_result_self* self);
  static void TTX_CALL
      failed(tetrodotoxin_production_result_self* self, ttx_abstract error);
  static void TTX_CALL planned(
      tetrodotoxin_production_result_self* self,
      tetrodotoxin_source_production production);

  tetrodotoxin_production_result_ops operations;
  ProductionObservation observation;
};

// These C++ helpers validate one synchronous result exchange without retaining
// its source input or result sink. The provider and returned graph keep their
// own lifetimes, so using the helpers does not add another semantic owner.
auto interpret(
    tetrodotoxin_dialect_provider provider,
    tetrodotoxin_source_input source,
    ttx_abstract context) -> InterpretationObservation;
auto validate(tetrodotoxin_source_graph graph) -> ValidationObservation;
auto produce(tetrodotoxin_source_graph graph, ttx_context context)
    -> ProductionObservation;
auto observe(tetrodotoxin_source_production production)
    -> ProductionPlanObservation;

}  // namespace Tetrodotoxin::Language
