// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/model/addressable.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <utility>

using namespace Ttx;

struct OfferedRoutes {
  ttx_concept_sink_ops operations;
  std::vector<std::vector<uint8_t>> routes;
  bool completed;
};

struct LayeredInterface final : ttx_interface_capability {
  LayeredInterface(
      const ttx_interface_ops* operations,
      const Addressable* owner,
      ttx_addressable_policy policy,
      ttx_interface delegated,
      ttx_abstract requirement,
      ttx_abstract candidate,
      ttx_interface_relation relation)
      : ttx_interface_capability{operations},
        owner(owner),
        policy(policy),
        delegated(delegated),
        requirement(requirement),
        candidate(candidate),
        relation(relation) {}

  const Addressable* owner;
  ttx_addressable_policy policy;
  ttx_interface delegated;
  ttx_abstract requirement;
  ttx_abstract candidate;
  ttx_interface_relation relation;
};

struct PolicyAbstractCapture {
  ttx_abstract_sink_ops operations;
  bool answered;
  ttx_abstract answer;
};

struct PolicyInterfaceCapture {
  ttx_addressable_interface_result_ops operations;
  bool answered;
  bool passes;
  ttx_interface_relation relation;
};

struct PolicyDomainCapture {
  ttx_domain_result result;
  bool answered = false;
  bool stopped = false;
};

struct PolicyCallableCapture {
  ttx_callable_result_ops operations;
  bool answered;
  Addressable::LayerAnswer<CallableObservation> answer;
};

struct PolicyRouteCapture {
  ttx_route_result_ops operations;
  bool answered;
  Addressable::LayerAnswer<RouteObservation> answer;
};

struct PolicyExtentCapture {
  ttx_finite_extent_result_ops operations;
  bool answered;
  Addressable::LayerAnswer<ExtentObservation> answer;
};

struct PolicyBytesCapture {
  ttx_bytes_result_ops operations;
  bool answered;
  Addressable::LayerAnswer<BytesObservation> answer;
};

struct DelegatedInterface {
  ttx_interface_sink_ops operations;
  const Addressable* owner;
  ttx_abstract source;
  ttx_abstract requirement;
  ttx_abstract candidate;
  ttx_interface_sink result;
  bool answered;
};

template <typename Operations>
static auto supports(const Operations* operations, uint32_t size) -> bool {
  return operations != nullptr &&
         operations->header.abi_major == TTX_ABI_MAJOR &&
         operations->header.size >= size;
}

template <typename Owner, typename Self>
static auto select_owner(Self* self) -> Owner& {
  return *reinterpret_cast<Owner*>(self);
}

static auto select(ttx_concept_sink self) -> OfferedRoutes& {
  return select_owner<OfferedRoutes>(self.self);
}

static auto select(ttx_abstract_sink self) -> PolicyAbstractCapture& {
  return select_owner<PolicyAbstractCapture>(self.self);
}

static void TTX_CALL
    policy_abstract_answer(ttx_abstract_sink self, ttx_abstract answer) {
  PolicyAbstractCapture& capture = select(self);
  if (capture.answered) {
    capture.answer = ttx_unknown();
    return;
  }
  capture.answered = true;
  capture.answer = answer;
}

static auto select(ttx_addressable_interface_result self)
    -> PolicyInterfaceCapture& {
  return select_owner<PolicyInterfaceCapture>(self.self);
}

static void TTX_CALL
    policy_interface_pass(ttx_addressable_interface_result self) {
  PolicyInterfaceCapture& capture = select(self);
  if (capture.answered) {
    capture.passes = false;
    capture.relation = TTX_INTERFACE_UNKNOWN;
    return;
  }
  capture.answered = true;
  capture.passes = true;
}

static void TTX_CALL policy_interface_answer(
    ttx_addressable_interface_result self,
    ttx_interface_relation relation) {
  PolicyInterfaceCapture& capture = select(self);
  if (capture.answered || relation < TTX_INTERFACE_UNKNOWN ||
      relation > TTX_INTERFACE_EQUIVALENT) {
    capture.answered = true;
    capture.passes = false;
    capture.relation = TTX_INTERFACE_UNKNOWN;
    return;
  }
  capture.answered = true;
  capture.passes = false;
  capture.relation = relation;
}

static auto select(ttx_domain_result self) -> PolicyDomainCapture& {
  return select_owner<PolicyDomainCapture>(self.self);
}

static void TTX_CALL policy_domain_unknown(ttx_domain_result self) {
  PolicyDomainCapture& capture = select(self);
  capture.answered = true;
  capture.stopped = true;
  capture.result.operations->unknown(capture.result);
}

static void TTX_CALL policy_domain_none(ttx_domain_result self) {
  PolicyDomainCapture& capture = select(self);
  capture.answered = true;
}

static void TTX_CALL policy_domain_resolved(
    ttx_domain_result self,
    ttx_abstract domain,
    ttx_layout layout) {
  PolicyDomainCapture& capture = select(self);
  capture.answered = true;
  capture.stopped = true;
  // Consume a synthetic policy shape while its dispatch frame is still alive.
  // Only the final consumer decides whether keeping it merits a snapshot.
  capture.result.operations->resolved(capture.result, domain, layout);
}

static auto select(ttx_callable_result self) -> PolicyCallableCapture& {
  return select_owner<PolicyCallableCapture>(self.self);
}

static void TTX_CALL policy_callable_unknown(ttx_callable_result self) {
  PolicyCallableCapture& capture = select(self);
  capture.answered = true;
  capture.answer = Addressable::LayerAnswer<CallableObservation>::stop({
    .state = CallableObservationState::Unknown,
    .callable = {},
  });
}

static void TTX_CALL policy_callable_none(ttx_callable_result self) {
  PolicyCallableCapture& capture = select(self);
  capture.answered = true;
  capture.answer = Addressable::LayerAnswer<CallableObservation>::pass();
}

static void policy_callable_failed(
    ttx_callable_result self,
    ttx_pack_support_failure failure) {
  auto& capture = select(self);
  capture.answered = true;
  capture.answer = Addressable::LayerAnswer<CallableObservation>::stop(
      {.state = CallableObservationState::SupportFailed,
       .callable = {},
       .failure = failure});
}

static void TTX_CALL
    policy_callable_resolved(ttx_callable_result self, ttx_callable callable) {
  PolicyCallableCapture& capture = select(self);
  capture.answered = true;
  capture.answer = Addressable::LayerAnswer<CallableObservation>::stop(
      Ttx::retain_callable(callable));
}

static auto select(ttx_route_result self) -> PolicyRouteCapture& {
  return select_owner<PolicyRouteCapture>(self.self);
}

static void TTX_CALL policy_route_unknown(ttx_route_result self) {
  PolicyRouteCapture& capture = select(self);
  capture.answered = true;
  capture.answer = Addressable::LayerAnswer<RouteObservation>::stop({
    .state = Observation::Unknown,
    .route = {},
    .candidate = ttx_unknown(),
    .bytes = {},
  });
}

static void TTX_CALL policy_route_none(ttx_route_result self) {
  PolicyRouteCapture& capture = select(self);
  capture.answered = true;
  capture.answer = Addressable::LayerAnswer<RouteObservation>::pass();
}

static void TTX_CALL
    policy_route_resolved(ttx_route_result self, ttx_route route) {
  PolicyRouteCapture& capture = select(self);
  capture.answered = true;
  capture.answer = Addressable::LayerAnswer<RouteObservation>::stop(
      Ttx::retain_route(route));
}

static auto select(ttx_finite_extent_result self) -> PolicyExtentCapture& {
  return select_owner<PolicyExtentCapture>(self.self);
}

static void TTX_CALL policy_extent_unknown(ttx_finite_extent_result self) {
  PolicyExtentCapture& capture = select(self);
  capture.answered = true;
  capture.answer = Addressable::LayerAnswer<ExtentObservation>::stop({
    .state = Observation::Unknown,
    .extent = {},
  });
}

static void TTX_CALL policy_extent_none(ttx_finite_extent_result self) {
  PolicyExtentCapture& capture = select(self);
  capture.answered = true;
  capture.answer = Addressable::LayerAnswer<ExtentObservation>::pass();
}

static void TTX_CALL policy_extent_resolved(
    ttx_finite_extent_result self,
    ttx_finite_extent extent) {
  PolicyExtentCapture& capture = select(self);
  capture.answered = true;
  capture.answer = Addressable::LayerAnswer<ExtentObservation>::stop(
      Ttx::retain_finite_extent(extent));
}

static auto select(ttx_bytes_result self) -> PolicyBytesCapture& {
  return select_owner<PolicyBytesCapture>(self.self);
}

static void TTX_CALL policy_bytes_unknown(ttx_bytes_result self) {
  PolicyBytesCapture& capture = select(self);
  capture.answered = true;
  capture.answer = Addressable::LayerAnswer<BytesObservation>::stop({
    .state = Observation::Unknown,
    .bytes = {},
  });
}

static void TTX_CALL policy_bytes_none(ttx_bytes_result self) {
  PolicyBytesCapture& capture = select(self);
  capture.answered = true;
  capture.answer = Addressable::LayerAnswer<BytesObservation>::pass();
}

static void TTX_CALL
    policy_bytes_resolved(ttx_bytes_result self, ttx_bytes bytes) {
  PolicyBytesCapture& capture = select(self);
  capture.answered = true;
  capture.answer = Addressable::LayerAnswer<BytesObservation>::stop(
      Ttx::retain_bytes(bytes));
}

static void TTX_CALL
    offer_route(ttx_concept_sink self, ttx_borrowed_bytes route, ttx_abstract) {
  OfferedRoutes& offered = select(self);
  if (offered.completed || (route.size != 0 && route.data == nullptr)) {
    offered.completed = true;
    offered.routes.clear();
    return;
  }
  std::vector<uint8_t> copied;
  if (route.size != 0) {
    copied.assign(route.data, route.data + route.size);
  }
  if (std::find(offered.routes.begin(), offered.routes.end(), copied) ==
      offered.routes.end()) {
    offered.routes.push_back(std::move(copied));
  }
}

static void TTX_CALL offers_completed(ttx_concept_sink self) {
  select(self).completed = true;
}

static auto valid_policy(ttx_addressable_policy policy) -> bool {
  return supports(policy.operations, sizeof(ttx_addressable_policy_ops)) &&
         policy.operations->resolve_concept != nullptr &&
         policy.operations->visit_concepts != nullptr &&
         policy.operations->interface != nullptr &&
         policy.operations->resolve_domain != nullptr &&
         policy.operations->resolve_callable != nullptr &&
         policy.operations->resolve_route != nullptr &&
         policy.operations->resolve_finite_extent != nullptr &&
         policy.operations->resolve_bytes != nullptr &&
         policy.operations->invoke != nullptr;
}

static auto observe_concept(
    ttx_addressable_policy policy,
    ttx_abstract candidate,
    ttx_borrowed_bytes route) -> ttx_abstract {
  if (!valid_policy(policy)) {
    return ttx_unknown();
  }
  PolicyAbstractCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_abstract_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .answer = policy_abstract_answer,
        },
    .answered = false,
    .answer = ttx_unknown(),
  };
  policy.operations->resolve_concept(
      policy, candidate, route,
      {
        .operations = &capture.operations,
        .self = reinterpret_cast<ttx_abstract_sink_self*>(&capture),
      });
  return capture.answered ? capture.answer : ttx_unknown();
}

static auto observe_interface(
    ttx_addressable_policy policy,
    ttx_abstract candidate,
    ttx_abstract requirement) -> Addressable::InterfaceAnswer {
  if (!valid_policy(policy)) {
    return Addressable::InterfaceAnswer::Unknown;
  }
  PolicyInterfaceCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_addressable_interface_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .pass = policy_interface_pass,
          .answer = policy_interface_answer,
        },
    .answered = false,
    .passes = false,
    .relation = TTX_INTERFACE_UNKNOWN,
  };
  policy.operations->interface(
      policy, candidate, requirement,
      {
        .operations = &capture.operations,
        .self =
            reinterpret_cast<ttx_addressable_interface_result_self*>(&capture),
      });
  if (!capture.answered) {
    return Addressable::InterfaceAnswer::Unknown;
  }
  if (capture.passes) {
    return Addressable::InterfaceAnswer::Pass;
  }
  switch (capture.relation) {
  case TTX_INTERFACE_REJECTED:
    return Addressable::InterfaceAnswer::Rejected;
  case TTX_INTERFACE_SATISFIED:
    return Addressable::InterfaceAnswer::Satisfied;
  case TTX_INTERFACE_EQUIVALENT:
    return Addressable::InterfaceAnswer::Equivalent;
  case TTX_INTERFACE_UNKNOWN:
    return Addressable::InterfaceAnswer::Unknown;
  }
  return Addressable::InterfaceAnswer::Unknown;
}

static auto observe_domain(
    ttx_addressable_policy policy,
    ttx_abstract candidate,
    ttx_domain_result result) -> bool {
  if (!valid_policy(policy)) {
    result.operations->unknown(result);
    return true;
  }
  static const ttx_domain_result_ops operations = {
    .header = {sizeof(ttx_domain_result_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
    .unknown = policy_domain_unknown,
    .none = policy_domain_none,
    .resolved = policy_domain_resolved,
  };
  PolicyDomainCapture capture{result};
  policy.operations->resolve_domain(
      policy, candidate,
      {
        .operations = &operations,
        .self = reinterpret_cast<ttx_domain_result_self*>(&capture),
      });
  if (!capture.answered) {
    result.operations->unknown(result);
    return true;
  }
  return capture.stopped;
}

static auto observe_callable(
    ttx_addressable_policy policy,
    ttx_abstract candidate) -> Addressable::LayerAnswer<CallableObservation> {
  if (!valid_policy(policy)) {
    return Addressable::LayerAnswer<CallableObservation>::stop({
      .state = CallableObservationState::Unknown,
      .callable = {},
    });
  }
  PolicyCallableCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_callable_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .unknown = policy_callable_unknown,
          .none = policy_callable_none,
          .resolved = policy_callable_resolved,
          .support_failed = policy_callable_failed,
        },
    .answered = false,
    .answer = Addressable::LayerAnswer<CallableObservation>::stop({
      .state = CallableObservationState::Unknown,
      .callable = {},
    }),
  };
  policy.operations->resolve_callable(
      policy, candidate,
      {
        .operations = &capture.operations,
        .self = reinterpret_cast<ttx_callable_result_self*>(&capture),
      });
  return capture.answer;
}

static auto observe_route(ttx_addressable_policy policy, ttx_abstract candidate)
    -> Addressable::LayerAnswer<RouteObservation> {
  if (!valid_policy(policy)) {
    return Addressable::LayerAnswer<RouteObservation>::stop({
      .state = Observation::Unknown,
      .route = {},
      .candidate = ttx_unknown(),
      .bytes = {},
    });
  }
  PolicyRouteCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_route_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .unknown = policy_route_unknown,
          .none = policy_route_none,
          .resolved = policy_route_resolved,
        },
    .answered = false,
    .answer = Addressable::LayerAnswer<RouteObservation>::stop({
      .state = Observation::Unknown,
      .route = {},
      .candidate = ttx_unknown(),
      .bytes = {},
    }),
  };
  policy.operations->resolve_route(
      policy, candidate,
      {
        .operations = &capture.operations,
        .self = reinterpret_cast<ttx_route_result_self*>(&capture),
      });
  return capture.answer;
}

static auto observe_extent(
    ttx_addressable_policy policy,
    ttx_abstract candidate) -> Addressable::LayerAnswer<ExtentObservation> {
  if (!valid_policy(policy)) {
    return Addressable::LayerAnswer<ExtentObservation>::stop({
      .state = Observation::Unknown,
      .extent = {},
    });
  }
  PolicyExtentCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_finite_extent_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .unknown = policy_extent_unknown,
          .none = policy_extent_none,
          .resolved = policy_extent_resolved,
        },
    .answered = false,
    .answer = Addressable::LayerAnswer<ExtentObservation>::stop({
      .state = Observation::Unknown,
      .extent = {},
    }),
  };
  policy.operations->resolve_finite_extent(
      policy, candidate,
      {
        .operations = &capture.operations,
        .self = reinterpret_cast<ttx_finite_extent_result_self*>(&capture),
      });
  return capture.answer;
}

static auto observe_bytes(ttx_addressable_policy policy, ttx_abstract candidate)
    -> Addressable::LayerAnswer<BytesObservation> {
  if (!valid_policy(policy)) {
    return Addressable::LayerAnswer<BytesObservation>::stop({
      .state = Observation::Unknown,
      .bytes = {},
    });
  }
  PolicyBytesCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_bytes_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .unknown = policy_bytes_unknown,
          .none = policy_bytes_none,
          .resolved = policy_bytes_resolved,
        },
    .answered = false,
    .answer = Addressable::LayerAnswer<BytesObservation>::stop({
      .state = Observation::Unknown,
      .bytes = {},
    }),
  };
  policy.operations->resolve_bytes(
      policy, candidate,
      {
        .operations = &capture.operations,
        .self = reinterpret_cast<ttx_bytes_result_self*>(&capture),
      });
  return capture.answer;
}

static auto select(ttx_interface self) -> LayeredInterface& {
  if (self == nullptr) {
    std::abort();
  }
  return *const_cast<LayeredInterface*>(
      static_cast<const LayeredInterface*>(self));
}

static auto TTX_CALL interface_requirement(ttx_interface self) -> ttx_abstract {
  return select(self).requirement;
}

static auto TTX_CALL interface_candidate(ttx_interface self) -> ttx_abstract {
  return select(self).candidate;
}

static auto TTX_CALL interface_relation(ttx_interface self)
    -> ttx_interface_relation {
  return select(self).relation;
}

static void TTX_CALL interface_invoke(
    ttx_interface self,
    ttx_abstract operation,
    ttx_pack input,
    ttx_context context,
    ttx_pack_result result) {
  LayeredInterface& selected = select(self);
  if (selected.relation == TTX_INTERFACE_UNKNOWN) {
    result.operations->unknown(result);
    return;
  }
  if (selected.relation != TTX_INTERFACE_SATISFIED &&
      selected.relation != TTX_INTERFACE_EQUIVALENT) {
    result.operations->none(result);
    return;
  }
  if (selected.policy.operations != nullptr) {
    selected.policy.operations->invoke(
        selected.policy, selected.candidate, selected.requirement, operation,
        input, context, result);
    return;
  }
  if (selected.delegated == nullptr ||
      !supports(selected.delegated->operations, sizeof(ttx_interface_ops))) {
    result.operations->none(result);
    return;
  }
  selected.delegated->operations->invoke(
      selected.delegated, operation, input, context, result);
}

static void publish_interface(
    const Addressable& owner,
    ttx_addressable_policy policy,
    ttx_interface delegated,
    ttx_abstract requirement,
    ttx_interface_relation relation,
    ttx_interface_sink result) {
  const ttx_abstract candidate = owner.get_abi();
  static const ttx_interface_ops operations = {
    .header =
        {
          .size = sizeof(ttx_interface_ops),
          .abi_major = TTX_ABI_MAJOR,
          .abi_minor = TTX_ABI_MINOR,
        },
    .requirement = interface_requirement,
    .candidate = interface_candidate,
    .negotiate = interface_relation,
    .invoke = interface_invoke,
  };
  LayeredInterface answer(
      &operations, &owner, policy, delegated, requirement, candidate, relation);
  result.operations->answer(result, &answer);
}

static auto select(ttx_interface_sink self) -> DelegatedInterface& {
  return select_owner<DelegatedInterface>(self.self);
}

static void TTX_CALL
    delegated_answer(ttx_interface_sink self, ttx_interface delegated) {
  DelegatedInterface& capture = select(self);
  capture.answered = true;
  if (delegated == nullptr ||
      !supports(delegated->operations, TTX_INTERFACE_RELATION_PREFIX_SIZE) ||
      !ttx_abstract_same(
          delegated->operations->requirement(delegated), capture.requirement) ||
      !ttx_abstract_same(
          delegated->operations->candidate(delegated), capture.source)) {
    publish_interface(
        *capture.owner, {}, {}, capture.requirement, TTX_INTERFACE_UNKNOWN,
        capture.result);
    return;
  }
  const ttx_abstract requirement =
      delegated->operations->requirement(delegated);
  ttx_interface_relation relation = delegated->operations->negotiate(delegated);
  if (relation == TTX_INTERFACE_EQUIVALENT &&
      !ttx_abstract_same(capture.candidate, requirement)) {
    relation = TTX_INTERFACE_SATISFIED;
  }
  publish_interface(
      *capture.owner, {}, delegated, requirement, relation, capture.result);
}

static auto to_interface_relation(Addressable::InterfaceAnswer answer)
    -> ttx_interface_relation {
  switch (answer) {
  case Addressable::InterfaceAnswer::Unknown:
    return TTX_INTERFACE_UNKNOWN;
  case Addressable::InterfaceAnswer::Rejected:
    return TTX_INTERFACE_REJECTED;
  case Addressable::InterfaceAnswer::Satisfied:
    return TTX_INTERFACE_SATISFIED;
  case Addressable::InterfaceAnswer::Equivalent:
    return TTX_INTERFACE_EQUIVALENT;
  case Addressable::InterfaceAnswer::Pass:
    std::abort();
  }
  std::abort();
}

Addressable::Layer::Layer() = default;

const ttx_addressable_policy_ops Addressable::Layer::operations = {
  .header =
      {
        .size = sizeof(ttx_addressable_policy_ops),
        .abi_major = TTX_ABI_MAJOR,
        .abi_minor = TTX_ABI_MINOR,
      },
  .resolve_concept = resolve_concept_abi,
  .visit_concepts = visit_concepts_abi,
  .interface = interface_abi,
  .resolve_domain = domain_abi,
  .resolve_callable = callable_abi,
  .resolve_route = route_abi,
  .resolve_finite_extent = extent_abi,
  .resolve_bytes = bytes_abi,
  .invoke = invoke_abi,
};

auto Addressable::Layer::get_abi() const -> ttx_addressable_policy {
  return {
    .operations = &operations,
    .self = reinterpret_cast<ttx_addressable_policy_self*>(
        const_cast<Layer*>(this)),
  };
}

auto Addressable::Layer::select(ttx_addressable_policy self) -> const Layer& {
  return select_owner<const Layer>(self.self);
}

void Addressable::Layer::resolve_concept_abi(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_borrowed_bytes route,
    ttx_abstract_sink result) {
  result.operations->answer(
      result, select(self).resolve_concept(candidate, route));
}

void Addressable::Layer::visit_concepts_abi(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_concept_sink result) {
  select(self).visit_concepts(candidate, result);
}

void Addressable::Layer::interface_abi(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_abstract requirement,
    ttx_addressable_interface_result result) {
  const InterfaceAnswer answer = select(self).interface(candidate, requirement);
  if (answer == InterfaceAnswer::Pass) {
    result.operations->pass(result);
    return;
  }
  result.operations->answer(result, to_interface_relation(answer));
}

void Addressable::Layer::domain_abi(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_domain_result result) {
  select(self).domain(candidate, result);
}

void Addressable::Layer::callable_abi(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_callable_result result) {
  const LayerAnswer<CallableObservation> answer =
      select(self).callable(candidate);
  if (!answer.stops || answer.answer.state == CallableObservationState::None) {
    result.operations->none(result);
  } else if (answer.answer.state == CallableObservationState::Unknown) {
    result.operations->unknown(result);
  } else if (answer.answer.state == CallableObservationState::SupportFailed) {
    result.operations->support_failed(result, answer.answer.failure);
  } else {
    result.operations->resolved(result, answer.answer.callable);
  }
}

void Addressable::Layer::route_abi(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_route_result result) {
  const LayerAnswer<RouteObservation> answer = select(self).route(candidate);
  if (!answer.stops || answer.answer.state == Observation::None) {
    result.operations->none(result);
  } else if (answer.answer.state == Observation::Unknown) {
    result.operations->unknown(result);
  } else {
    result.operations->resolved(result, answer.answer.route);
  }
}

void Addressable::Layer::extent_abi(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_finite_extent_result result) {
  const LayerAnswer<ExtentObservation> answer = select(self).extent(candidate);
  if (!answer.stops || answer.answer.state == Observation::None) {
    result.operations->none(result);
  } else if (answer.answer.state == Observation::Unknown) {
    result.operations->unknown(result);
  } else {
    result.operations->resolved(result, answer.answer.extent);
  }
}

void Addressable::Layer::bytes_abi(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_bytes_result result) {
  const LayerAnswer<BytesObservation> answer = select(self).bytes(candidate);
  if (!answer.stops || answer.answer.state == Observation::None) {
    result.operations->none(result);
  } else if (answer.answer.state == Observation::Unknown) {
    result.operations->unknown(result);
  } else {
    result.operations->resolved(result, answer.answer.bytes);
  }
}

void Addressable::Layer::invoke_abi(
    ttx_addressable_policy self,
    ttx_abstract candidate,
    ttx_abstract requirement,
    ttx_abstract operation,
    ttx_pack input,
    ttx_context context,
    ttx_pack_result result) {
  select(self).invoke(
      candidate, requirement, operation, input, context, result);
}

auto Addressable::Layer::resolve_concept(ttx_abstract, ttx_borrowed_bytes) const
    -> ttx_abstract {
  return ttx_none();
}

void Addressable::Layer::visit_concepts(ttx_abstract, ttx_concept_sink result)
    const {
  result.operations->completed(result);
}

void Addressable::Layer::domain(ttx_abstract, ttx_domain_result result) const {
  result.operations->none(result);
}

auto Addressable::Layer::callable(ttx_abstract) const
    -> LayerAnswer<CallableObservation> {
  return LayerAnswer<CallableObservation>::pass();
}

auto Addressable::Layer::route(ttx_abstract) const
    -> LayerAnswer<RouteObservation> {
  return LayerAnswer<RouteObservation>::pass();
}

auto Addressable::Layer::extent(ttx_abstract) const
    -> LayerAnswer<ExtentObservation> {
  return LayerAnswer<ExtentObservation>::pass();
}

auto Addressable::Layer::bytes(ttx_abstract) const
    -> LayerAnswer<BytesObservation> {
  return LayerAnswer<BytesObservation>::pass();
}

auto Addressable::Layer::interface(ttx_abstract, ttx_abstract) const
    -> InterfaceAnswer {
  return InterfaceAnswer::Pass;
}

void Addressable::Layer::invoke(
    ttx_abstract,
    ttx_abstract,
    ttx_abstract,
    ttx_pack,
    ttx_context,
    ttx_pack_result result) const {
  result.operations->none(result);
}

static auto policy_handles(
    const std::vector<std::shared_ptr<const Addressable::Layer>>& layers)
    -> std::vector<ttx_addressable_policy> {
  std::vector<ttx_addressable_policy> result;
  result.reserve(layers.size());
  for (const std::shared_ptr<const Addressable::Layer>& layer : layers) {
    if (layer == nullptr) {
      result.push_back({});
    } else {
      result.push_back(layer->get_abi());
    }
  }
  return result;
}

Addressable::Addressable(
    ttx_abstract referent,
    std::vector<std::shared_ptr<const Layer>> layers)
    : referent(Ttx::resolve(referent)),
      retained_layers(std::move(layers)),
      policies(policy_handles(retained_layers)) {
  if (this->referent == nullptr ||
      !supports(this->referent->operations, sizeof(ttx_abstract_ops)) ||
      ttx_abstract_same(this->referent, ttx_none()) ||
      std::find(retained_layers.begin(), retained_layers.end(), nullptr) !=
          retained_layers.end() ||
      std::find_if(
          policies.begin(), policies.end(), [](ttx_addressable_policy policy) {
            return !valid_policy(policy);
          }) != policies.end()) {
    std::abort();
  }
}

Addressable::Addressable(
    ttx_abstract referent,
    std::vector<ttx_addressable_policy> policies)
    : referent(Ttx::resolve(referent)),
      retained_layers(),
      policies(std::move(policies)) {
  if (this->referent == nullptr ||
      !supports(this->referent->operations, sizeof(ttx_abstract_ops)) ||
      ttx_abstract_same(this->referent, ttx_none()) ||
      std::find_if(
          this->policies.begin(), this->policies.end(),
          [](ttx_addressable_policy policy) {
            return !valid_policy(policy);
          }) != this->policies.end()) {
    std::abort();
  }
}

auto Addressable::name() const -> ttx_borrowed_bytes {
  return referent->operations->name(referent);
}

auto Addressable::documentation(ttx_abstract) const -> ttx_documentation {
  return referent->operations->documentation(referent);
}

auto Addressable::resolve(ttx_abstract self) const -> ttx_abstract {
  return self;
}

auto Addressable::resolve_concept(ttx_borrowed_bytes route) const
    -> ttx_abstract {
  const ttx_abstract candidate = get_abi();
  for (ttx_addressable_policy policy : policies) {
    const ttx_abstract answer = observe_concept(policy, candidate, route);
    if (answer == nullptr ||
        !supports(answer->operations, sizeof(ttx_abstract_ops))) {
      return ttx_unknown();
    }
    if (!ttx_abstract_same(answer, ttx_none())) {
      return answer;
    }
  }
  return Ttx::resolve_concept(referent, route);
}

void Addressable::visit_concepts(ttx_concept_sink result) const {
  OfferedRoutes offered = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_concept_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .item = offer_route,
          .completed = offers_completed,
        },
    .routes = {},
    .completed = false,
  };
  const ttx_concept_sink collector = {
    .operations = &offered.operations,
    .self = reinterpret_cast<ttx_concept_sink_self*>(&offered),
  };
  for (ttx_addressable_policy policy : policies) {
    offered.completed = false;
    if (!valid_policy(policy)) {
      result.operations->completed(result);
      return;
    }
    policy.operations->visit_concepts(policy, get_abi(), collector);
    if (!offered.completed) {
      result.operations->completed(result);
      return;
    }
  }
  offered.completed = false;
  referent->operations->visit_concepts(referent, collector);
  if (!offered.completed) {
    result.operations->completed(result);
    return;
  }
  for (const std::vector<uint8_t>& route : offered.routes) {
    const ttx_borrowed_bytes name = {
      .data = route.data(),
      .size = route.size(),
    };
    const ttx_abstract answer = resolve_concept(name);
    if (!ttx_abstract_same(answer, ttx_unknown()) &&
        !ttx_abstract_same(answer, ttx_none())) {
      result.operations->item(result, name, answer);
    }
  }
  result.operations->completed(result);
}

void Addressable::interface(
    ttx_abstract self,
    ttx_abstract requirement,
    ttx_interface_sink result) const {
  if (ttx_abstract_same(self, requirement)) {
    publish_interface(
        *this, {}, {}, requirement, TTX_INTERFACE_EQUIVALENT, result);
    return;
  }
  if (ttx_abstract_same(requirement, ttx_addressable_requirement())) {
    publish_interface(
        *this, {}, {}, requirement, TTX_INTERFACE_SATISFIED, result);
    return;
  }
  for (ttx_addressable_policy policy : policies) {
    const InterfaceAnswer answer = observe_interface(policy, self, requirement);
    if (answer != InterfaceAnswer::Pass) {
      ttx_interface_relation selected = to_interface_relation(answer);
      if (selected == TTX_INTERFACE_EQUIVALENT &&
          !ttx_abstract_same(self, requirement)) {
        selected = TTX_INTERFACE_SATISFIED;
      }
      publish_interface(*this, policy, {}, requirement, selected, result);
      return;
    }
  }
  DelegatedInterface capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_interface_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .answer = delegated_answer,
        },
    .owner = this,
    .source = referent,
    .requirement = requirement,
    .candidate = self,
    .result = result,
    .answered = false,
  };
  const ttx_interface_sink sink = {
    .operations = &capture.operations,
    .self = reinterpret_cast<ttx_interface_sink_self*>(&capture),
  };
  referent->operations->interface(referent, requirement, sink);
  if (!capture.answered) {
    publish_interface(
        *this, {}, {}, requirement, TTX_INTERFACE_UNKNOWN, result);
  }
}

void Addressable::domain(ttx_abstract self, ttx_domain_result result) const {
  for (ttx_addressable_policy policy : policies) {
    const bool stopped = observe_domain(policy, self, result);
    if (stopped) {
      return;
    }
  }
  referent->operations->resolve_domain(referent, result);
}

void Addressable::callable(ttx_abstract self, ttx_callable_result result)
    const {
  CallableObservation selected = {
    .state = CallableObservationState::Unknown, .callable = {}};
  bool stopped = false;
  for (ttx_addressable_policy policy : policies) {
    const LayerAnswer<CallableObservation> answer =
        observe_callable(policy, self);
    if (!answer.stops) {
      continue;
    }
    switch (answer.answer.state) {
    case CallableObservationState::Unknown:
      result.operations->unknown(result);
      return;
    case CallableObservationState::None:
      continue;
    case CallableObservationState::SupportFailed:
      result.operations->support_failed(result, answer.answer.failure);
      return;
    case CallableObservationState::Resolved:
      selected = answer.answer;
      stopped = true;
      break;
    }
    if (stopped) {
      break;
    }
  }
  if (!stopped) {
    selected = Ttx::resolve_callable(referent);
  }
  if (selected.state == CallableObservationState::SupportFailed) {
    result.operations->support_failed(result, selected.failure);
    return;
  }
  if (selected.state == CallableObservationState::Unknown) {
    result.operations->unknown(result);
    return;
  }
  if (selected.state == CallableObservationState::None) {
    result.operations->none(result);
    return;
  }
  // The caller received authority over this Addressable. Keep that subject
  // visible while lending only the call shape supplied by the selected layer.
  struct View {
    ttx_abstract candidate;
    ttx_callable delegate;
  } view{self, selected.callable};
  static const ttx_callable_ops ops = {
    .header = {sizeof(ttx_callable_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
    .candidate =
        [](ttx_callable v) {
          return reinterpret_cast<const View*>(v.self)->candidate;
        },
    .parameters =
        [](ttx_callable v) {
          const auto d = reinterpret_cast<const View*>(v.self)->delegate;
          return d.operations->parameters(d);
        },
    .results =
        [](ttx_callable v) {
          const auto d = reinterpret_cast<const View*>(v.self)->delegate;
          return d.operations->results(d);
        },
  };
  result.operations->resolved(
      result, {.operations = &ops,
               .self = reinterpret_cast<ttx_callable_self*>(&view)});
}

void Addressable::route(ttx_abstract self, ttx_route_result result) const {
  RouteObservation selected = {
    .state = Observation::Unknown,
    .route = {},
    .candidate = ttx_unknown(),
    .bytes = {}};
  bool stopped = false;
  for (ttx_addressable_policy policy : policies) {
    auto answer = observe_route(policy, self);
    if (!answer.stops) {
      continue;
    }
    selected = std::move(answer.answer);
    stopped = true;
    break;
  }
  if (!stopped) {
    selected = Ttx::resolve_route(referent);
  }
  if (selected.state == Observation::Unknown) {
    result.operations->unknown(result);
    return;
  }
  if (selected.state == Observation::None) {
    result.operations->none(result);
    return;
  }
  struct View {
    ttx_abstract candidate;
    ttx_route source;
  } view{self, selected.route};
  static const ttx_route_ops ops = {
    .header = {sizeof(ttx_route_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
    .candidate =
        [](ttx_route v) {
          return reinterpret_cast<const View*>(v.self)->candidate;
        },
    .bytes = [](ttx_route v) -> ttx_borrowed_bytes {
      const auto d = reinterpret_cast<const View*>(v.self)->source;
      return d.operations->bytes(d);
    },
  };
  result.operations->resolved(
      result,
      {.operations = &ops, .self = reinterpret_cast<ttx_route_self*>(&view)});
}

void Addressable::finite_extent(
    ttx_abstract self,
    ttx_finite_extent_result result) const {
  ExtentObservation selected = {.state = Observation::Unknown, .extent = {}};
  bool stopped = false;
  for (ttx_addressable_policy policy : policies) {
    auto answer = observe_extent(policy, self);
    if (!answer.stops) {
      continue;
    }
    selected = std::move(answer.answer);
    stopped = true;
    break;
  }
  if (!stopped) {
    selected = Ttx::resolve_finite_extent(referent);
  }
  if (selected.state == Observation::Unknown) {
    result.operations->unknown(result);
    return;
  }
  if (selected.state == Observation::None) {
    result.operations->none(result);
    return;
  }
  struct View {
    ttx_abstract candidate;
    ttx_finite_extent source;
  } view{self, selected.extent};
  static const ttx_finite_extent_ops ops = {
    .header = {sizeof(ttx_finite_extent_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
    .candidate =
        [](ttx_finite_extent v) {
          return reinterpret_cast<const View*>(v.self)->candidate;
        },
    .cardinality = [](ttx_finite_extent v) -> uint64_t {
      const auto d = reinterpret_cast<const View*>(v.self)->source;
      return d.operations->cardinality(d);
    },
  };
  result.operations->resolved(
      result, {.operations = &ops,
               .self = reinterpret_cast<ttx_finite_extent_self*>(&view)});
}

void Addressable::bytes(ttx_abstract self, ttx_bytes_result result) const {
  BytesObservation selected = {.state = Observation::Unknown, .bytes = {}};
  bool stopped = false;
  for (ttx_addressable_policy policy : policies) {
    auto answer = observe_bytes(policy, self);
    if (!answer.stops) {
      continue;
    }
    selected = std::move(answer.answer);
    stopped = true;
    break;
  }
  if (!stopped) {
    selected = Ttx::resolve_bytes(referent);
  }
  if (selected.state == Observation::Unknown) {
    result.operations->unknown(result);
    return;
  }
  if (selected.state == Observation::None) {
    result.operations->none(result);
    return;
  }
  struct View {
    ttx_abstract candidate;
    ttx_bytes source;
  } view{self, selected.bytes};
  static const ttx_bytes_ops ops = {
    .header = {sizeof(ttx_bytes_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
    .candidate =
        [](ttx_bytes v) {
          return reinterpret_cast<const View*>(v.self)->candidate;
        },
    .size =
        [](ttx_bytes v) {
          const auto d = reinterpret_cast<const View*>(v.self)->source;
          return d.operations->size(d);
        },
    .visit =
        [](ttx_bytes v, ttx_bytes_sink sink) {
          const auto d = reinterpret_cast<const View*>(v.self)->source;
          d.operations->visit(d, sink);
        },
  };
  result.operations->resolved(
      result,
      {.operations = &ops, .self = reinterpret_cast<ttx_bytes_self*>(&view)});
}
