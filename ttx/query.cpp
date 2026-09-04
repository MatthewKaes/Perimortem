// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/query.hpp"

#include <cstddef>

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

template <typename Operations>
static auto supports(const Operations* operations, uint32_t size) -> bool {
  return operations != nullptr &&
         operations->header.abi_major == TTX_ABI_MAJOR &&
         operations->header.size >= size;
}

static auto select(ttx_abstract_sink self) -> ResolveCapture& {
  static_assert(offsetof(ResolveCapture, operations) == 0);
  return *reinterpret_cast<ResolveCapture*>(
      const_cast<ttx_abstract_sink_ops*>(self.operations));
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
  static_assert(offsetof(DomainCapture, operations) == 0);
  return *reinterpret_cast<DomainCapture*>(
      const_cast<ttx_domain_result_ops*>(self.operations));
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
  static_assert(offsetof(CallableCapture, operations) == 0);
  return *reinterpret_cast<CallableCapture*>(
      const_cast<ttx_callable_result_ops*>(self.operations));
}

static void TTX_CALL callable_unknown(ttx_callable_result self) {
  CallableCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = {
    .state = Observation::Unknown,
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
    .state = Observation::None,
    .callable = {},
  };
}

static void TTX_CALL
    callable_resolved(ttx_callable_result self, ttx_callable callable) {
  CallableCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = {
    .state = Observation::Resolved,
    .callable = callable,
  };
}

static auto select(ttx_route_result self) -> RouteCapture& {
  static_assert(offsetof(RouteCapture, operations) == 0);
  return *reinterpret_cast<RouteCapture*>(
      const_cast<ttx_route_result_ops*>(self.operations));
}

static auto select(ttx_finite_extent_result self) -> ExtentCapture& {
  static_assert(offsetof(ExtentCapture, operations) == 0);
  return *reinterpret_cast<ExtentCapture*>(
      const_cast<ttx_finite_extent_result_ops*>(self.operations));
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

static void TTX_CALL finite_extent_resolved(
    ttx_finite_extent_result self,
    ttx_finite_extent extent) {
  ExtentCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = {
    .state = Observation::Resolved,
    .extent = extent,
  };
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
    .candidate = ttx_none(),
    .bytes = {},
  };
}

static void TTX_CALL route_resolved(ttx_route_result self, ttx_route route) {
  if (route.operations == nullptr) {
    route_unknown(self);
    return;
  }
  RouteCapture& capture = select(self);
  if (capture.answered) {
    capture.valid = false;
    return;
  }
  capture.answered = true;
  capture.observation = {
    .state = Observation::Resolved,
    .candidate = route.operations->candidate(route),
    .bytes = route.operations->bytes(route),
  };
}

static auto select(ttx_interface_sink self) -> InterfaceCapture& {
  static_assert(offsetof(InterfaceCapture, operations) == 0);
  return *reinterpret_cast<InterfaceCapture*>(
      const_cast<ttx_interface_sink_ops*>(self.operations));
}

static void TTX_CALL
    interface_answer(ttx_interface_sink self, ttx_interface interface) {
  InterfaceCapture& capture = select(self);
  if (capture.answered || interface.operations == nullptr ||
      interface.operations->header.abi_major != TTX_ABI_MAJOR ||
      interface.operations->header.size < TTX_INTERFACE_RELATION_PREFIX_SIZE) {
    capture.answered = true;
    capture.relation = TTX_INTERFACE_UNKNOWN;
    return;
  }
  capture.answered = true;
  capture.requirement = interface.operations->requirement(interface);
  capture.candidate = interface.operations->candidate(interface);
  capture.relation = interface.operations->negotiate(interface);
}

static auto select(ttx_pack_result self) -> PackCapture& {
  static_assert(offsetof(PackCapture, operations) == 0);
  return *reinterpret_cast<PackCapture*>(
      const_cast<ttx_pack_result_ops*>(self.operations));
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
  if (source.operations == nullptr ||
      source.operations->header.abi_major != TTX_ABI_MAJOR ||
      source.operations->header.size < TTX_ABSTRACT_INTERFACE_PREFIX_SIZE) {
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
    .owner = source.owner,
    .value = source.value,
  };
  if (route == nullptr) {
    source.operations->resolve(source, result);
  } else {
    source.operations->resolve_concept(source, *route, result);
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
  if (!supports(source.operations, sizeof(ttx_abstract_ops))) {
    return unknown_domain();
  }
  const ttx_domain_result result = {
    .operations = &capture.operations,
    .owner = source.owner,
    .value = source.value,
  };
  source.operations->resolve_domain(source, result);
  if (!capture.answered || !capture.valid) {
    return unknown_domain();
  }
  if (capture.observation.state == Observation::Resolved &&
      (!supports(
           capture.observation.domain.operations, sizeof(ttx_abstract_ops)) ||
       !supports(
           capture.observation.layout.operations, sizeof(ttx_layout_ops)))) {
    return unknown_domain();
  }
  return capture.observation;
}

auto Ttx::resolve_domain(ttx_abstract source) -> DomainObservation {
  if (!supports(source.operations, sizeof(ttx_abstract_ops))) {
    return unknown_domain();
  }
  source = Ttx::resolve(source);
  if (!supports(source.operations, sizeof(ttx_abstract_ops))) {
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
        },
    .answered = false,
    .valid = true,
    .observation =
        {
          .state = Observation::Unknown,
          .callable = {},
        },
  };
  if (source.operations == nullptr ||
      source.operations->header.abi_major != TTX_ABI_MAJOR ||
      source.operations->header.size < sizeof(ttx_abstract_ops)) {
    return capture.observation;
  }
  source = Ttx::resolve(source);
  const ttx_interface_relation proof =
      Ttx::relation(source, ttx_callable_requirement());
  if (proof == TTX_INTERFACE_UNKNOWN) {
    return capture.observation;
  }
  if (proof == TTX_INTERFACE_REJECTED) {
    capture.observation.state = Observation::None;
    return capture.observation;
  }
  const ttx_callable_result result = {
    .operations = &capture.operations,
    .owner = source.owner,
    .value = source.value,
  };
  source.operations->resolve_callable(source, result);
  if (!capture.answered || !capture.valid ||
      (capture.observation.state == Observation::Resolved &&
       (capture.observation.callable.operations == nullptr ||
        !ttx_abstract_same(
            capture.observation.callable.operations->candidate(
                capture.observation.callable),
            source)))) {
    capture.observation = {
      .state = Observation::Unknown,
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
          .candidate = ttx_unknown(),
          .bytes = {},
        },
  };
  if (source.operations == nullptr ||
      source.operations->header.abi_major != TTX_ABI_MAJOR ||
      source.operations->header.size < sizeof(ttx_abstract_ops)) {
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
    capture.observation.candidate = ttx_none();
    return capture.observation;
  }
  const ttx_route_result result = {
    .operations = &capture.operations,
    .owner = source.owner,
    .value = source.value,
  };
  source.operations->resolve_route(source, result);
  if (!capture.answered || !capture.valid ||
      (capture.observation.state == Observation::Resolved &&
       (!ttx_abstract_same(capture.observation.candidate, source) ||
        (capture.observation.bytes.size != 0 &&
         capture.observation.bytes.data == nullptr)))) {
    capture.observation = {
      .state = Observation::Unknown,
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
  if (!supports(source.operations, sizeof(ttx_abstract_ops))) {
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
    .owner = source.owner,
    .value = source.value,
  };
  source.operations->resolve_finite_extent(source, result);
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

auto Ttx::relation(ttx_abstract candidate, ttx_abstract requirement)
    -> ttx_interface_relation {
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
  if (candidate.operations == nullptr || requirement.operations == nullptr ||
      candidate.operations->header.abi_major != TTX_ABI_MAJOR ||
      candidate.operations->header.size < TTX_ABSTRACT_INTERFACE_PREFIX_SIZE) {
    return TTX_INTERFACE_UNKNOWN;
  }
  const ttx_interface_sink result = {
    .operations = &capture.operations,
    .owner = candidate.owner,
    .value = candidate.value,
  };
  candidate.operations->interface(candidate, requirement, result);
  if (!capture.answered ||
      !ttx_abstract_same(capture.requirement, requirement) ||
      !ttx_abstract_same(capture.candidate, candidate)) {
    return TTX_INTERFACE_UNKNOWN;
  }
  return capture.relation;
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
    .owner = receiving.owner,
    .value = receiving.value,
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
    .owner = context.owner,
    .value = context.value,
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
