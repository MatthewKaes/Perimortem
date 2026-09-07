// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "ttx/concept/interface.hpp"

#include "validation/unit_test.hpp"

#include <string>

#include "tetrodotoxin/terminal/graph_text.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/callable.hpp"
#include "ttx/concept/domain.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/extent.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/value.hpp"
#include "ttx/model/route.hpp"
#include "ttx/query.hpp"
#include "validation/cross_language/bridge.h"

using namespace Perimortem::Core;
using namespace Validation;

static Harness InterfaceTests = {
  .name = "TTX::Interface"_view,
};

class FlowDomain : public Ttx::Domain {
 public:
  FlowDomain() : shape(get_abi()) {}
  auto layout() const -> ttx_layout override { return shape.get_abi(); }

 private:
  const Ttx::Layouts::Value shape;
};

class TemporaryDomain : public Ttx::Abstract {
 public:
  explicit TemporaryDomain(std::vector<ttx_abstract> producers)
      : producers(std::move(producers)) {}

  void domain(ttx_abstract self, ttx_domain_result result) const override {
    std::vector<Ttx::Layouts::Fluid::Entry> entries;
    for (size_t index = 0; index < producers.size(); ++index) {
      const std::string path = std::to_string(index);
      entries.push_back(
          {.path = std::vector<uint8_t>(path.begin(), path.end()),
           .producer = producers[index]});
    }
    Ttx::Layouts::Fluid shape(std::move(entries));
    result.operations->resolved(result, self, shape.get_abi());
  }

 private:
  const std::vector<ttx_abstract> producers;
};

class DomainPolicy : public Ttx::Addressable::Layer {
 public:
  explicit DomainPolicy(const TemporaryDomain& owner) : owner(owner) {}
  Ttx::Observation state = Ttx::Observation::Unknown;

  void domain(ttx_abstract, ttx_domain_result result) const override {
    switch (state) {
    case Ttx::Observation::Unknown:
      result.operations->unknown(result);
      return;
    case Ttx::Observation::None:
      result.operations->none(result);
      return;
    case Ttx::Observation::Resolved:
      owner.domain(owner.get_abi(), result);
      return;
    }
  }

 private:
  const TemporaryDomain& owner;
};

struct DomainPack {
  ttx_context context;
  Ttx::PackObservation pack;
};

static void TTX_CALL pack_unknown_domain(ttx_domain_result result) {
  reinterpret_cast<DomainPack*>(result.self)->pack.state =
      Ttx::PackObservationState::Unknown;
}

static void TTX_CALL pack_absent_domain(ttx_domain_result result) {
  reinterpret_cast<DomainPack*>(result.self)->pack.state =
      Ttx::PackObservationState::None;
}

static void TTX_CALL
    pack_domain(ttx_domain_result result, ttx_abstract, ttx_layout layout) {
  auto& capture = *reinterpret_cast<DomainPack*>(result.self);
  capture.pack = Ttx::pack(capture.context, layout);
}

PERIMORTEM_UNIT_TEST(InterfaceTests, domain_shape_ends_before_consumption) {
  FlowDomain first, second;
  TemporaryDomain owner({first.get_abi(), second.get_abi()});
  auto policy = std::make_shared<DomainPolicy>(owner);
  Ttx::Addressable layered(first.get_abi(), {policy});
  EXPECT(
      Ttx::resolve_domain(layered.get_abi()).state ==
      Ttx::Observation::Unknown);
  policy->state = Ttx::Observation::None;
  EXPECT(ttx_abstract_same(
      Ttx::resolve_domain(layered.get_abi()).domain, first.get_abi()));
  policy->state = Ttx::Observation::Resolved;
  const auto identity = Ttx::resolve_domain(layered.get_abi());
  EXPECT(identity.state == Ttx::Observation::Resolved);
  EXPECT(ttx_abstract_same(identity.domain, owner.get_abi()));

  DomainPack captured{.context = ttx_context_create(), .pack = {}};
  static const ttx_domain_result_ops operations = {
    .header = {sizeof(ttx_domain_result_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
    .unknown = pack_unknown_domain,
    .none = pack_absent_domain,
    .resolved = pack_domain,
  };
  Ttx::resolve_domain(
      layered.get_abi(),
      {
        .operations = &operations,
        .self = reinterpret_cast<ttx_domain_result_self*>(&captured),
      });
  EXPECT(captured.pack.state == Ttx::PackObservationState::Packed);
  const auto producers = Ttx::producers(captured.pack.pack);
  EXPECT(producers.has_value());
  if (producers) {
    EXPECT(producers->size() == 2);
    EXPECT(ttx_abstract_same((*producers)[0], first.get_abi()));
    EXPECT(ttx_abstract_same((*producers)[1], second.get_abi()));
  }

  // Graph Text queues shape traversal after Domain resolution has returned.
  // This exercises its snapshot ownership with the same temporary producer.
  const auto text = Tetrodotoxin::Terminal::GraphText::write(
      {}, ttx_unknown(), layered.get_abi(), owner.get_abi());
  const std::string report(text.begin(), text.end());
  EXPECT(report.find("  enumerable 2\n") != std::string::npos);
  captured.context.operations->release(captured.context);
}

class UnretainedDomain : public Ttx::Abstract {
 public:
  void domain(ttx_abstract self, ttx_domain_result result) const override {
    class Shape : public Ttx::Layout {
     public:
      void enumerable(ttx_enumerable_result result) const override {
        result.operations->rejected(result);
      }
      void snapshot(ttx_layout_snapshot_result result) const override {
        result.operations->support_failed(result, TTX_PACK_SUPPORT_EXHAUSTED);
      }
    } shape;
    result.operations->resolved(result, self, shape.get_abi());
  }
};

PERIMORTEM_UNIT_TEST(
    InterfaceTests,
    domain_storage_failure_preserves_identity) {
  UnretainedDomain owner;
  const auto identity = Ttx::resolve_domain(owner.get_abi());
  EXPECT(identity.state == Ttx::Observation::Resolved);
  EXPECT(ttx_abstract_same(identity.domain, owner.get_abi()));
  const auto text = Tetrodotoxin::Terminal::GraphText::write(
      {}, ttx_unknown(), owner.get_abi(), owner.get_abi());
  const std::string report(text.begin(), text.end());
  EXPECT(report.find("  domain support-failed\n") != std::string::npos);
}

class Contract final : public Ttx::Abstract {
 public:
  explicit Contract(View::Bytes name) : name(name) {}

  auto get_name() const -> View::Bytes override { return name; }

 private:
  const View::Bytes name;
};

class ContractCandidate final : public Ttx::Abstract {
 public:
  explicit ContractCandidate(ttx_abstract supported) : supported(supported) {}

  void interface(
      ttx_abstract self,
      ttx_abstract requirement,
      ttx_interface_sink result) const override {
    const Ttx::Interface answer(
        requirement, self,
        ttx_abstract_same(requirement, supported) ? TTX_INTERFACE_SATISFIED
                                                  : TTX_INTERFACE_REJECTED);
    answer.publish(result);
  }

 private:
  const ttx_abstract supported;
};

PERIMORTEM_UNIT_TEST(InterfaceTests, candidate_synthesizes_exact_witness) {
  Contract supported("Supported"_view);
  Contract unrelated("Unrelated"_view);
  ContractCandidate candidate(supported.get_abi());

  EXPECT(
      Ttx::relation(candidate.get_abi(), supported.get_abi()) ==
      TTX_INTERFACE_SATISFIED);
  EXPECT(
      Ttx::relation(candidate.get_abi(), unrelated.get_abi()) ==
      TTX_INTERFACE_REJECTED);
  EXPECT_NOT(ttx_abstract_same(candidate.get_abi(), supported.get_abi()));
}

class IndependentOwner {
 public:
  struct State {
    View::Bytes name;
    ttx_abstract supported;
  };

  class Projection final : public Ttx::Abstract {
   public:
    explicit Projection(const State& state) : state(state) {}

    auto get_name() const -> View::Bytes override { return state.name; }

    void interface(
        ttx_abstract self,
        ttx_abstract requirement,
        ttx_interface_sink result) const override {
      const Ttx::Interface answer(
          requirement, self,
          ttx_abstract_same(requirement, state.supported)
              ? TTX_INTERFACE_SATISFIED
              : TTX_INTERFACE_REJECTED);
      answer.publish(result);
    }

   private:
    const State& state;
  };

  IndependentOwner(ttx_abstract first_contract, ttx_abstract second_contract)
      : first_state{"First"_view, first_contract},
        second_state{"Second"_view, second_contract},
        first(first_state),
        second(second_state) {}

  const State first_state;
  const State second_state;
  const Projection first;
  const Projection second;
};

PERIMORTEM_UNIT_TEST(InterfaceTests, owner_composes_independent_capabilities) {
  Contract first_contract("FirstContract"_view);
  Contract second_contract("SecondContract"_view);
  IndependentOwner owner(first_contract.get_abi(), second_contract.get_abi());

  EXPECT_NOT(ttx_abstract_same(owner.first.get_abi(), owner.second.get_abi()));
  EXPECT(
      Ttx::relation(owner.first.get_abi(), first_contract.get_abi()) ==
      TTX_INTERFACE_SATISFIED);
  EXPECT(
      Ttx::relation(owner.first.get_abi(), second_contract.get_abi()) ==
      TTX_INTERFACE_REJECTED);
  EXPECT(
      Ttx::relation(owner.second.get_abi(), second_contract.get_abi()) ==
      TTX_INTERFACE_SATISFIED);
}

class FlowCandidate : public Ttx::Abstract {
 public:
  FlowCandidate(ttx_abstract requirement, ttx_abstract operation)
      : requirement(requirement), operation(operation) {}

  ttx_interface_relation relationship = TTX_INTERFACE_UNKNOWN;
  ttx_abstract advertised = nullptr;
  mutable unsigned calls = 0;

  void interface(
      ttx_abstract self,
      ttx_abstract requested,
      ttx_interface_sink result) const override {
    class Witness : public Ttx::Interface {
     public:
      Witness(
          const FlowCandidate& owner,
          ttx_abstract requested,
          ttx_abstract candidate)
          : Interface(
                requested,
                candidate,
                ttx_abstract_same(requested, owner.requirement)
                    ? owner.relationship
                    : TTX_INTERFACE_REJECTED),
            owner(owner) {}

     private:
      void invoke(
          ttx_abstract operation,
          ttx_pack input,
          ttx_context context,
          ttx_pack_result result) const override {
        if (!ttx_abstract_same(operation, owner.operation)) {
          result.operations->none(result);
          return;
        }
        ++owner.calls;
        context.operations->pack(
            context, input.operations->layout(input), result);
      }
      const FlowCandidate& owner;
    } witness(*this, requested, advertised ? advertised : self);
    witness.publish(result);
  }

 private:
  const ttx_abstract requirement;
  const ttx_abstract operation;
};

PERIMORTEM_UNIT_TEST(InterfaceTests, uncertainty_and_rejection_do_not_execute) {
  Contract requirement("Flow"_view), operation("pass"_view);
  FlowCandidate owner(requirement.get_abi(), operation.get_abi());
  const auto context = ttx_context_create();
  const auto input = Ttx::pack(context, ttx_empty_layout());
  auto invoke = [&] {
    return Ttx::invoke(
        owner.get_abi(), requirement.get_abi(), operation.get_abi(), input.pack,
        context);
  };
  EXPECT(invoke().state == Ttx::PackObservationState::Unknown);
  owner.relationship = TTX_INTERFACE_REJECTED;
  EXPECT(invoke().state == Ttx::PackObservationState::None);
  EXPECT(owner.calls == 0);
  owner.relationship = TTX_INTERFACE_SATISFIED;
  EXPECT(invoke().state == Ttx::PackObservationState::Packed);
  EXPECT(owner.calls == 1);
  context.operations->release(context);
}

PERIMORTEM_UNIT_TEST(InterfaceTests, witness_cannot_substitute_its_candidate) {
  Contract requirement("Flow"_view), operation("pass"_view),
      other("Other"_view);
  FlowCandidate owner(requirement.get_abi(), operation.get_abi());
  owner.relationship = TTX_INTERFACE_SATISFIED;
  owner.advertised = other.get_abi();
  const auto context = ttx_context_create();
  const auto input = Ttx::pack(context, ttx_empty_layout());
  const auto observed = Ttx::invoke(
      owner.get_abi(), requirement.get_abi(), operation.get_abi(), input.pack,
      context);
  EXPECT(observed.state == Ttx::PackObservationState::SupportFailed);
  EXPECT(owner.calls == 0);
  context.operations->release(context);
}

PERIMORTEM_UNIT_TEST(InterfaceTests, alias_forwards_the_complete_negotiation) {
  Contract requirement("Flow"_view), operation("pass"_view),
      other("Other"_view);
  FlowCandidate owner(requirement.get_abi(), operation.get_abi());
  owner.relationship = TTX_INTERFACE_SATISFIED;
  Ttx::Alias alias;
  EXPECT(
      Ttx::relation(alias.get_abi(), requirement.get_abi()) ==
      TTX_INTERFACE_UNKNOWN);
  EXPECT_NOT(alias.bind(alias.get_abi()));
  EXPECT_NOT(alias.bind(ttx_none()));
  EXPECT(alias.bind(owner.get_abi()));
  EXPECT_NOT(alias.bind(other.get_abi()));
  EXPECT(
      Ttx::relation(alias.get_abi(), requirement.get_abi()) ==
      TTX_INTERFACE_SATISFIED);
  const auto context = ttx_context_create();
  const auto input = Ttx::pack(context, ttx_empty_layout());
  EXPECT(
      Ttx::invoke(
          alias.get_abi(), requirement.get_abi(), operation.get_abi(),
          input.pack, context)
          .state == Ttx::PackObservationState::Packed);
  EXPECT(owner.calls == 1);
  context.operations->release(context);
}

class FlowPolicy : public Ttx::Addressable::Layer {
 public:
  Ttx::Addressable::InterfaceAnswer answer =
      Ttx::Addressable::InterfaceAnswer::Unknown;
  auto interface(ttx_abstract, ttx_abstract) const
      -> Ttx::Addressable::InterfaceAnswer override {
    return answer;
  }
};

PERIMORTEM_UNIT_TEST(InterfaceTests, policy_requeries_before_forwarding) {
  Contract requirement("Flow"_view), operation("pass"_view);
  FlowCandidate source(requirement.get_abi(), operation.get_abi());
  source.relationship = TTX_INTERFACE_SATISFIED;
  auto policy = std::make_shared<FlowPolicy>();
  Ttx::Addressable view(source.get_abi(), {policy});
  const auto context = ttx_context_create();
  const auto input = Ttx::pack(context, ttx_empty_layout());
  auto invoke = [&] {
    return Ttx::invoke(
        view.get_abi(), requirement.get_abi(), operation.get_abi(), input.pack,
        context);
  };
  EXPECT(invoke().state == Ttx::PackObservationState::Unknown);
  policy->answer = Ttx::Addressable::InterfaceAnswer::Rejected;
  EXPECT(invoke().state == Ttx::PackObservationState::None);
  EXPECT(source.calls == 0);
  policy->answer = Ttx::Addressable::InterfaceAnswer::Pass;
  EXPECT(invoke().state == Ttx::PackObservationState::Packed);
  EXPECT(source.calls == 1);
  policy->answer = Ttx::Addressable::InterfaceAnswer::Unknown;
  EXPECT(invoke().state == Ttx::PackObservationState::Unknown);
  EXPECT(source.calls == 1);
  context.operations->release(context);
}

class TemporaryCallable : public Ttx::Abstract {
 public:
  explicit TemporaryCallable(ttx_abstract value) : value(value) {}
  ttx_abstract value;
  bool fail = false;
  void callable(ttx_abstract self, ttx_callable_result result) const override {
    if (fail) {
      result.operations->support_failed(result, TTX_PACK_SUPPORT_EXHAUSTED);
      return;
    }
    Ttx::Layouts::Value layout(value);
    struct Projection {
      ttx_abstract candidate;
      ttx_layout layout;
    } projection{self, layout.get_abi()};
    const ttx_callable_ops operations = {
      .header = {sizeof(ttx_callable_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
      .candidate =
          [](ttx_callable p) {
            return reinterpret_cast<Projection*>(p.self)->candidate;
          },
      .parameters =
          [](ttx_callable p) {
            return reinterpret_cast<Projection*>(p.self)->layout;
          },
      .results =
          [](ttx_callable p) {
            return reinterpret_cast<Projection*>(p.self)->layout;
          },
    };
    result.operations->resolved(
        result, {.operations = &operations,
                 .self = reinterpret_cast<ttx_callable_self*>(&projection)});
  }

 protected:
  auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation override {
    return ttx_abstract_same(requirement, ttx_callable_requirement())
               ? TTX_INTERFACE_SATISFIED
               : Abstract::negotiate(requirement);
  }
};

PERIMORTEM_UNIT_TEST(
    InterfaceTests,
    callable_snapshot_outlives_synthetic_view) {
  FlowDomain first, second;
  TemporaryCallable source(first.get_abi());
  Ttx::Addressable view(source.get_abi());
  const auto before = Ttx::resolve_callable(view.get_abi());
  source.value = second.get_abi();
  const auto after = Ttx::resolve_callable(view.get_abi());
  ASSERT(before.state == Ttx::CallableObservationState::Resolved);
  ASSERT(after.state == Ttx::CallableObservationState::Resolved);
  EXPECT(ttx_abstract_same(
      before.callable.operations->candidate(before.callable), view.get_abi()));
  const auto context = ttx_context_create();
  const auto old = Ttx::pack(
      context, before.callable.operations->parameters(before.callable));
  const auto current =
      Ttx::pack(context, after.callable.operations->parameters(after.callable));
  EXPECT(
      Ttx::producers(old.pack).value() ==
      std::vector<ttx_abstract>{first.get_abi()});
  EXPECT(
      Ttx::producers(current.pack).value() ==
      std::vector<ttx_abstract>{second.get_abi()});
  source.fail = true;
  const auto failure = Ttx::resolve_callable(view.get_abi());
  EXPECT(failure.state == Ttx::CallableObservationState::SupportFailed);
  EXPECT(failure.failure == TTX_PACK_SUPPORT_EXHAUSTED);
  context.operations->release(context);
}

PERIMORTEM_UNIT_TEST(InterfaceTests, rust_layout_can_end_before_its_pack) {
  FlowDomain producer;
  const auto context = ttx_context_create();
  struct Capture {
    ttx_pack pack = {};
    bool failed = false;
  } capture;
  const ttx_pack_result_ops ops = {
    .header = {sizeof(ttx_pack_result_ops), TTX_ABI_MAJOR, TTX_ABI_MINOR},
    .unknown =
        [](ttx_pack_result r) {
          reinterpret_cast<Capture*>(r.self)->failed = true;
        },
    .none =
        [](ttx_pack_result r) {
          reinterpret_cast<Capture*>(r.self)->failed = true;
        },
    .packed = [](ttx_pack_result r,
                 ttx_pack p) { reinterpret_cast<Capture*>(r.self)->pack = p; },
    .support_failed =
        [](ttx_pack_result r, ttx_pack_support_failure) {
          reinterpret_cast<Capture*>(r.self)->failed = true;
        },
  };
  rust_temporary_pack(
      producer.get_abi(), context,
      {.operations = &ops,
       .self = reinterpret_cast<ttx_pack_result_self*>(&capture)});
  EXPECT_NOT(capture.failed);
  ASSERT(capture.pack.operations != nullptr);
  EXPECT(
      Ttx::producers(capture.pack).value() ==
      std::vector<ttx_abstract>{producer.get_abi()});
  context.operations->release(context);
}

PERIMORTEM_UNIT_TEST(
    InterfaceTests,
    constant_views_preserve_the_layer_candidate) {
  Ttx::Route source({0xff, 0x00, 0x01});
  Ttx::Extent count(4);
  Ttx::Addressable route(source.get_abi()), extent(count.get_abi());
  const auto r = Ttx::resolve_route(route.get_abi());
  const auto e = Ttx::resolve_finite_extent(extent.get_abi());
  ASSERT(r.state == Ttx::Observation::Resolved);
  ASSERT(e.state == Ttx::Observation::Resolved);
  EXPECT(ttx_abstract_same(r.candidate, route.get_abi()));
  EXPECT(r.bytes.size == 3 && r.bytes.data[0] == 0xff && r.bytes.data[1] == 0);
  EXPECT(ttx_abstract_same(
      e.extent.operations->candidate(e.extent), extent.get_abi()));
  EXPECT(e.extent.operations->cardinality(e.extent) == 4);
}

PERIMORTEM_UNIT_TEST(
    InterfaceTests,
    nested_witnesses_keep_each_visible_subject) {
  Contract requirement("Flow"_view), operation("pass"_view);
  FlowCandidate source(requirement.get_abi(), operation.get_abi());
  source.relationship = TTX_INTERFACE_SATISFIED;
  std::vector<std::unique_ptr<Ttx::Addressable>> layers;
  ttx_abstract current = source.get_abi();
  for (unsigned index = 0; index < 128; ++index) {
    auto layer = std::make_unique<Ttx::Addressable>(current);
    current = layer->get_abi();
    layers.push_back(std::move(layer));
  }
  const auto context = ttx_context_create();
  const auto input = Ttx::pack(context, ttx_empty_layout());
  EXPECT(
      Ttx::invoke(
          current, requirement.get_abi(), operation.get_abi(), input.pack,
          context)
          .state == Ttx::PackObservationState::Packed);
  EXPECT(source.calls == 1);
  context.operations->release(context);
}
