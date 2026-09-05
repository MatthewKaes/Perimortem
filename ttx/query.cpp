// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/query.hpp"

#include <cstddef>
#include <limits>
#include <utility>

using namespace Ttx;

struct ResolveCapture {
  ttx_abstract_sink_ops operations;
  bool answered;
  ttx_abstract answer;
};

struct DomainCapture {
  ttx_domain_result_ops operations;
  bool answered;
  bool valid;
  DomainObservation observation;
};

struct CallableCapture {
  ttx_callable_result_ops operations;
  bool answered;
  bool valid;
  CallableObservation observation;
};

struct CallableSnapshot {
  ttx_abstract candidate;
  ttx_layout_snapshot parameters = {};
  ttx_layout_snapshot results = {};

  ~CallableSnapshot() {
    if (parameters.operations) {
      parameters.operations->release(parameters);
    }
    if (results.operations) {
      results.operations->release(results);
    }
  }
};

struct CallableLayoutCapture {
  ttx_layout_snapshot snapshot = {};
  bool answered = false;
  bool valid = true;
  ttx_pack_support_failure failure = TTX_PACK_SUPPORT_INVALID_LAYOUT;
};

static void callable_layout_retained(
    ttx_layout_snapshot_result self,
    ttx_layout_snapshot snapshot) {
  auto& capture = *reinterpret_cast<CallableLayoutCapture*>(self.self);
  if (capture.answered) {
    capture.valid = false;
    if (snapshot.operations) {
      snapshot.operations->release(snapshot);
    }
    return;
  }
  capture.answered = true;
  capture.snapshot = snapshot;
}

static void callable_layout_failed(
    ttx_layout_snapshot_result self,
    ttx_pack_support_failure failure) {
  auto& capture = *reinterpret_cast<CallableLayoutCapture*>(self.self);
  capture.answered = true;
  capture.valid = false;
  capture.failure = failure;
}

static auto callable_layout_snapshot(
    ttx_layout layout,
    ttx_pack_support_failure& failure) -> ttx_layout_snapshot {
  if (!layout.operations || !layout.operations->snapshot) {
    return {};
  }
  CallableLayoutCapture capture;
  const ttx_layout_snapshot_result_ops ops = {
    .header =
        {sizeof(ttx_layout_snapshot_result_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
    .retained = callable_layout_retained,
    .support_failed = callable_layout_failed,
  };
  layout.operations->snapshot(
      layout,
      {.operations = &ops,
       .self = reinterpret_cast<ttx_layout_snapshot_result_self*>(&capture)});
  if (capture.valid && capture.answered) {
    return capture.snapshot;
  }
  failure = capture.failure;
  if (capture.snapshot.operations) {
    capture.snapshot.operations->release(capture.snapshot);
  }
  return {};
}

struct RouteCapture {
  ttx_route_result_ops operations;
  bool answered;
  bool valid;
  RouteObservation observation;
};

struct ExtentCapture {
  ttx_finite_extent_result_ops operations;
  bool answered;
  bool valid;
  ExtentObservation observation;
};

struct BytesCapture {
  ttx_bytes_result_ops operations;
  bool answered;
  bool valid;
  BytesObservation observation;
};

struct InterfaceCapture {
  ttx_interface_sink_ops operations;
  bool answered;
  ttx_abstract requirement;
  ttx_abstract candidate;
  ttx_interface_relation relation;
};

struct PackCapture {
  ttx_pack_result_ops operations;
  bool answered;
  bool valid;
  PackObservation observation;
};

struct InvocationCapture {
  ttx_interface_sink_ops operations;
  bool answered;
  bool valid;
  ttx_abstract candidate;
  ttx_abstract requirement;
  ttx_abstract operation;
  ttx_pack input;
  ttx_context context;
  ttx_pack_result result;
};

struct ProducerCapture {
  ttx_layout_entry_sink_ops operations;
  std::vector<PackEntry> values;
  bool valid;
  bool completed;
};

struct PackEnumerableCapture {
  ttx_enumerable_result_ops operations;
  bool answered;
  bool satisfied;
  ttx_enumerable enumerable;
};

struct ByteCopyCapture {
  ttx_bytes_sink_ops operations;
  std::vector<uint8_t> value;
  bool valid;
  bool completed;
};

struct BytesSnapshot {
  ttx_abstract candidate;
  std::vector<uint8_t> value;
};

template <typename Operations>
static auto supports(const Operations* operations, uint32_t size) -> bool {
  return operations != nullptr &&
         operations->header.abi_major == TTX_ABI_MAJOR &&
         operations->header.size >= size;
}

static auto producer_capture(ttx_layout_entry_sink self) -> ProducerCapture& {
  return *reinterpret_cast<ProducerCapture*>(self.self);
}

static void TTX_CALL capture_producer(
    ttx_layout_entry_sink self,
    ttx_borrowed_bytes path,
    ttx_abstract producer) {
  ProducerCapture& capture = producer_capture(self);
  if (capture.completed || (path.size != 0 && path.data == nullptr)) {
    capture.valid = false;
    return;
  }
  capture.values.push_back({
    .path = path.size == 0
                ? std::vector<uint8_t>()
                : std::vector<uint8_t>(path.data, path.data + path.size),
    .producer = producer,
  });
}

static void TTX_CALL complete_producers(ttx_layout_entry_sink self) {
  producer_capture(self).completed = true;
}

static auto enumerable_capture(ttx_enumerable_result self)
    -> PackEnumerableCapture& {
  return *reinterpret_cast<PackEnumerableCapture*>(self.self);
}

static void TTX_CALL reject_enumerable(ttx_enumerable_result self) {
  PackEnumerableCapture& capture = enumerable_capture(self);
  capture.answered = true;
  capture.satisfied = false;
}

static void TTX_CALL
    accept_enumerable(ttx_enumerable_result self, ttx_enumerable enumerable) {
  PackEnumerableCapture& capture = enumerable_capture(self);
  capture.answered = true;
  capture.satisfied = true;
  capture.enumerable = enumerable;
}

static auto byte_copy(ttx_bytes_sink self) -> ByteCopyCapture& {
  return *reinterpret_cast<ByteCopyCapture*>(self.self);
}

static void TTX_CALL
    copy_byte_chunk(ttx_bytes_sink self, ttx_borrowed_bytes value) {
  ByteCopyCapture& capture = byte_copy(self);
  if (capture.completed || (value.size != 0 && value.data == nullptr) ||
      value.size > std::numeric_limits<size_t>::max() - capture.value.size()) {
    capture.valid = false;
    return;
  }
  if (value.size != 0) {
    capture.value.insert(
        capture.value.end(), value.data, value.data + value.size);
  }
}

static void TTX_CALL complete_byte_copy(ttx_bytes_sink self) {
  ByteCopyCapture& capture = byte_copy(self);
  if (capture.completed) {
    capture.valid = false;
  }
  capture.completed = true;
}

static auto select(ttx_abstract_sink self) -> ResolveCapture& {
  return *reinterpret_cast<ResolveCapture*>(self.self);
}

static void TTX_CALL answer(ttx_abstract_sink self, ttx_abstract value) {
  ResolveCapture& capture = select(self);
  if (capture.answered) {
    capture.answer = ttx_unknown();
    return;
  }
  capture.answered = true;
  capture.answer = value;
}

static auto select(ttx_domain_result self) -> DomainCapture& {
  return *reinterpret_cast<DomainCapture*>(self.self);
}

static void TTX_CALL domain_unknown(ttx_domain_result self) {
  DomainCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = {
    .state = Observation::Unknown,
    .domain = ttx_unknown(),
    .layout = {},
  };
}

static void TTX_CALL domain_none(ttx_domain_result self) {
  DomainCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = {
    .state = Observation::None,
    .domain = ttx_none(),
    .layout = {},
  };
}

static void TTX_CALL domain_resolved(
    ttx_domain_result self,
    ttx_abstract domain,
    ttx_layout layout) {
  DomainCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = {
    .state = Observation::Resolved,
    .domain = domain,
    .layout = layout,
  };
}

static auto select(ttx_callable_result self) -> CallableCapture& {
  return *reinterpret_cast<CallableCapture*>(self.self);
}

static void TTX_CALL callable_unknown(ttx_callable_result self) {
  CallableCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = {
    .state = CallableObservationState::Unknown,
    .callable = {},
  };
}

static void TTX_CALL callable_none(ttx_callable_result self) {
  CallableCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = {
    .state = CallableObservationState::None,
    .callable = {},
  };
}

static void callable_failed(
    ttx_callable_result self,
    ttx_pack_support_failure failure) {
  auto& capture = select(self);
  capture.valid = !capture.answered;
  capture.answered = true;
  capture.observation = {
    .state = CallableObservationState::SupportFailed,
    .callable = {},
    .failure = failure};
}

static void TTX_CALL
    callable_resolved(ttx_callable_result self, ttx_callable callable) {
  CallableCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = Ttx::retain_callable(callable);
}

auto Ttx::retain_callable(ttx_callable callable) -> CallableObservation {
  if (!callable.operations) {
    return {.state = CallableObservationState::SupportFailed, .callable = {}};
  }
  // Snapshot while the foreign callback still owns its temporary view. Only
  // the Layout owners know how to preserve their complete structure and fits.
  auto snapshot = std::make_shared<CallableSnapshot>();
  snapshot->candidate = callable.operations->candidate(callable);
  ttx_pack_support_failure failure = TTX_PACK_SUPPORT_INVALID_LAYOUT;
  snapshot->parameters = callable_layout_snapshot(
      callable.operations->parameters(callable), failure);
  if (!snapshot->parameters.operations) {
    return {
      .state = CallableObservationState::SupportFailed,
      .callable = {},
      .failure = failure};
  }
  snapshot->results =
      callable_layout_snapshot(callable.operations->results(callable), failure);
  if (!snapshot->parameters.operations || !snapshot->results.operations) {
    return {
      .state = CallableObservationState::SupportFailed,
      .callable = {},
      .failure = failure};
  }
  static const ttx_callable_ops ops = {
    .header = {sizeof(ttx_callable_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
    .candidate =
        [](ttx_callable view) {
          return reinterpret_cast<const CallableSnapshot*>(view.self)
              ->candidate;
        },
    .parameters =
        [](ttx_callable view) {
          const auto s =
              reinterpret_cast<const CallableSnapshot*>(view.self)->parameters;
          return s.operations->layout(s);
        },
    .results =
        [](ttx_callable view) {
          const auto s =
              reinterpret_cast<const CallableSnapshot*>(view.self)->results;
          return s.operations->layout(s);
        },
  };
  const auto state = reinterpret_cast<ttx_callable_self*>(snapshot.get());
  return {
    .state = CallableObservationState::Resolved,
    .callable = {.operations = &ops, .self = state},
    .retained = std::shared_ptr<const ttx_callable_self>(snapshot, state),
  };
}

static auto select(ttx_route_result self) -> RouteCapture& {
  return *reinterpret_cast<RouteCapture*>(self.self);
}

static auto select(ttx_finite_extent_result self) -> ExtentCapture& {
  return *reinterpret_cast<ExtentCapture*>(self.self);
}

static auto select(ttx_bytes_result self) -> BytesCapture& {
  return *reinterpret_cast<BytesCapture*>(self.self);
}

static void TTX_CALL bytes_unknown(ttx_bytes_result self) {
  BytesCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = {
    .state = Observation::Unknown,
    .bytes = {},
  };
}

static void TTX_CALL bytes_none(ttx_bytes_result self) {
  BytesCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = {
    .state = Observation::None,
    .bytes = {},
  };
}

static void TTX_CALL bytes_resolved(ttx_bytes_result self, ttx_bytes bytes) {
  BytesCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = Ttx::retain_bytes(bytes);
}

auto Ttx::retain_bytes(ttx_bytes bytes) -> BytesObservation {
  if (!bytes.operations) {
    return {.state = Observation::Unknown, .bytes = {}};
  }
  ByteCopyCapture copied = {
    .operations =
        {
          .header = {sizeof(ttx_bytes_sink_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
          .bytes = copy_byte_chunk,
          .completed = complete_byte_copy,
        },
    .value = {},
    .valid = true,
    .completed = false,
  };
  const auto expected = bytes.operations->size(bytes);
  bytes.operations->visit(
      bytes, {.operations = &copied.operations,
              .self = reinterpret_cast<ttx_bytes_sink_self*>(&copied)});
  if (!copied.valid || !copied.completed || copied.value.size() != expected) {
    return {.state = Observation::Unknown, .bytes = {}};
  }
  auto snapshot = std::make_shared<BytesSnapshot>(BytesSnapshot{
    bytes.operations->candidate(bytes), std::move(copied.value)});
  static const ttx_bytes_ops ops = {
    .header = {sizeof(ttx_bytes_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
    .candidate =
        [](ttx_bytes view) {
          return reinterpret_cast<const BytesSnapshot*>(view.self)->candidate;
        },
    .size = [](ttx_bytes view) -> uint64_t {
      return reinterpret_cast<const BytesSnapshot*>(view.self)->value.size();
    },
    .visit =
        [](ttx_bytes view, ttx_bytes_sink sink) {
          const auto& value =
              reinterpret_cast<const BytesSnapshot*>(view.self)->value;
          sink.operations->bytes(sink, {value.data(), value.size()});
          sink.operations->completed(sink);
        },
  };
  const auto state = reinterpret_cast<ttx_bytes_self*>(snapshot.get());
  return {
    .state = Observation::Resolved,
    .bytes = {.operations = &ops, .self = state},
    .retained = std::shared_ptr<const ttx_bytes_self>(snapshot, state),
  };
}

static void TTX_CALL finite_extent_unknown(ttx_finite_extent_result self) {
  ExtentCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = {
    .state = Observation::Unknown,
    .extent = {},
  };
}

static void TTX_CALL finite_extent_none(ttx_finite_extent_result self) {
  ExtentCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = {
    .state = Observation::None,
    .extent = {},
  };
}

struct ExtentSnapshot {
  ttx_abstract candidate;
  uint64_t count;
};
auto Ttx::retain_finite_extent(ttx_finite_extent source) -> ExtentObservation {
  if (!supports(source.operations, sizeof(ttx_finite_extent_ops))) {
    return {.state = Observation::Unknown, .extent = {}};
  }
  auto snapshot = std::make_shared<ExtentSnapshot>(ExtentSnapshot{
    source.operations->candidate(source),
    source.operations->cardinality(source)});
  static const ttx_finite_extent_ops ops = {
    .header = {sizeof(ttx_finite_extent_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
    .candidate =
        [](ttx_finite_extent v) {
          return reinterpret_cast<const ExtentSnapshot*>(v.self)->candidate;
        },
    .cardinality =
        [](ttx_finite_extent v) {
          return reinterpret_cast<const ExtentSnapshot*>(v.self)->count;
        },
  };
  auto state = reinterpret_cast<ttx_finite_extent_self*>(snapshot.get());
  return {
    .state = Observation::Resolved,
    .extent = {.operations = &ops, .self = state},
    .retained = std::shared_ptr<const ttx_finite_extent_self>(snapshot, state)};
}
static void TTX_CALL finite_extent_resolved(
    ttx_finite_extent_result self,
    ttx_finite_extent extent) {
  auto& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = Ttx::retain_finite_extent(extent);
}

static void TTX_CALL route_unknown(ttx_route_result self) {
  RouteCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = {
    .state = Observation::Unknown,
    .route = {},
    .candidate = ttx_unknown(),
    .bytes = {},
  };
}

static void TTX_CALL route_none(ttx_route_result self) {
  RouteCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = {
    .state = Observation::None,
    .route = {},
    .candidate = ttx_none(),
    .bytes = {},
  };
}

struct RouteSnapshot {
  ttx_abstract candidate;
  std::vector<uint8_t> value;
};
auto Ttx::retain_route(ttx_route source) -> RouteObservation {
  if (!supports(source.operations, sizeof(ttx_route_ops))) {
    return {
      .state = Observation::Unknown,
      .route = {},
      .candidate = ttx_unknown(),
      .bytes = {}};
  }
  const auto bytes = source.operations->bytes(source);
  if (bytes.size && !bytes.data) {
    return {
      .state = Observation::Unknown,
      .route = {},
      .candidate = ttx_unknown(),
      .bytes = {}};
  }
  auto snapshot = std::make_shared<RouteSnapshot>();
  snapshot->candidate = source.operations->candidate(source);
  if (bytes.size) {
    snapshot->value.assign(bytes.data, bytes.data + bytes.size);
  }
  static const ttx_route_ops ops = {
    .header = {sizeof(ttx_route_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
    .candidate =
        [](ttx_route v) {
          return reinterpret_cast<const RouteSnapshot*>(v.self)->candidate;
        },
    .bytes = [](ttx_route v) -> ttx_borrowed_bytes {
      const auto& value = reinterpret_cast<const RouteSnapshot*>(v.self)->value;
      return {value.data(), value.size()};
    },
  };
  auto state = reinterpret_cast<ttx_route_self*>(snapshot.get());
  return {
    .state = Observation::Resolved,
    .route = {.operations = &ops, .self = state},
    .candidate = snapshot->candidate,
    .bytes = {snapshot->value.data(), snapshot->value.size()},
    .retained = std::shared_ptr<const ttx_route_self>(snapshot, state)};
}
static void TTX_CALL route_resolved(ttx_route_result self, ttx_route route) {
  auto& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = Ttx::retain_route(route);
}

static auto select(ttx_interface_sink self) -> InterfaceCapture& {
  return *reinterpret_cast<InterfaceCapture*>(self.self);
}

static void TTX_CALL
    interface_answer(ttx_interface_sink self, ttx_interface interface) {
  InterfaceCapture& capture = select(self);
  if (capture.answered || interface == nullptr ||
      interface->operations == nullptr ||
      interface->operations->header.abi_major != TTX_ABI_MAJOR ||
      interface->operations->header.size < TTX_INTERFACE_RELATION_PREFIX_SIZE) {
    capture.answered = true;
    capture.relation = TTX_INTERFACE_UNKNOWN;
    return;
  }
  capture.answered = true;
  capture.requirement = interface->operations->requirement(interface);
  capture.candidate = interface->operations->candidate(interface);
  capture.relation = interface->operations->negotiate(interface);
}

static auto select_invocation(ttx_interface_sink self) -> InvocationCapture& {
  return *reinterpret_cast<InvocationCapture*>(self.self);
}

static void TTX_CALL
    invoke_interface(ttx_interface_sink self, ttx_interface interface) {
  InvocationCapture& capture = select_invocation(self);
  if (capture.answered || interface == nullptr ||
      !supports(interface->operations, sizeof(ttx_interface_ops)) ||
      !ttx_abstract_same(
          interface->operations->candidate(interface), capture.candidate) ||
      !ttx_abstract_same(
          interface->operations->requirement(interface), capture.requirement)) {
    capture.answered = true;
    capture.valid = false;
    return;
  }
  capture.answered = true;
  const ttx_interface_relation relation =
      interface->operations->negotiate(interface);
  if (relation == TTX_INTERFACE_UNKNOWN) {
    capture.result.operations->unknown(capture.result);
    return;
  }
  if (relation != TTX_INTERFACE_SATISFIED &&
      relation != TTX_INTERFACE_EQUIVALENT) {
    capture.result.operations->none(capture.result);
    return;
  }
  interface->operations->invoke(
      interface, capture.operation, capture.input, capture.context,
      capture.result);
}

static auto select(ttx_pack_result self) -> PackCapture& {
  return *reinterpret_cast<PackCapture*>(self.self);
}

static void TTX_CALL pack_unknown(ttx_pack_result self) {
  PackCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation.state = PackObservationState::Unknown;
}

static void TTX_CALL pack_none(ttx_pack_result self) {
  PackCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation.state = PackObservationState::None;
}

static void TTX_CALL pack_retained(ttx_pack_result self, ttx_pack pack) {
  PackCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation.state = PackObservationState::Packed;
  capture.observation.pack = pack;
}

static void TTX_CALL pack_support_failed(
    ttx_pack_result self,
    ttx_pack_support_failure failure) {
  PackCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation.state = PackObservationState::SupportFailed;
  capture.observation.failure = failure;
}

static auto capture(ttx_abstract source, ttx_borrowed_bytes* route)
    -> ttx_abstract {
  if (source == nullptr || source->operations == nullptr ||
      source->operations->header.abi_major != TTX_ABI_MAJOR ||
      source->operations->header.size < TTX_ABSTRACT_INTERFACE_PREFIX_SIZE) {
    return ttx_unknown();
  }
  ResolveCapture state = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_abstract_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .answer = answer,
        },
    .answered = false,
    .answer = ttx_unknown(),
  };
  const ttx_abstract_sink result = {
    .operations = &state.operations,
    .self = reinterpret_cast<ttx_abstract_sink_self*>(&state),
  };
  if (route == nullptr) {
    source->operations->resolve(source, result);
  } else {
    source->operations->resolve_concept(source, *route, result);
  }
  return state.answered ? state.answer : ttx_unknown();
}

auto Ttx::resolve(ttx_abstract source) -> ttx_abstract {
  return capture(source, nullptr);
}

auto Ttx::resolve_concept(ttx_abstract source, ttx_borrowed_bytes route)
    -> ttx_abstract {
  return capture(source, &route);
}

static auto unknown_domain() -> DomainObservation {
  return {
    .state = Observation::Unknown,
    .domain = ttx_unknown(),
    .layout = {},
  };
}

static auto observe_domain(ttx_abstract source) -> DomainObservation {
  DomainCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_domain_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .unknown = domain_unknown,
          .none = domain_none,
          .resolved = domain_resolved,
        },
    .answered = false,
    .valid = true,
    .observation =
        {
          .state = Observation::Unknown,
          .domain = ttx_unknown(),
          .layout = {},
        },
  };
  if (source == nullptr ||
      !supports(source->operations, sizeof(ttx_abstract_ops))) {
    return unknown_domain();
  }
  const ttx_domain_result result = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_domain_result_self*>(&capture),
  };
  source->operations->resolve_domain(source, result);
  if (!capture.answered || !capture.valid) {
    return unknown_domain();
  }
  if (capture.observation.state == Observation::Resolved &&
      (capture.observation.domain == nullptr ||
       !supports(
           capture.observation.domain->operations, sizeof(ttx_abstract_ops)) ||
       !supports(
           capture.observation.layout.operations, sizeof(ttx_layout_ops)))) {
    return unknown_domain();
  }
  return capture.observation;
}

auto Ttx::resolve_domain(ttx_abstract source) -> DomainObservation {
  if (source == nullptr ||
      !supports(source->operations, sizeof(ttx_abstract_ops))) {
    return unknown_domain();
  }
  source = Ttx::resolve(source);
  if (source == nullptr ||
      !supports(source->operations, sizeof(ttx_abstract_ops))) {
    return unknown_domain();
  }
  const DomainObservation observation = observe_domain(source);
  if (observation.state != Observation::Resolved) {
    return observation;
  }

  // A completed Domain closes its own total route. Checking that fixed point
  // rejects a two owner cycle without requiring a native Domain class or
  // turning the relationship into an inheritance walk.
  const ttx_abstract domain = Ttx::resolve(observation.domain);
  if (!ttx_abstract_same(domain, observation.domain)) {
    return unknown_domain();
  }
  const DomainObservation closure = observe_domain(domain);
  if (closure.state != Observation::Resolved ||
      !ttx_abstract_same(closure.domain, domain)) {
    return unknown_domain();
  }
  return observation;
}

auto Ttx::resolve_callable(ttx_abstract source) -> CallableObservation {
  CallableCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_callable_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .unknown = callable_unknown,
          .none = callable_none,
          .resolved = callable_resolved,
          .support_failed = callable_failed,
        },
    .answered = false,
    .valid = true,
    .observation =
        {
          .state = CallableObservationState::Unknown,
          .callable = {},
        },
  };
  if (source == nullptr || source->operations == nullptr ||
      source->operations->header.abi_major != TTX_ABI_MAJOR ||
      source->operations->header.size < sizeof(ttx_abstract_ops)) {
    return capture.observation;
  }
  source = Ttx::resolve(source);
  const ttx_interface_relation proof =
      Ttx::relation(source, ttx_callable_requirement());
  if (proof == TTX_INTERFACE_UNKNOWN) {
    return capture.observation;
  }
  if (proof == TTX_INTERFACE_REJECTED) {
    capture.observation.state = CallableObservationState::None;
    return capture.observation;
  }
  const ttx_callable_result result = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_callable_result_self*>(&capture),
  };
  source->operations->resolve_callable(source, result);
  if (!capture.answered || !capture.valid ||
      (capture.observation.state == CallableObservationState::Resolved &&
       (capture.observation.callable.operations == nullptr ||
        !ttx_abstract_same(
            capture.observation.callable.operations->candidate(
                capture.observation.callable),
            source)))) {
    capture.observation = {
      .state = CallableObservationState::Unknown,
      .callable = {},
    };
  }
  return capture.observation;
}

auto Ttx::resolve_route(ttx_abstract source) -> RouteObservation {
  RouteCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_route_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .unknown = route_unknown,
          .none = route_none,
          .resolved = route_resolved,
        },
    .answered = false,
    .valid = true,
    .observation =
        {
          .state = Observation::Unknown,
          .route = {},
          .candidate = ttx_unknown(),
          .bytes = {},
        },
  };
  if (source == nullptr || source->operations == nullptr ||
      source->operations->header.abi_major != TTX_ABI_MAJOR ||
      source->operations->header.size < sizeof(ttx_abstract_ops)) {
    return capture.observation;
  }
  source = Ttx::resolve(source);
  const ttx_interface_relation constant =
      Ttx::relation(source, ttx_constant_requirement());
  const ttx_interface_relation proof =
      Ttx::relation(source, ttx_route_requirement());
  if (constant == TTX_INTERFACE_UNKNOWN || proof == TTX_INTERFACE_UNKNOWN) {
    return capture.observation;
  }
  if (constant == TTX_INTERFACE_REJECTED || proof == TTX_INTERFACE_REJECTED) {
    capture.observation.state = Observation::None;
    capture.observation.route = {};
    capture.observation.candidate = ttx_none();
    return capture.observation;
  }
  const ttx_route_result result = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_route_result_self*>(&capture),
  };
  source->operations->resolve_route(source, result);
  if (!capture.answered || !capture.valid ||
      (capture.observation.state == Observation::Resolved &&
       (!ttx_abstract_same(capture.observation.candidate, source) ||
        (capture.observation.bytes.size != 0 &&
         capture.observation.bytes.data == nullptr)))) {
    capture.observation = {
      .state = Observation::Unknown,
      .route = {},
      .candidate = ttx_unknown(),
      .bytes = {},
    };
  }
  return capture.observation;
}

auto Ttx::resolve_finite_extent(ttx_abstract source) -> ExtentObservation {
  ExtentCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_finite_extent_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .unknown = finite_extent_unknown,
          .none = finite_extent_none,
          .resolved = finite_extent_resolved,
        },
    .answered = false,
    .valid = true,
    .observation =
        {
          .state = Observation::Unknown,
          .extent = {},
        },
  };
  if (source == nullptr ||
      !supports(source->operations, sizeof(ttx_abstract_ops))) {
    return capture.observation;
  }
  source = Ttx::resolve(source);
  const ttx_interface_relation constant =
      Ttx::relation(source, ttx_constant_requirement());
  const ttx_interface_relation proof =
      Ttx::relation(source, ttx_finite_extent_requirement());
  if (constant == TTX_INTERFACE_UNKNOWN || proof == TTX_INTERFACE_UNKNOWN) {
    return capture.observation;
  }
  if (constant == TTX_INTERFACE_REJECTED || proof == TTX_INTERFACE_REJECTED) {
    capture.observation.state = Observation::None;
    return capture.observation;
  }
  const ttx_finite_extent_result result = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_finite_extent_result_self*>(&capture),
  };
  source->operations->resolve_finite_extent(source, result);
  if (!capture.answered || !capture.valid ||
      (capture.observation.state == Observation::Resolved &&
       (!supports(
            capture.observation.extent.operations,
            sizeof(ttx_finite_extent_ops)) ||
        !ttx_abstract_same(
            capture.observation.extent.operations->candidate(
                capture.observation.extent),
            source)))) {
    capture.observation = {
      .state = Observation::Unknown,
      .extent = {},
    };
  }
  return capture.observation;
}

auto Ttx::resolve_bytes(ttx_abstract source) -> BytesObservation {
  BytesCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_bytes_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .unknown = bytes_unknown,
          .none = bytes_none,
          .resolved = bytes_resolved,
        },
    .answered = false,
    .valid = true,
    .observation = {.state = Observation::Unknown, .bytes = {}},
  };
  if (source == nullptr ||
      !supports(source->operations, sizeof(ttx_abstract_ops))) {
    return capture.observation;
  }
  source = Ttx::resolve(source);
  const ttx_interface_relation proof =
      Ttx::relation(source, ttx_bytes_requirement());
  if (proof == TTX_INTERFACE_UNKNOWN) {
    return capture.observation;
  }
  if (proof == TTX_INTERFACE_REJECTED) {
    capture.observation.state = Observation::None;
    return capture.observation;
  }
  const ttx_bytes_result result = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_bytes_result_self*>(&capture),
  };
  source->operations->resolve_bytes(source, result);
  if (!capture.answered || !capture.valid ||
      (capture.observation.state == Observation::Resolved &&
       (!supports(
            capture.observation.bytes.operations, sizeof(ttx_bytes_ops)) ||
        !ttx_abstract_same(
            capture.observation.bytes.operations->candidate(
                capture.observation.bytes),
            source)))) {
    capture.observation = {.state = Observation::Unknown, .bytes = {}};
  }
  return capture.observation;
}

auto Ttx::copy_bytes(ttx_abstract source)
    -> std::optional<std::vector<uint8_t>> {
  const BytesObservation observation = resolve_bytes(source);
  if (observation.state != Observation::Resolved ||
      !supports(observation.bytes.operations, sizeof(ttx_bytes_ops))) {
    return std::nullopt;
  }
  ByteCopyCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_bytes_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .bytes = copy_byte_chunk,
          .completed = complete_byte_copy,
        },
    .value = {},
    .valid = true,
    .completed = false,
  };
  observation.bytes.operations->visit(
      observation.bytes,
      {
        .operations = &capture.operations,
        .self = reinterpret_cast<ttx_bytes_sink_self*>(&capture),
      });
  if (!capture.valid || !capture.completed ||
      capture.value.size() !=
          observation.bytes.operations->size(observation.bytes)) {
    return std::nullopt;
  }
  return std::move(capture.value);
}

auto Ttx::relation(ttx_abstract candidate, ttx_abstract requirement)
    -> ttx_interface_relation {
  // An Alias forwards the question to its referent, including the candidate
  // reported by a witness. Establish that subject before checking the pair.
  candidate = Ttx::resolve(candidate);
  requirement = Ttx::resolve(requirement);
  InterfaceCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_interface_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .answer = interface_answer,
        },
    .answered = false,
    .requirement = ttx_unknown(),
    .candidate = ttx_unknown(),
    .relation = TTX_INTERFACE_UNKNOWN,
  };
  if (candidate == nullptr || requirement == nullptr ||
      candidate->operations == nullptr || requirement->operations == nullptr ||
      candidate->operations->header.abi_major != TTX_ABI_MAJOR ||
      candidate->operations->header.size < TTX_ABSTRACT_INTERFACE_PREFIX_SIZE) {
    return TTX_INTERFACE_UNKNOWN;
  }
  const ttx_interface_sink result = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_interface_sink_self*>(&capture),
  };
  candidate->operations->interface(candidate, requirement, result);
  if (!capture.answered ||
      !ttx_abstract_same(capture.requirement, requirement) ||
      !ttx_abstract_same(capture.candidate, candidate)) {
    return TTX_INTERFACE_UNKNOWN;
  }
  return capture.relation;
}

enum class ShapeFit {
  Unknown,
  Rejected,
  Accepted,
};

static auto fit_shape(
    ttx_layout receiving,
    ttx_layout source,
    ttx_context context) -> ShapeFit {
  const Ttx::PackObservation source_pack = Ttx::pack(context, source);
  if (source_pack.state != Ttx::PackObservationState::Packed) {
    return ShapeFit::Unknown;
  }
  const Ttx::PackObservation admitted =
      Ttx::fit(receiving, source_pack.pack, context);
  if (admitted.state == Ttx::PackObservationState::Packed) {
    return ShapeFit::Accepted;
  }
  return admitted.state == Ttx::PackObservationState::None ? ShapeFit::Rejected
                                                           : ShapeFit::Unknown;
}

auto Ttx::compare_call_shape(ttx_abstract requirement, ttx_abstract candidate)
    -> ttx_interface_relation {
  const CallableObservation required = resolve_callable(requirement);
  const CallableObservation supplied = resolve_callable(candidate);
  if (required.state == CallableObservationState::None ||
      supplied.state == CallableObservationState::None) {
    return TTX_INTERFACE_REJECTED;
  }
  if (required.state != CallableObservationState::Resolved ||
      supplied.state != CallableObservationState::Resolved) {
    return TTX_INTERFACE_UNKNOWN;
  }
  if (ttx_abstract_same(requirement, candidate)) {
    return TTX_INTERFACE_EQUIVALENT;
  }

  const ttx_callable required_callable = required.callable;
  const ttx_callable supplied_callable = supplied.callable;
  const ttx_layout required_parameters =
      required_callable.operations->parameters(required_callable);
  const ttx_layout supplied_parameters =
      supplied_callable.operations->parameters(supplied_callable);
  const ttx_layout required_results =
      required_callable.operations->results(required_callable);
  const ttx_layout supplied_results =
      supplied_callable.operations->results(supplied_callable);
  const ttx_context context = ttx_context_create();
  if (!supports(context.operations, sizeof(ttx_context_ops))) {
    return TTX_INTERFACE_UNKNOWN;
  }

  const ShapeFit required_to_supplied =
      fit_shape(supplied_parameters, required_parameters, context);
  const ShapeFit supplied_to_required =
      fit_shape(required_parameters, supplied_parameters, context);
  const ShapeFit result_direction =
      fit_shape(required_results, supplied_results, context);
  if (required_to_supplied == ShapeFit::Rejected ||
      supplied_to_required == ShapeFit::Rejected ||
      result_direction == ShapeFit::Rejected) {
    context.operations->release(context);
    return TTX_INTERFACE_REJECTED;
  }
  if (required_to_supplied == ShapeFit::Unknown ||
      supplied_to_required == ShapeFit::Unknown ||
      result_direction == ShapeFit::Unknown) {
    context.operations->release(context);
    return TTX_INTERFACE_UNKNOWN;
  }

  context.operations->release(context);
  return TTX_INTERFACE_SATISFIED;
}

auto Ttx::invoke(
    ttx_abstract candidate,
    ttx_abstract requirement,
    ttx_abstract operation,
    ttx_pack input,
    ttx_context context) -> PackObservation {
  candidate = Ttx::resolve(candidate);
  requirement = Ttx::resolve(requirement);
  operation = Ttx::resolve(operation);
  PackCapture result_capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_pack_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .unknown = pack_unknown,
          .none = pack_none,
          .packed = pack_retained,
          .support_failed = pack_support_failed,
        },
    .answered = false,
    .valid = true,
    .observation =
        {
          .state = PackObservationState::SupportFailed,
          .pack = {},
          .failure = TTX_PACK_SUPPORT_INVALID_LAYOUT,
        },
  };
  const ttx_pack_result result = {
    .operations = &result_capture.operations,
    .self = reinterpret_cast<ttx_pack_result_self*>(&result_capture),
  };
  InvocationCapture invocation = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_interface_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .answer = invoke_interface,
        },
    .answered = false,
    .valid = true,
    .candidate = candidate,
    .requirement = requirement,
    .operation = operation,
    .input = input,
    .context = context,
    .result = result,
  };
  if (candidate == nullptr || requirement == nullptr || operation == nullptr ||
      !supports(candidate->operations, TTX_ABSTRACT_INTERFACE_PREFIX_SIZE) ||
      !supports(requirement->operations, TTX_ABSTRACT_INTERFACE_PREFIX_SIZE) ||
      !supports(operation->operations, TTX_ABSTRACT_INTERFACE_PREFIX_SIZE) ||
      !supports(input.operations, sizeof(ttx_pack_ops)) ||
      !supports(context.operations, sizeof(ttx_context_ops))) {
    return result_capture.observation;
  }
  const ttx_interface_sink sink = {
    .operations = &invocation.operations,
    .self = reinterpret_cast<ttx_interface_sink_self*>(&invocation),
  };
  candidate->operations->interface(candidate, requirement, sink);
  if (!invocation.answered || !invocation.valid || !result_capture.answered ||
      !result_capture.valid) {
    return {
      .state = PackObservationState::SupportFailed,
      .pack = {},
      .failure = TTX_PACK_SUPPORT_INVALID_LAYOUT,
    };
  }
  return result_capture.observation;
}

auto Ttx::fit(ttx_layout receiving, ttx_pack source, ttx_context context)
    -> PackObservation {
  PackCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_pack_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .unknown = pack_unknown,
          .none = pack_none,
          .packed = pack_retained,
          .support_failed = pack_support_failed,
        },
    .answered = false,
    .valid = true,
    .observation =
        {
          .state = PackObservationState::SupportFailed,
          .pack = {},
          .failure = TTX_PACK_SUPPORT_INVALID_LAYOUT,
        },
  };
  if (receiving.operations == nullptr || source.operations == nullptr ||
      context.operations == nullptr ||
      receiving.operations->header.abi_major != TTX_ABI_MAJOR ||
      receiving.operations->header.size < sizeof(ttx_layout_ops)) {
    return capture.observation;
  }
  const ttx_pack_result result = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_pack_result_self*>(&capture),
  };
  receiving.operations->fit(receiving, source, context, result);
  if (!capture.answered || !capture.valid) {
    capture.observation = {
      .state = PackObservationState::SupportFailed,
      .pack = {},
      .failure = TTX_PACK_SUPPORT_INVALID_LAYOUT,
    };
  }
  return capture.observation;
}

auto Ttx::pack(ttx_context context, ttx_layout produced_flow)
    -> PackObservation {
  PackCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_pack_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .unknown = pack_unknown,
          .none = pack_none,
          .packed = pack_retained,
          .support_failed = pack_support_failed,
        },
    .answered = false,
    .valid = true,
    .observation =
        {
          .state = PackObservationState::SupportFailed,
          .pack = {},
          .failure = TTX_PACK_SUPPORT_INVALID_LAYOUT,
        },
  };
  if (context.operations == nullptr || produced_flow.operations == nullptr ||
      context.operations->header.abi_major != TTX_ABI_MAJOR ||
      context.operations->header.size < sizeof(ttx_context_ops)) {
    return capture.observation;
  }
  const ttx_pack_result result = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_pack_result_self*>(&capture),
  };
  context.operations->pack(context, produced_flow, result);
  if (!capture.answered || !capture.valid) {
    capture.observation = {
      .state = PackObservationState::SupportFailed,
      .pack = {},
      .failure = TTX_PACK_SUPPORT_INVALID_LAYOUT,
    };
  }
  return capture.observation;
}

auto Ttx::pack_entries(ttx_pack pack) -> std::optional<std::vector<PackEntry>> {
  if (!supports(pack.operations, sizeof(ttx_pack_ops))) {
    return std::nullopt;
  }
  const ttx_layout layout = pack.operations->layout(pack);
  if (!supports(layout.operations, sizeof(ttx_layout_ops))) {
    return std::nullopt;
  }
  PackEnumerableCapture enumerable = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_enumerable_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .rejected = reject_enumerable,
          .satisfied = accept_enumerable,
        },
    .answered = false,
    .satisfied = false,
    .enumerable = {},
  };
  layout.operations->enumerable(
      layout,
      {
        .operations = &enumerable.operations,
        .self = reinterpret_cast<ttx_enumerable_result_self*>(&enumerable),
      });
  if (!enumerable.answered || !enumerable.satisfied ||
      !supports(enumerable.enumerable.operations, sizeof(ttx_enumerable_ops))) {
    return std::nullopt;
  }

  ProducerCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_layout_entry_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .entry = capture_producer,
          .completed = complete_producers,
        },
    .values = {},
    .valid = true,
    .completed = false,
  };
  enumerable.enumerable.operations->visit(
      enumerable.enumerable,
      {
        .operations = &capture.operations,
        .self = reinterpret_cast<ttx_layout_entry_sink_self*>(&capture),
      });
  if (!capture.valid || !capture.completed ||
      capture.values.size() != enumerable.enumerable.operations->cardinality(
                                   enumerable.enumerable)) {
    return std::nullopt;
  }
  return std::move(capture.values);
}

auto Ttx::producers(ttx_pack pack) -> std::optional<std::vector<ttx_abstract>> {
  auto entries = pack_entries(pack);
  if (!entries) {
    return std::nullopt;
  }
  std::vector<ttx_abstract> result;
  result.reserve(entries->size());
  for (const PackEntry& entry : *entries) {
    result.push_back(entry.producer);
  }
  return result;
}
