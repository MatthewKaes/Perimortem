// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/query.hpp"

using namespace Tetrodotoxin::Language;

struct ProductionPlanCapture {
  tetrodotoxin_product_request_sink_ops operations;
  ProductionPlanObservation observation;
  bool completed;
};

struct InterpretationCapture {
  InterpretationObservation observation = {
    InterpretationState::Invalid,
    {},
    {}};
  bool answered = false;
};

struct ValidationCapture {
  ValidationObservation observation = {ValidationState::Invalid, {}};
  bool answered = false;
};

template <typename Operations>
static auto supports(const Operations* operations, uint32_t size) -> bool {
  return operations != nullptr &&
         operations->header.abi_major == TTX_ABI_MAJOR &&
         operations->header.size >= size;
}

static auto interpretation(tetrodotoxin_interpret_result_self* self)
    -> InterpretationCapture& {
  return *reinterpret_cast<InterpretationCapture*>(self);
}

static void TTX_CALL constructed(
    tetrodotoxin_interpret_result_self* self,
    tetrodotoxin_source_graph graph) {
  auto& capture = interpretation(self);
  auto& observation = capture.observation;
  const bool valid =
      supports(graph.operations, sizeof(tetrodotoxin_source_graph_ops)) &&
      graph.self != nullptr && graph.operations->retain != nullptr &&
      graph.operations->release != nullptr &&
      graph.operations->root != nullptr &&
      graph.operations->dialect != nullptr &&
      graph.operations->visit_dependencies != nullptr &&
      graph.operations->validate != nullptr &&
      graph.operations->produce != nullptr &&
      graph.operations->visit_associations != nullptr &&
      graph.operations->visit_diagnostics != nullptr;
  const bool repeated = capture.answered;
  capture.answered = true;
  if (repeated || !valid) {
    // Construction transfers ownership even if a later service is missing.
    // Release through the supplied table when its complete ABI is available
    // so rejecting a malformed graph does not strand its input and storage.
    if (supports(graph.operations, sizeof(tetrodotoxin_source_graph_ops)) &&
        graph.self && graph.operations->release) {
      graph.operations->release(graph.self);
    }
    if (observation.graph.operations != nullptr) {
      observation.graph.operations->release(observation.graph.self);
      observation.graph = {};
    }
    observation.state = InterpretationState::Invalid;
    return;
  }
  observation.state = InterpretationState::Constructed;
  observation.graph = graph;
}

static void TTX_CALL interpretation_failed(
    tetrodotoxin_interpret_result_self* self,
    ttx_abstract error) {
  auto& capture = interpretation(self);
  auto& observation = capture.observation;
  const bool repeated = capture.answered;
  capture.answered = true;
  if (repeated || error == nullptr || error->operations == nullptr) {
    if (observation.graph.operations != nullptr) {
      observation.graph.operations->release(observation.graph.self);
      observation.graph = {};
    }
    observation.state = InterpretationState::Invalid;
    return;
  }
  observation.state = InterpretationState::Failed;
  observation.error = error;
}

auto Tetrodotoxin::Language::interpret(
    tetrodotoxin_dialect_provider provider,
    tetrodotoxin_source_input source,
    ttx_abstract context) -> InterpretationObservation {
  InterpretationCapture capture;
  if (!supports(
          provider.operations, sizeof(tetrodotoxin_dialect_provider_ops)) ||
      provider.self == nullptr || provider.operations->interpret == nullptr ||
      !supports(source.operations, sizeof(tetrodotoxin_source_input_ops)) ||
      source.self == nullptr || source.operations->bytes == nullptr ||
      source.operations->diagnostic_path == nullptr ||
      source.operations->retain == nullptr ||
      source.operations->release == nullptr || context == nullptr ||
      context->operations == nullptr) {
    return capture.observation;
  }
  static const tetrodotoxin_interpret_result_ops operations = {
    .header =
        {
          .size = sizeof(tetrodotoxin_interpret_result_ops),
          .abi_major = TTX_ABI_MAJOR,
          .abi_minor = TTX_ABI_MINOR,
        },
    .constructed = constructed,
    .failed = interpretation_failed,
  };
  provider.operations->interpret(
      provider.self, source, context,
      {
        .operations = &operations,
        .self = reinterpret_cast<tetrodotoxin_interpret_result_self*>(&capture),
      });
  return capture.observation;
}

static auto validation(tetrodotoxin_validation_result_self* self)
    -> ValidationCapture& {
  return *reinterpret_cast<ValidationCapture*>(self);
}

static auto validation_answer(
    tetrodotoxin_validation_result_self* self,
    ValidationState state) -> ValidationObservation& {
  auto& capture = validation(self);
  capture.observation.state =
      capture.answered ? ValidationState::Invalid : state;
  capture.answered = true;
  return capture.observation;
}

static void TTX_CALL accepted(tetrodotoxin_validation_result_self* self) {
  validation_answer(self, ValidationState::Accepted);
}

static void TTX_CALL incomplete(tetrodotoxin_validation_result_self* self) {
  validation_answer(self, ValidationState::Incomplete);
}

static void TTX_CALL validation_failed(
    tetrodotoxin_validation_result_self* self,
    ttx_abstract error) {
  ValidationObservation& observation =
      validation_answer(self, ValidationState::Failed);
  if (error == nullptr || error->operations == nullptr) {
    observation.state = ValidationState::Invalid;
  }
  observation.error = error;
}

auto Tetrodotoxin::Language::validate(tetrodotoxin_source_graph graph)
    -> ValidationObservation {
  ValidationCapture capture;
  if (!supports(graph.operations, sizeof(tetrodotoxin_source_graph_ops)) ||
      graph.self == nullptr || graph.operations->validate == nullptr) {
    return capture.observation;
  }
  static const tetrodotoxin_validation_result_ops operations = {
    .header =
        {
          .size = sizeof(tetrodotoxin_validation_result_ops),
          .abi_major = TTX_ABI_MAJOR,
          .abi_minor = TTX_ABI_MINOR,
        },
    .accepted = accepted,
    .incomplete = incomplete,
    .failed = validation_failed,
  };
  graph.operations->validate(
      graph.self,
      {
        .operations = &operations,
        .self =
            reinterpret_cast<tetrodotoxin_validation_result_self*>(&capture),
      });
  return capture.observation;
}

ProductionSink::ProductionSink()
    : operations({
        .header =
            {
              .size = sizeof(tetrodotoxin_production_result_ops),
              .abi_major = TTX_ABI_MAJOR,
              .abi_minor = TTX_ABI_MINOR,
            },
        .unknown = unknown,
        .none = none,
        .failed = failed,
        .planned = planned,
      }),
      observation({
        .state = ProductionState::Invalid,
        .production = {},
        .error = {},
      }) {}

auto ProductionSink::get_handle() -> tetrodotoxin_production_result {
  return {
    .operations = &operations,
    .self = reinterpret_cast<tetrodotoxin_production_result_self*>(this),
  };
}

auto ProductionSink::select(tetrodotoxin_production_result_self* self)
    -> ProductionSink& {
  return *reinterpret_cast<ProductionSink*>(self);
}

auto ProductionSink::answer(ProductionState state) -> ProductionObservation& {
  observation.state = observation.state == ProductionState::Invalid
                          ? state
                          : ProductionState::Invalid;
  return observation;
}

void TTX_CALL
    ProductionSink::unknown(tetrodotoxin_production_result_self* self) {
  select(self).answer(ProductionState::Unknown);
}

void TTX_CALL ProductionSink::none(tetrodotoxin_production_result_self* self) {
  select(self).answer(ProductionState::None);
}

void TTX_CALL ProductionSink::failed(
    tetrodotoxin_production_result_self* self,
    ttx_abstract error) {
  ProductionObservation& observation =
      select(self).answer(ProductionState::Failed);
  if (error == nullptr || error->operations == nullptr) {
    observation.state = ProductionState::Invalid;
  }
  observation.error = error;
}

void TTX_CALL ProductionSink::planned(
    tetrodotoxin_production_result_self* self,
    tetrodotoxin_source_production production) {
  ProductionObservation& observation =
      select(self).answer(ProductionState::Planned);
  if (!supports(
          production.operations, sizeof(tetrodotoxin_source_production_ops)) ||
      production.self == nullptr || production.operations->retain == nullptr ||
      production.operations->release == nullptr ||
      production.operations->publication_root == nullptr ||
      production.operations->visit_requests == nullptr) {
    observation.state = ProductionState::Invalid;
  }
  observation.production = production;
}

auto Tetrodotoxin::Language::produce(
    tetrodotoxin_source_graph graph,
    ttx_context context) -> ProductionObservation {
  ProductionSink result;
  if (!supports(graph.operations, sizeof(tetrodotoxin_source_graph_ops)) ||
      graph.self == nullptr || graph.operations->produce == nullptr ||
      !supports(context.operations, sizeof(ttx_context_ops))) {
    return result.get_observation();
  }
  graph.operations->produce(graph.self, context, result.get_handle());
  return result.get_observation();
}

static auto select(tetrodotoxin_product_request_sink_self* self)
    -> ProductionPlanCapture& {
  return *reinterpret_cast<ProductionPlanCapture*>(self);
}

static void TTX_CALL production_request(
    tetrodotoxin_product_request_sink_self* self,
    tetrodotoxin_product_request request) {
  ProductionPlanCapture& capture = select(self);
  if (capture.completed || capture.observation.error != nullptr ||
      !supports(request.operations, TETRODOTOXIN_PRODUCT_REQUEST_PREFIX_SIZE) ||
      request.self == nullptr || request.operations->observe == nullptr ||
      request.operations->cancel == nullptr ||
      request.operations->retain == nullptr ||
      request.operations->release == nullptr) {
    capture.observation.valid = false;
    return;
  }
  capture.observation.requests.push_back(request);
}

static void TTX_CALL
    production_completed(tetrodotoxin_product_request_sink_self* self) {
  ProductionPlanCapture& capture = select(self);
  if (capture.completed || capture.observation.error != nullptr) {
    capture.observation.valid = false;
  }
  capture.completed = true;
}

static void TTX_CALL production_failed(
    tetrodotoxin_product_request_sink_self* self,
    ttx_abstract error) {
  ProductionPlanCapture& capture = select(self);
  if (capture.completed || capture.observation.error != nullptr ||
      error == nullptr || error->operations == nullptr) {
    capture.observation.valid = false;
    return;
  }
  capture.completed = true;
  capture.observation.error = error;
}

auto Tetrodotoxin::Language::observe(tetrodotoxin_source_production production)
    -> ProductionPlanObservation {
  ProductionPlanCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(tetrodotoxin_product_request_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .request = production_request,
          .completed = production_completed,
          .failed = production_failed,
        },
    .observation =
        {
          .valid = false,
          .publication_root = {},
          .requests = {},
          .error = {},
        },
    .completed = false,
  };
  if (!supports(
          production.operations, sizeof(tetrodotoxin_source_production_ops)) ||
      production.self == nullptr ||
      production.operations->publication_root == nullptr ||
      production.operations->visit_requests == nullptr) {
    return capture.observation;
  }
  const ttx_borrowed_bytes root =
      production.operations->publication_root(production.self);
  if (root.size == 0 || root.data == nullptr) {
    return capture.observation;
  }
  capture.observation.publication_root.assign(root.data, root.data + root.size);
  capture.observation.valid = true;
  production.operations->visit_requests(
      production.self,
      {
        .operations = &capture.operations,
        .self =
            reinterpret_cast<tetrodotoxin_product_request_sink_self*>(&capture),
      });
  if (!capture.completed || capture.observation.requests.empty() ||
      capture.observation.error != nullptr) {
    capture.observation.valid = false;
  }
  return std::move(capture.observation);
}
