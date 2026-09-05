// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/query.hpp"

#include <utility>

#include "ttx/query.hpp"

using namespace Tetrodotoxin::Terminal;

template <typename Operations>
static auto supports(const Operations* operations, uint32_t size) -> bool {
  return operations != nullptr &&
         operations->header.abi_major == TTX_ABI_MAJOR &&
         operations->header.size >= size;
}

static auto select(tetrodotoxin_terminal_begin_result_self* self)
    -> BeginObservation& {
  return *reinterpret_cast<BeginObservation*>(self);
}

static void TTX_CALL begin_requested(
    tetrodotoxin_terminal_begin_result_self* self,
    tetrodotoxin_product_request request) {
  BeginObservation& observation = select(self);
  const bool valid =
      supports(request.operations, TETRODOTOXIN_PRODUCT_REQUEST_PREFIX_SIZE) &&
      request.self != nullptr && request.operations->retain != nullptr &&
      request.operations->release != nullptr &&
      request.operations->observe != nullptr &&
      request.operations->cancel != nullptr;
  observation.state = observation.state == BeginState::Invalid && valid
                          ? BeginState::Requested
                          : BeginState::Invalid;
  observation.request = request;
}

static void TTX_CALL begin_failed(
    tetrodotoxin_terminal_begin_result_self* self,
    ttx_abstract error) {
  BeginObservation& observation = select(self);
  observation.state =
      observation.state == BeginState::Invalid && error.operations != nullptr
          ? BeginState::Failed
          : BeginState::Invalid;
  observation.error = error;
}

auto Tetrodotoxin::Terminal::begin(
    tetrodotoxin_terminal_provider provider,
    ttx_borrowed_bytes output_route,
    ttx_abstract product,
    tetrodotoxin_workspace_view workspace,
    ttx_abstract environment,
    ttx_abstract invocation) -> BeginObservation {
  BeginObservation observation = {
    .state = BeginState::Invalid,
    .request = {},
    .error = {},
  };
  if (!supports(
          provider.operations, sizeof(tetrodotoxin_terminal_provider_ops))) {
    return observation;
  }
  const tetrodotoxin_terminal_begin_result_ops operations = {
    .header =
        {
          .size = sizeof(tetrodotoxin_terminal_begin_result_ops),
          .abi_major = TTX_ABI_MAJOR,
          .abi_minor = TTX_ABI_MINOR,
        },
    .requested = begin_requested,
    .failed = begin_failed,
  };
  provider.operations->begin(
      provider.self, output_route, product, workspace, environment, invocation,
      {
        .operations = &operations,
        .self = reinterpret_cast<tetrodotoxin_terminal_begin_result_self*>(
            &observation),
      });
  return observation;
}

static auto select(tetrodotoxin_product_observation_result_self* self)
    -> ProductObservation& {
  return *reinterpret_cast<ProductObservation*>(self);
}

static auto answer(
    tetrodotoxin_product_observation_result_self* self,
    ProductState state) -> ProductObservation& {
  ProductObservation& observation = select(self);
  observation.state = observation.state == ProductState::Invalid
                          ? state
                          : ProductState::Invalid;
  return observation;
}

static void TTX_CALL
    product_unknown(tetrodotoxin_product_observation_result_self* self) {
  answer(self, ProductState::Unknown);
}

static void TTX_CALL
    product_none(tetrodotoxin_product_observation_result_self* self) {
  answer(self, ProductState::None);
}

static void TTX_CALL product_failed(
    tetrodotoxin_product_observation_result_self* self,
    ttx_abstract error) {
  ProductObservation& observation = answer(self, ProductState::Failed);
  if (error.operations == nullptr) {
    observation.state = ProductState::Invalid;
  }
  observation.error = error;
}

static void TTX_CALL product_produced(
    tetrodotoxin_product_observation_result_self* self,
    ttx_pack products,
    tetrodotoxin_production_closure closure) {
  ProductObservation& observation = answer(self, ProductState::Produced);
  if (!supports(products.operations, sizeof(ttx_pack_ops)) ||
      !supports(
          closure.operations, TETRODOTOXIN_PRODUCTION_CLOSURE_PREFIX_SIZE)) {
    observation.state = ProductState::Invalid;
  }
  observation.products = products;
  observation.closure = closure;
}

auto Tetrodotoxin::Terminal::observe(tetrodotoxin_product_request request)
    -> ProductObservation {
  ProductObservation observation = {
    .state = ProductState::Invalid,
    .products = {},
    .closure = {},
    .error = {},
  };
  if (!supports(request.operations, TETRODOTOXIN_PRODUCT_REQUEST_PREFIX_SIZE) ||
      request.self == nullptr || request.operations->observe == nullptr) {
    return observation;
  }
  const tetrodotoxin_product_observation_result_ops operations = {
    .header =
        {
          .size = sizeof(tetrodotoxin_product_observation_result_ops),
          .abi_major = TTX_ABI_MAJOR,
          .abi_minor = TTX_ABI_MINOR,
        },
    .unknown = product_unknown,
    .none = product_none,
    .failed = product_failed,
    .produced = product_produced,
  };
  request.operations->observe(
      request.self,
      {
        .operations = &operations,
        .self = reinterpret_cast<tetrodotoxin_product_observation_result_self*>(
            &observation),
      });
  return observation;
}

static auto select(tetrodotoxin_product_cancel_result_self* self)
    -> CancelObservation& {
  return *reinterpret_cast<CancelObservation*>(self);
}

static auto cancel_answer(
    tetrodotoxin_product_cancel_result_self* self,
    CancelState state) -> CancelObservation& {
  CancelObservation& observation = select(self);
  observation.state =
      observation.state == CancelState::Invalid ? state : CancelState::Invalid;
  return observation;
}

static void TTX_CALL
    product_cancelled(tetrodotoxin_product_cancel_result_self* self) {
  cancel_answer(self, CancelState::Cancelled);
}

static void TTX_CALL
    product_already_settled(tetrodotoxin_product_cancel_result_self* self) {
  cancel_answer(self, CancelState::AlreadySettled);
}

static void TTX_CALL cancel_failed(
    tetrodotoxin_product_cancel_result_self* self,
    ttx_abstract error) {
  CancelObservation& observation = cancel_answer(self, CancelState::Failed);
  if (error.operations == nullptr) {
    observation.state = CancelState::Invalid;
  }
  observation.error = error;
}

auto Tetrodotoxin::Terminal::cancel(tetrodotoxin_product_request request)
    -> CancelObservation {
  CancelObservation observation = {
    .state = CancelState::Invalid,
    .error = {},
  };
  if (!supports(request.operations, TETRODOTOXIN_PRODUCT_REQUEST_PREFIX_SIZE) ||
      request.self == nullptr || request.operations->cancel == nullptr) {
    return observation;
  }
  const tetrodotoxin_product_cancel_result_ops operations = {
    .header =
        {
          .size = sizeof(tetrodotoxin_product_cancel_result_ops),
          .abi_major = TTX_ABI_MAJOR,
          .abi_minor = TTX_ABI_MINOR,
        },
    .cancelled = product_cancelled,
    .already_settled = product_already_settled,
    .failed = cancel_failed,
  };
  request.operations->cancel(
      request.self,
      {
        .operations = &operations,
        .self = reinterpret_cast<tetrodotoxin_product_cancel_result_self*>(
            &observation),
      });
  return observation;
}

static auto select(tetrodotoxin_reobserve_prepare_result_self* self)
    -> ReobservePrepareObservation& {
  return *reinterpret_cast<ReobservePrepareObservation*>(self);
}

static void TTX_CALL reobserve_prepared(
    tetrodotoxin_reobserve_prepare_result_self* self,
    tetrodotoxin_reobserve reobserve) {
  ReobservePrepareObservation& observation = select(self);
  const bool valid =
      supports(reobserve.operations, sizeof(tetrodotoxin_reobserve_ops)) &&
      reobserve.self != nullptr && reobserve.operations->commit != nullptr &&
      reobserve.operations->close != nullptr;
  observation.state =
      observation.state == ReobservePrepareState::Invalid && valid
          ? ReobservePrepareState::Prepared
          : ReobservePrepareState::Invalid;
  observation.reobserve = reobserve;
}

static void TTX_CALL reobserve_prepare_failed(
    tetrodotoxin_reobserve_prepare_result_self* self,
    ttx_abstract error) {
  ReobservePrepareObservation& observation = select(self);
  observation.state = observation.state == ReobservePrepareState::Invalid &&
                              error.operations != nullptr
                          ? ReobservePrepareState::Failed
                          : ReobservePrepareState::Invalid;
  observation.error = error;
}

auto Tetrodotoxin::Terminal::prepare_reobserve(
    tetrodotoxin_product_request request,
    tetrodotoxin_reobserve_callback callback) -> ReobservePrepareObservation {
  ReobservePrepareObservation observation = {
    .state = ReobservePrepareState::Invalid,
    .reobserve = {},
    .error = {},
  };
  if (!supports(request.operations, sizeof(tetrodotoxin_product_request_ops)) ||
      request.self == nullptr ||
      request.operations->prepare_reobserve == nullptr) {
    observation.state = ReobservePrepareState::Unavailable;
    return observation;
  }
  if (!supports(
          callback.operations, sizeof(tetrodotoxin_reobserve_callback_ops)) ||
      callback.self == nullptr || callback.operations->changed == nullptr) {
    return observation;
  }
  const tetrodotoxin_reobserve_prepare_result_ops operations = {
    .header =
        {
          .size = sizeof(tetrodotoxin_reobserve_prepare_result_ops),
          .abi_major = TTX_ABI_MAJOR,
          .abi_minor = TTX_ABI_MINOR,
        },
    .prepared = reobserve_prepared,
    .failed = reobserve_prepare_failed,
  };
  request.operations->prepare_reobserve(
      request.self, callback,
      {
        .operations = &operations,
        .self = reinterpret_cast<tetrodotoxin_reobserve_prepare_result_self*>(
            &observation),
      });
  return observation;
}

static auto select(tetrodotoxin_reobserve_commit_result_self* self)
    -> ReobserveCommitObservation& {
  return *reinterpret_cast<ReobserveCommitObservation*>(self);
}

static auto commit_answer(
    tetrodotoxin_reobserve_commit_result_self* self,
    ReobserveCommitState state) -> ReobserveCommitObservation& {
  ReobserveCommitObservation& observation = select(self);
  observation.state = observation.state == ReobserveCommitState::Invalid
                          ? state
                          : ReobserveCommitState::Invalid;
  return observation;
}

static void TTX_CALL
    reobserve_changed(tetrodotoxin_reobserve_commit_result_self* self) {
  commit_answer(self, ReobserveCommitState::Changed);
}

static void TTX_CALL
    reobserve_armed(tetrodotoxin_reobserve_commit_result_self* self) {
  commit_answer(self, ReobserveCommitState::Armed);
}

static void TTX_CALL reobserve_commit_failed(
    tetrodotoxin_reobserve_commit_result_self* self,
    ttx_abstract error) {
  ReobserveCommitObservation& observation =
      commit_answer(self, ReobserveCommitState::Failed);
  if (error.operations == nullptr) {
    observation.state = ReobserveCommitState::Invalid;
  }
  observation.error = error;
}

auto Tetrodotoxin::Terminal::commit_reobserve(tetrodotoxin_reobserve reobserve)
    -> ReobserveCommitObservation {
  ReobserveCommitObservation observation = {
    .state = ReobserveCommitState::Invalid,
    .error = {},
  };
  if (!supports(reobserve.operations, sizeof(tetrodotoxin_reobserve_ops)) ||
      reobserve.self == nullptr || reobserve.operations->commit == nullptr) {
    return observation;
  }
  const tetrodotoxin_reobserve_commit_result_ops operations = {
    .header =
        {
          .size = sizeof(tetrodotoxin_reobserve_commit_result_ops),
          .abi_major = TTX_ABI_MAJOR,
          .abi_minor = TTX_ABI_MINOR,
        },
    .changed = reobserve_changed,
    .armed = reobserve_armed,
    .failed = reobserve_commit_failed,
  };
  reobserve.operations->commit(
      reobserve.self,
      {
        .operations = &operations,
        .self = reinterpret_cast<tetrodotoxin_reobserve_commit_result_self*>(
            &observation),
      });
  return observation;
}

static auto select(tetrodotoxin_reobserve_close_result_self* self)
    -> ReobserveCloseObservation& {
  return *reinterpret_cast<ReobserveCloseObservation*>(self);
}

static void TTX_CALL
    reobserve_closed(tetrodotoxin_reobserve_close_result_self* self) {
  ReobserveCloseObservation& observation = select(self);
  observation.state = observation.state == ReobserveCloseState::Invalid
                          ? ReobserveCloseState::Closed
                          : ReobserveCloseState::Invalid;
}

static void TTX_CALL reobserve_close_failed(
    tetrodotoxin_reobserve_close_result_self* self,
    ttx_abstract error) {
  ReobserveCloseObservation& observation = select(self);
  observation.state = observation.state == ReobserveCloseState::Invalid &&
                              error.operations != nullptr
                          ? ReobserveCloseState::Failed
                          : ReobserveCloseState::Invalid;
  observation.error = error;
}

auto Tetrodotoxin::Terminal::close_reobserve(tetrodotoxin_reobserve reobserve)
    -> ReobserveCloseObservation {
  ReobserveCloseObservation observation = {
    .state = ReobserveCloseState::Invalid,
    .error = {},
  };
  if (!supports(reobserve.operations, sizeof(tetrodotoxin_reobserve_ops)) ||
      reobserve.self == nullptr || reobserve.operations->close == nullptr) {
    return observation;
  }
  const tetrodotoxin_reobserve_close_result_ops operations = {
    .header =
        {
          .size = sizeof(tetrodotoxin_reobserve_close_result_ops),
          .abi_major = TTX_ABI_MAJOR,
          .abi_minor = TTX_ABI_MINOR,
        },
    .closed = reobserve_closed,
    .failed = reobserve_close_failed,
  };
  reobserve.operations->close(
      reobserve.self,
      {
        .operations = &operations,
        .self = reinterpret_cast<tetrodotoxin_reobserve_close_result_self*>(
            &observation),
      });
  return observation;
}

struct ClosureCapture {
  tetrodotoxin_closure_constant_sink_ops constant_operations;
  tetrodotoxin_closure_authority_sink_ops authority_operations;
  ClosureObservation observation;
  bool constants_completed;
  bool authorities_completed;
};

static auto select(tetrodotoxin_closure_constant_sink_self* self)
    -> ClosureCapture& {
  return *reinterpret_cast<ClosureCapture*>(self);
}

static void TTX_CALL closure_constant(
    tetrodotoxin_closure_constant_sink_self* self,
    ttx_abstract constant) {
  ClosureCapture& capture = select(self);
  if (capture.constants_completed || constant.operations == nullptr) {
    capture.observation.valid = false;
    return;
  }
  const ttx_interface_relation relationship =
      Ttx::relation(constant, ttx_constant_requirement());
  if (relationship != TTX_INTERFACE_SATISFIED &&
      relationship != TTX_INTERFACE_EQUIVALENT) {
    capture.observation.valid = false;
    return;
  }
  capture.observation.constants.push_back(constant);
}

static void TTX_CALL
    closure_constants_completed(tetrodotoxin_closure_constant_sink_self* self) {
  ClosureCapture& capture = select(self);
  if (capture.constants_completed) {
    capture.observation.valid = false;
  }
  capture.constants_completed = true;
}

static auto select(tetrodotoxin_closure_authority_sink_self* self)
    -> ClosureCapture& {
  return *reinterpret_cast<ClosureCapture*>(self);
}

static void TTX_CALL closure_authority(
    tetrodotoxin_closure_authority_sink_self* self,
    tetrodotoxin_authority_revision authority) {
  ClosureCapture& capture = select(self);
  if (capture.authorities_completed ||
      !supports(
          authority.operations, sizeof(tetrodotoxin_authority_revision_ops)) ||
      authority.self == nullptr ||
      authority.operations->revision == nullptr ||
      authority.operations->is_current == nullptr) {
    capture.observation.valid = false;
    return;
  }
  const AuthorityRevisionObservation observed = {
    .authority = authority,
    .revision = authority.operations->revision(authority.self),
    .current = authority.operations->is_current(authority.self) != 0,
  };
  if (observed.revision == 0) {
    capture.observation.valid = false;
    return;
  }
  capture.observation.authorities.push_back(observed);
}

static void TTX_CALL closure_authorities_completed(
    tetrodotoxin_closure_authority_sink_self* self) {
  ClosureCapture& capture = select(self);
  if (capture.authorities_completed) {
    capture.observation.valid = false;
  }
  capture.authorities_completed = true;
}

auto Tetrodotoxin::Terminal::observe_closure(
    tetrodotoxin_production_closure closure) -> ClosureObservation {
  ClosureCapture capture = {
    .constant_operations =
        {
          .header =
              {
                .size = sizeof(tetrodotoxin_closure_constant_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .constant = closure_constant,
          .completed = closure_constants_completed,
        },
    .authority_operations =
        {
          .header =
              {
                .size = sizeof(tetrodotoxin_closure_authority_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .authority = closure_authority,
          .completed = closure_authorities_completed,
        },
    .observation = {.valid = true, .constants = {}, .authorities = {}},
    .constants_completed = false,
    .authorities_completed = false,
  };
  if (!supports(
          closure.operations, TETRODOTOXIN_PRODUCTION_CLOSURE_PREFIX_SIZE) ||
      closure.self == nullptr ||
      closure.operations->visit_constants == nullptr) {
    capture.observation.valid = false;
    return capture.observation;
  }
  closure.operations->visit_constants(
      closure.self,
      {
        .operations = &capture.constant_operations,
        .self = reinterpret_cast<tetrodotoxin_closure_constant_sink_self*>(
            &capture),
      });
  if (!capture.constants_completed) {
    capture.observation.valid = false;
  }

  if (closure.operations->header.size >=
          sizeof(tetrodotoxin_production_closure_ops) &&
      closure.operations->visit_authorities != nullptr) {
    closure.operations->visit_authorities(
        closure.self,
        {
          .operations = &capture.authority_operations,
          .self = reinterpret_cast<tetrodotoxin_closure_authority_sink_self*>(
              &capture),
        });
    if (!capture.authorities_completed) {
      capture.observation.valid = false;
    }
  } else {
    capture.authorities_completed = true;
  }
  return std::move(capture.observation);
}
