// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "cross_language/bridge.h"
#include "cross_language/cpp/echo.hpp"
#include "cross_language/cpp/root.hpp"
#include "cross_language/cpp/ttx.hpp"
#include "cross_language/cpp/value.hpp"
#include "ttx/callable.hpp"
#include "ttx/composite.hpp"
#include "ttx/finite_extent.hpp"
#include "ttx/named.hpp"
#include "ttx/query.hpp"
#include "ttx/ranged.hpp"
#include "ttx/route.hpp"
#include "ttx/value_layout.hpp"

static uint64_t failures;

static void fail(const char* message) {
  std::fprintf(stderr, "%s\n", message);
  ++failures;
}

static auto valid(ttx_abstract value) -> bool {
  return value.operations != nullptr &&
         value.operations->header.abi_major == TTX_ABI_MAJOR &&
         value.operations->header.size >= sizeof(ttx_abstract_ops);
}

class EmptyCallable final : public Ttx::Callable {
 public:
  auto name() const -> ttx_borrowed_bytes override {
    static const uint8_t value[] = "C++ empty Callable";
    return {value, sizeof(value) - 1};
  }

  auto parameters() const -> ttx_layout override { return ttx_empty_layout(); }
  auto results() const -> ttx_layout override { return ttx_empty_layout(); }
};

struct NamedCapture {
  ttx_named_result_ops operations;
  bool answered;
  bool satisfied;
  ttx_named named;
};

struct CompositeCapture {
  ttx_composite_layout_result_ops operations;
  bool answered;
  bool satisfied;
  ttx_composite_layout composite;
};

struct RouteSummary {
  ttx_named_route_sink_ops operations;
  uint64_t unknown;
  uint64_t none;
  uint64_t exact;
  bool matched;
  bool completed;
};

enum class SelectionState {
  Unanswered,
  Unknown,
  None,
  Selected,
};

struct SelectionCapture {
  ttx_named_selection_result_ops operations;
  SelectionState state;
  std::vector<uint8_t> path;
};

static void TTX_CALL named_rejected(ttx_named_result self) {
  static_assert(offsetof(NamedCapture, operations) == 0);
  auto& capture = *reinterpret_cast<NamedCapture*>(
      const_cast<ttx_named_result_ops*>(self.operations));
  capture.answered = true;
}

static void TTX_CALL named_satisfied(ttx_named_result self, ttx_named named) {
  static_assert(offsetof(NamedCapture, operations) == 0);
  auto& capture = *reinterpret_cast<NamedCapture*>(
      const_cast<ttx_named_result_ops*>(self.operations));
  capture.answered = true;
  capture.satisfied = true;
  capture.named = named;
}

static auto composite_capture(ttx_composite_layout_result self)
    -> CompositeCapture& {
  static_assert(offsetof(CompositeCapture, operations) == 0);
  return *reinterpret_cast<CompositeCapture*>(
      const_cast<ttx_composite_layout_result_ops*>(self.operations));
}

static void TTX_CALL composite_rejected(ttx_composite_layout_result self) {
  CompositeCapture& capture = composite_capture(self);
  capture.answered = true;
  capture.satisfied = false;
}

static void TTX_CALL composite_satisfied(
    ttx_composite_layout_result self,
    ttx_composite_layout composite) {
  CompositeCapture& capture = composite_capture(self);
  capture.answered = true;
  capture.satisfied = true;
  capture.composite = composite;
}

static auto query_composite(ttx_layout layout) -> CompositeCapture {
  CompositeCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_composite_layout_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .rejected = composite_rejected,
          .satisfied = composite_satisfied,
        },
    .answered = false,
    .satisfied = false,
    .composite = {},
  };
  const ttx_composite_layout_result result = {
    .operations = &capture.operations,
    .owner = layout.owner,
    .value = layout.value,
  };
  layout.operations->composite(layout, result);
  return capture;
}

static auto route_summary(ttx_named_route_sink self) -> RouteSummary& {
  static_assert(offsetof(RouteSummary, operations) == 0);
  return *reinterpret_cast<RouteSummary*>(
      const_cast<ttx_named_route_sink_ops*>(self.operations));
}

static void TTX_CALL
    route_unknown(ttx_named_route_sink self, ttx_borrowed_bytes) {
  ++route_summary(self).unknown;
}

static void TTX_CALL route_none(ttx_named_route_sink self, ttx_borrowed_bytes) {
  ++route_summary(self).none;
}

static void TTX_CALL route_exact(
    ttx_named_route_sink self,
    ttx_borrowed_bytes,
    ttx_borrowed_bytes route) {
  RouteSummary& summary = route_summary(self);
  ++summary.exact;
  summary.matched = route.size == 2 && route.data != nullptr &&
                    route.data[0] == '.' && route.data[1] == 'g';
}

static void TTX_CALL route_completed(ttx_named_route_sink self) {
  route_summary(self).completed = true;
}

static auto selection_capture(ttx_named_selection_result self)
    -> SelectionCapture& {
  static_assert(offsetof(SelectionCapture, operations) == 0);
  return *reinterpret_cast<SelectionCapture*>(
      const_cast<ttx_named_selection_result_ops*>(self.operations));
}

static void TTX_CALL selection_unknown(ttx_named_selection_result self) {
  selection_capture(self).state = SelectionState::Unknown;
}

static void TTX_CALL selection_none(ttx_named_selection_result self) {
  selection_capture(self).state = SelectionState::None;
}

static void TTX_CALL selection_selected(
    ttx_named_selection_result self,
    ttx_borrowed_bytes path) {
  SelectionCapture& capture = selection_capture(self);
  if (path.size != 0 && path.data == nullptr) {
    capture.state = SelectionState::Unknown;
    return;
  }
  capture.state = SelectionState::Selected;
  capture.path.clear();
  if (path.size != 0) {
    capture.path.assign(path.data, path.data + path.size);
  }
}

static auto select_named(ttx_named named, ttx_borrowed_bytes route)
    -> SelectionCapture {
  SelectionCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_named_selection_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .unknown = selection_unknown,
          .none = selection_none,
          .selected = selection_selected,
        },
    .state = SelectionState::Unanswered,
    .path = {},
  };
  const ttx_named_selection_result result = {
    .operations = &capture.operations,
    .owner = named.owner,
    .value = named.value,
  };
  named.operations->select(named, route, result);
  return capture;
}

static auto named_snapshot_is_complete(ttx_pack pack) -> bool {
  const ttx_layout layout = pack.operations->layout(pack);
  NamedCapture capture = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_named_result_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .rejected = named_rejected,
          .satisfied = named_satisfied,
        },
    .answered = false,
    .satisfied = false,
    .named = {},
  };
  const ttx_named_result result = {
    .operations = &capture.operations,
    .owner = layout.owner,
    .value = layout.value,
  };
  layout.operations->named(layout, result);
  if (!capture.answered || !capture.satisfied ||
      capture.named.operations == nullptr) {
    return false;
  }
  const ttx_layout candidate =
      capture.named.operations->candidate(capture.named);
  if (candidate.owner != layout.owner || candidate.value != layout.value) {
    return false;
  }
  RouteSummary summary = {
    .operations =
        {
          .header =
              {
                .size = sizeof(ttx_named_route_sink_ops),
                .abi_major = TTX_ABI_MAJOR,
                .abi_minor = TTX_ABI_MINOR,
              },
          .unknown = route_unknown,
          .none = route_none,
          .route = route_exact,
          .completed = route_completed,
        },
    .unknown = 0,
    .none = 0,
    .exact = 0,
    .matched = false,
    .completed = false,
  };
  const ttx_named_route_sink visitor = {
    .operations = &summary.operations,
    .owner = layout.owner,
    .value = layout.value,
  };
  capture.named.operations->visit_routes(capture.named, visitor);
  return summary.completed && summary.unknown == 1 && summary.none == 1 &&
         summary.exact == 1 && summary.matched;
}

int main() {
  static_assert(sizeof(ttx_abstract) == sizeof(void*) + 16);
  static_assert(sizeof(ttx_layout) == sizeof(void*) + 16);
  static_assert(sizeof(ttx_pack) == sizeof(void*) + 16);

  TtxTest::install(ttx_authority_create());
  const ttx_context context = ttx_context_create();
  const ttx_test_rust_exports rust = rust_test_exports(
      ttx_authority_create(), ttx_test_create_bytes_terminal());
  if (!valid(rust.echo) || !valid(rust.view_bytes) || !valid(rust.value)) {
    fail("Rust did not export complete Abstract carriers.");
    return EXIT_FAILURE;
  }

  static const uint8_t print_name[] = "print";
  static const uint8_t to_string_name[] = "to_string";
  const ttx_borrowed_bytes print = {print_name, sizeof(print_name) - 1};
  const ttx_borrowed_bytes to_string = {
    to_string_name,
    sizeof(to_string_name) - 1,
  };
  static const uint8_t value_name[] = "value";
  static const uint8_t echo_name[] = "echo";
  const ttx_borrowed_bytes value_route = {
    value_name,
    sizeof(value_name) - 1,
  };
  const ttx_borrowed_bytes echo_route = {
    echo_name,
    sizeof(echo_name) - 1,
  };
  const ttx_abstract print_operation =
      TtxTest::resolve_concept(rust.echo, print);
  const ttx_abstract to_string_operation =
      TtxTest::resolve_concept(rust.value, to_string);

  const Ttx::DomainObservation echo_domain = Ttx::resolve_domain(rust.echo);
  const Ttx::DomainObservation view_domain =
      Ttx::resolve_domain(rust.view_bytes);
  if (echo_domain.state != Ttx::Observation::Resolved ||
      !ttx_abstract_same(echo_domain.domain, rust.echo) ||
      view_domain.state != Ttx::Observation::Resolved ||
      !ttx_abstract_same(view_domain.domain, rust.view_bytes) ||
      Ttx::resolve_domain(ttx_unknown()).state != Ttx::Observation::Unknown ||
      Ttx::resolve_domain(ttx_none()).state != Ttx::Observation::None ||
      Ttx::resolve_callable(rust.echo).state != Ttx::Observation::None ||
      Ttx::resolve_callable(ttx_unknown()).state != Ttx::Observation::Unknown) {
    fail("Domain or Callable observations lost their factual outcomes.");
  }
  EmptyCallable empty_callable;
  const Ttx::CallableObservation callable =
      Ttx::resolve_callable(empty_callable.get_abi());
  if (callable.state != Ttx::Observation::Resolved ||
      callable.callable.operations == nullptr ||
      !ttx_abstract_same(
          callable.callable.operations->candidate(callable.callable),
          empty_callable.get_abi())) {
    fail("The C++ Callable view did not preserve its exact candidate.");
  }
  Ttx::Route empty_route({});
  Ttx::Route binary_route({0xFF, 0x00, 0xA7});
  Ttx::Alias route_alias(binary_route.get_abi());
  const Ttx::RouteObservation empty_route_view =
      Ttx::resolve_route(empty_route.get_abi());
  const Ttx::RouteObservation binary_route_view =
      Ttx::resolve_route(route_alias.get_abi());
  if (empty_route_view.state != Ttx::Observation::Resolved ||
      empty_route_view.bytes.size != 0 ||
      binary_route_view.state != Ttx::Observation::Resolved ||
      binary_route_view.bytes.size != 3 ||
      binary_route_view.bytes.data[0] != 0xFF ||
      binary_route_view.bytes.data[1] != 0x00 ||
      binary_route_view.bytes.data[2] != 0xA7 ||
      !ttx_abstract_same(binary_route_view.candidate, binary_route.get_abi()) ||
      Ttx::relation(binary_route.get_abi(), ttx_constant_requirement()) !=
          TTX_INTERFACE_SATISFIED) {
    fail("Route observation changed an empty or non-textual atomic question.");
  }

  const ttx_abstract integer = ttx_test_c_integer_create(
      rust.value, rust.view_bytes, to_string_operation);
  const ttx_abstract boolean = ttx_test_c_boolean_create(
      rust.value, rust.view_bytes, to_string_operation);
  const ttx_abstract cpp_value = TtxTest::create_value(
      rust.value, rust.view_bytes, to_string_operation,
      TtxTest::Route{
        'v', 'a', 'l', 'u', 'e', ' ', 'f', 'r', 'o', 'm', ' ', 'C', '+', '+'});
  if (!valid(integer) || !valid(boolean) || !valid(cpp_value) ||
      ttx_abstract_same(integer, boolean)) {
    fail("Independent C and C++ Value owners failed construction.");
    return EXIT_FAILURE;
  }
  ttx_test_bytes_terminal_add(ttx_test_c_value_bytes_provider());
  ttx_test_bytes_terminal_add(TtxTest::value_bytes_provider());
  ttx_test_bytes_terminal_add(rust_echo_bytes_provider());

  if (TtxTest::relation(integer, rust.value) != TTX_INTERFACE_SATISFIED ||
      TtxTest::relation(integer, rust.view_bytes) != TTX_INTERFACE_SATISFIED ||
      TtxTest::relation(cpp_value, rust.value) != TTX_INTERFACE_SATISFIED ||
      TtxTest::relation(cpp_value, rust.view_bytes) !=
          TTX_INTERFACE_SATISFIED) {
    fail("A Value could not satisfy two unrelated Rust-owned requirements.");
  }
  if (Ttx::relation(integer, rust.value) != TTX_INTERFACE_SATISFIED ||
      Ttx::relation(ttx_unknown(), rust.value) != TTX_INTERFACE_UNKNOWN) {
    fail("The canonical C++ Interface observation changed Rust evidence.");
  }

  const ttx_abstract direct = rust_echo_create(integer);
  const ttx_abstract nested = rust_echo_create(boolean);
  const ttx_abstract cpp_backed = rust_echo_create(cpp_value);
  if (!valid(direct) || !valid(nested) || !valid(cpp_backed)) {
    fail("Rust coupled Echo construction to one language's Value model.");
    return EXIT_FAILURE;
  }

  const ttx_abstract transported = TtxTest::redispatch(direct);
  if (!ttx_abstract_same(transported, direct) ||
      transported.operations == direct.operations) {
    fail("C++ transport changed Rust semantic identity.");
    return EXIT_FAILURE;
  }

  const ttx_abstract unresolved = TtxTest::create_echo_simulacrum(
      rust.echo, print_operation, ttx_unknown());
  const ttx_abstract layered =
      TtxTest::create_echo_simulacrum(rust.echo, print_operation, nested);
  const ttx_abstract twice =
      TtxTest::create_echo_simulacrum(rust.echo, print_operation, layered);
  const ttx_test_policy_exports policy = rust_policy_exports();
  if (!valid(unresolved) || !valid(layered) || !valid(twice) ||
      !valid(policy.candidate) || !valid(policy.requirement) ||
      !valid(policy.operation)) {
    fail(
        "An independently registered simulacrum or policy failed "
        "construction.");
    return EXIT_FAILURE;
  }
  if (TtxTest::relation(direct, rust.echo) != TTX_INTERFACE_SATISFIED ||
      TtxTest::relation(direct, rust.view_bytes) != TTX_INTERFACE_SATISFIED ||
      TtxTest::relation(layered, rust.echo) != TTX_INTERFACE_SATISFIED ||
      TtxTest::relation(layered, rust.view_bytes) != TTX_INTERFACE_SATISFIED ||
      !ttx_abstract_same(
          TtxTest::resolve_concept(direct, value_route), integer) ||
      !ttx_abstract_same(
          TtxTest::resolve_concept(layered, echo_route), nested) ||
      !ttx_abstract_same(
          TtxTest::resolve_concept(layered, value_route), boolean)) {
    fail("The mixed-language tree did not preserve its synthetic contracts.");
  }
  if (ttx_abstract_same(unresolved, layered) ||
      valid(
          TtxTest::create_echo_simulacrum(
              rust.echo, print_operation, ttx_none()))) {
    fail("C++ collapsed distinct owners or accepted a rejected Echo child.");
  }

  const TtxTest::AliasBinding alias = TtxTest::make_alias(direct);
  if (!valid(alias.abstract) ||
      !ttx_abstract_same(TtxTest::spot_resolve(alias.abstract), direct) ||
      !alias.owner->bind(direct) || alias.owner->bind(boolean)) {
    fail("The reusable C++ Alias did not remain transparent.");
  }
  const TtxTest::AliasBinding collapsed = TtxTest::make_alias(alias.abstract);
  auto unbound = std::make_shared<Ttx::Alias>();
  if (!valid(collapsed.abstract) ||
      !ttx_abstract_same(collapsed.target, direct) ||
      unbound->bind(unbound->get_abi())) {
    fail("Alias construction retained a chain or admitted a cycle.");
  }

  TtxTest::Routes routes;
  routes.emplace_back(TtxTest::Route{'d', 'i', 'r', 'e', 'c', 't'}, direct);
  routes.emplace_back(
      TtxTest::Route{'l', 'a', 'y', 'e', 'r', 'e', 'd'}, layered);
  routes.emplace_back(
      TtxTest::Route{'c', 'p', 'p', '-', 'v', 'a', 'l', 'u', 'e'}, cpp_backed);
  routes.emplace_back(TtxTest::Route{'t', 'w', 'i', 'c', 'e'}, twice);
  routes.emplace_back(TtxTest::Route{}, unresolved);
  routes.emplace_back(TtxTest::Route{0xFF, 0x00, 0xA7}, policy.candidate);
  routes.emplace_back(
      TtxTest::Route{'n', 'o', 't', '-', 'e', 'c', 'h', 'o'}, ttx_none());
  const ttx_abstract root = TtxTest::create_root(std::move(routes));

  const ttx_test_discovery discovery =
      ttx_test_discover_empty(root, rust.echo, print_operation, context);
  if (discovery.visited != 7 || discovery.satisfied != 5 ||
      discovery.rejected != 2 || discovery.unknown != 0 ||
      discovery.equivalent != 0 || discovery.support_failed != 0) {
    fail("C did not discover the complete mixed-language Echo tree.");
  }

  const ttx_pack policy_input =
      TtxTest::retain_pack(context, {{TtxTest::Route{0}, cpp_value}});
  const ttx_test_receipt policy_result = ttx_test_consume_pack(
      policy.candidate, policy.requirement, policy.operation, policy_input,
      context);
  if (policy_result.outcome != TTX_TEST_CONSUMED_SATISFIED ||
      ttx_test_pack_cardinality(policy_result.result) != 1 ||
      !ttx_abstract_same(
          ttx_test_pack_first(policy_result.result), cpp_value) ||
      TtxTest::relation(policy.candidate, rust.echo) !=
          TTX_INTERFACE_REJECTED) {
    fail("The unrelated Rust policy did not preserve its opaque input Pack.");
  }

  const ttx_pack positional_source = TtxTest::retain_pack(
      context, {
                 {TtxTest::Route{0}, integer},
                 {TtxTest::Route{1}, boolean},
               });
  Ttx::Layouts::Value scalar_flow(integer);
  const Ttx::PackObservation scalar_source =
      Ttx::pack(context, scalar_flow.get_abi());
  Ttx::Layouts::Value scalar_receiver(rust.value);
  const Ttx::PackObservation fitted_scalar =
      Ttx::fit(scalar_receiver.get_abi(), scalar_source.pack, context);
  if (fitted_scalar.state != Ttx::PackObservationState::Packed ||
      ttx_test_pack_cardinality(fitted_scalar.pack) != 1 ||
      !ttx_abstract_same(ttx_test_pack_first(fitted_scalar.pack), integer)) {
    fail("Value fitting did not preserve its one exact source producer.");
  }

  ttx_pack composite_source = {};
  {
    Ttx::Layouts::Value left(integer);
    Ttx::Layouts::Value right(boolean);
    Ttx::Layouts::Composite flow(left.get_abi(), right.get_abi());
    const Ttx::PackObservation retained = Ttx::pack(context, flow.get_abi());
    if (retained.state == Ttx::PackObservationState::Packed) {
      composite_source = retained.pack;
    }
  }
  Ttx::Layouts::Value left_requirement(rust.value);
  Ttx::Layouts::Value right_requirement(rust.value);
  Ttx::Layouts::Composite composite_receiver(
      left_requirement.get_abi(), right_requirement.get_abi());
  const Ttx::PackObservation fitted_composite =
      Ttx::fit(composite_receiver.get_abi(), composite_source, context);
  CompositeCapture composite_view = {
    .operations = {},
    .answered = false,
    .satisfied = false,
    .composite = {},
  };
  if (fitted_composite.state == Ttx::PackObservationState::Packed) {
    composite_view = query_composite(
        fitted_composite.pack.operations->layout(fitted_composite.pack));
  }
  if (fitted_composite.state != Ttx::PackObservationState::Packed ||
      ttx_test_pack_cardinality(fitted_composite.pack) != 2 ||
      !ttx_abstract_same(ttx_test_pack_first(fitted_composite.pack), integer) ||
      !composite_view.answered || !composite_view.satisfied) {
    fail("Composite fitting flattened or lost one coupled child boundary.");
  }

  Ttx::Layouts::Value unknown_value(ttx_unknown());
  Ttx::Layouts::Value known_value(boolean);
  Ttx::Layouts::Composite partial_flow(
      unknown_value.get_abi(), known_value.get_abi());
  const Ttx::PackObservation partial_source =
      Ttx::pack(context, partial_flow.get_abi());
  const Ttx::PackObservation partial_composite =
      Ttx::fit(composite_receiver.get_abi(), partial_source.pack, context);
  if (partial_composite.state != Ttx::PackObservationState::Packed ||
      ttx_test_pack_cardinality(partial_composite.pack) != 2 ||
      !ttx_abstract_same(
          ttx_test_pack_first(partial_composite.pack), ttx_unknown())) {
    fail("Composite discarded an aligned partial child witness.");
  }

  Ttx::Extent extent_zero(0);
  Ttx::Extent extent_three(3);
  Ttx::Extent extent_four(4);
  Ttx::Alias extent_alias(extent_four.get_abi());
  const Ttx::ExtentObservation aliased_extent =
      Ttx::resolve_finite_extent(extent_alias.get_abi());
  if (aliased_extent.state != Ttx::Observation::Resolved ||
      aliased_extent.extent.operations->cardinality(aliased_extent.extent) !=
          4 ||
      Ttx::relation(extent_four.get_abi(), ttx_constant_requirement()) !=
          TTX_INTERFACE_SATISFIED) {
    fail("Extent did not remain a narrow Constant backed observation.");
  }
  Ttx::Layouts::Ranged ranged_flow(integer, extent_four.get_abi());
  const Ttx::PackObservation ranged_source =
      Ttx::pack(context, ranged_flow.get_abi());
  Ttx::Layouts::Ranged ranged_receiver(rust.value, extent_four.get_abi());
  const Ttx::PackObservation fitted_ranged =
      Ttx::fit(ranged_receiver.get_abi(), ranged_source.pack, context);
  if (ranged_source.state != Ttx::PackObservationState::Packed ||
      ttx_test_pack_cardinality(ranged_source.pack) != 4 ||
      fitted_ranged.state != Ttx::PackObservationState::Packed ||
      ttx_test_pack_cardinality(fitted_ranged.pack) != 4 ||
      !ttx_abstract_same(ttx_test_pack_first(fitted_ranged.pack), integer)) {
    fail("Ranged did not repeat one producer through an exact finite extent.");
  }
  Ttx::Layouts::Ranged mismatched_range(rust.value, extent_three.get_abi());
  if (Ttx::fit(mismatched_range.get_abi(), ranged_source.pack, context).state !=
      Ttx::PackObservationState::None) {
    fail("Ranged accepted unequal completed extents.");
  }
  Ttx::Layouts::Ranged unsettled_range(integer, ttx_unknown());
  if (Ttx::pack(context, unsettled_range.get_abi()).state !=
          Ttx::PackObservationState::SupportFailed ||
      Ttx::fit(unsettled_range.get_abi(), ranged_source.pack, context).state !=
          Ttx::PackObservationState::Unknown) {
    fail("Ranged laundered an unsettled extent into completed flow.");
  }
  Ttx::Layouts::Ranged empty_range(integer, extent_zero.get_abi());
  const Ttx::PackObservation empty_ranged =
      Ttx::pack(context, empty_range.get_abi());
  if (empty_ranged.state != Ttx::PackObservationState::Packed ||
      ttx_test_pack_cardinality(empty_ranged.pack) != 0) {
    fail("Ranged lost a completed zero extent or manufactured a value.");
  }

  Ttx::Layouts::Value third(cpp_value);
  Ttx::Layouts::Composite source_tail(known_value.get_abi(), third.get_abi());
  Ttx::Layouts::Composite source_association(
      scalar_flow.get_abi(), source_tail.get_abi());
  const Ttx::PackObservation associated_source =
      Ttx::pack(context, source_association.get_abi());
  Ttx::Layouts::Composite receiving_head(
      left_requirement.get_abi(), right_requirement.get_abi());
  Ttx::Layouts::Composite receiving_association(
      receiving_head.get_abi(), right_requirement.get_abi());
  if (Ttx::fit(receiving_association.get_abi(), associated_source.pack, context)
          .state != Ttx::PackObservationState::None) {
    fail("Composite implicitly reassociated equal enumerable leaves.");
  }
  Ttx::Layouts::Fluid positional_receiver({
    {.path = {0}, .producer = rust.value},
    {.path = {1}, .producer = rust.value},
  });
  const Ttx::PackObservation fitted =
      Ttx::fit(positional_receiver.get_abi(), positional_source, context);
  if (fitted.state != Ttx::PackObservationState::Packed ||
      ttx_test_pack_cardinality(fitted.pack) != 2 ||
      !ttx_abstract_same(ttx_test_pack_first(fitted.pack), integer)) {
    fail("Fluid fitting did not preserve the admitted source producers.");
  }

  Ttx::Layouts::Fluid mismatched_receiver({
    {.path = {0}, .producer = rust.value},
  });
  if (Ttx::fit(mismatched_receiver.get_abi(), positional_source, context)
          .state != Ttx::PackObservationState::None) {
    fail("Fluid fitting accepted a mismatched positional shape.");
  }

  const ttx_pack unknown_source =
      TtxTest::retain_pack(context, {{TtxTest::Route{0}, ttx_unknown()}});
  Ttx::Layouts::Fluid absent_flow({
    {.path = {0}, .producer = ttx_none()},
  });
  Ttx::Layouts::Fluid empty_domain_flow({
    {.path = {0}, .producer = rust.echo},
  });
  if (Ttx::pack(context, absent_flow.get_abi()).state !=
          Ttx::PackObservationState::SupportFailed ||
      Ttx::pack(context, empty_domain_flow.get_abi()).state !=
          Ttx::PackObservationState::SupportFailed) {
    fail("Context admitted semantic absence or an exact empty Domain as flow.");
  }
  const Ttx::PackObservation unsettled_fit =
      Ttx::fit(mismatched_receiver.get_abi(), unknown_source, context);
  if (unsettled_fit.state != Ttx::PackObservationState::Packed ||
      !ttx_abstract_same(
          ttx_test_pack_first(unsettled_fit.pack), ttx_unknown())) {
    fail("Fluid fitting discarded an aligned unsettled producer relationship.");
  }
  Ttx::Layouts::Fluid unknown_receiver({
    {.path = {0}, .producer = ttx_unknown()},
  });
  const Ttx::PackObservation partial =
      Ttx::fit(unknown_receiver.get_abi(), unknown_source, context);
  if (partial.state != Ttx::PackObservationState::Packed ||
      !ttx_abstract_same(ttx_test_pack_first(partial.pack), ttx_unknown())) {
    fail("Fluid fitting lost a position proven independently of its producer.");
  }

  Ttx::Route green_route({'.', 'g'});
  ttx_pack retained_named = {};
  {
    Ttx::Layouts::Fluid named_values({
      {.path = {0}, .producer = integer},
      {.path = {1}, .producer = boolean},
      {.path = {2}, .producer = cpp_value},
    });
    Ttx::Layouts::Fluid named_routes({
      {.path = {0}, .producer = ttx_unknown()},
      {.path = {1}, .producer = green_route.get_abi()},
      {.path = {2}, .producer = ttx_none()},
    });
    Ttx::Layouts::Named named(named_values.get_abi(), named_routes.get_abi());
    const Ttx::PackObservation retained = Ttx::pack(context, named.get_abi());
    if (retained.state == Ttx::PackObservationState::Packed) {
      retained_named = retained.pack;
    }
  }
  if (retained_named.operations == nullptr ||
      !named_snapshot_is_complete(retained_named)) {
    fail("Named did not retain partial routes after its source Layouts ended.");
  }

  {
    const ttx_layout retained_layout =
        retained_named.operations->layout(retained_named);
    NamedCapture retained_view = {
      .operations =
          {
            .header =
                {
                  .size = sizeof(ttx_named_result_ops),
                  .abi_major = TTX_ABI_MAJOR,
                  .abi_minor = TTX_ABI_MINOR,
                },
            .rejected = named_rejected,
            .satisfied = named_satisfied,
          },
      .answered = false,
      .satisfied = false,
      .named = {},
    };
    const ttx_named_result named_result = {
      .operations = &retained_view.operations,
      .owner = retained_layout.owner,
      .value = retained_layout.value,
    };
    retained_layout.operations->named(retained_layout, named_result);
    static const uint8_t green_bytes[] = ".g";
    const SelectionCapture unsettled = select_named(
        retained_view.named, {green_bytes, sizeof(green_bytes) - 1});
    if (unsettled.state != SelectionState::Unknown) {
      fail("Named selection ignored a route that could still match.");
    }
  }

  Ttx::Route red_route({'.', 'r'});
  {
    Ttx::Layouts::Fluid settled_values({
      {.path = {0}, .producer = integer},
      {.path = {1}, .producer = boolean},
    });
    Ttx::Layouts::Fluid settled_routes({
      {.path = {1}, .producer = green_route.get_abi()},
      {.path = {0}, .producer = red_route.get_abi()},
    });
    Ttx::Layouts::Named settled(
        settled_values.get_abi(), settled_routes.get_abi());
    NamedCapture view = {
      .operations =
          {
            .header =
                {
                  .size = sizeof(ttx_named_result_ops),
                  .abi_major = TTX_ABI_MAJOR,
                  .abi_minor = TTX_ABI_MINOR,
                },
            .rejected = named_rejected,
            .satisfied = named_satisfied,
          },
      .answered = false,
      .satisfied = false,
      .named = {},
    };
    const ttx_named_result named_result = {
      .operations = &view.operations,
      .owner = settled.get_abi().owner,
      .value = settled.get_abi().value,
    };
    settled.get_abi().operations->named(settled.get_abi(), named_result);
    const ttx_borrowed_bytes green = {
      reinterpret_cast<const uint8_t*>(".g"), 2};
    const ttx_borrowed_bytes missing = {
      reinterpret_cast<const uint8_t*>("Width"), 5};
    const SelectionCapture selected = select_named(view.named, green);
    const SelectionCapture absent = select_named(view.named, missing);
    if (!view.satisfied || selected.state != SelectionState::Selected ||
        selected.path != std::vector<uint8_t>{1} ||
        absent.state != SelectionState::None) {
      fail("Named selection did not use complete route identity by path.");
    }
  }

  {
    Ttx::Layouts::Fluid duplicate_values({
      {.path = {0}, .producer = integer},
      {.path = {1}, .producer = boolean},
    });
    Ttx::Layouts::Fluid duplicate_routes({
      {.path = {0}, .producer = green_route.get_abi()},
      {.path = {1}, .producer = green_route.get_abi()},
    });
    Ttx::Layouts::Named duplicate(
        duplicate_values.get_abi(), duplicate_routes.get_abi());
    NamedCapture view = {
      .operations =
          {
            .header =
                {
                  .size = sizeof(ttx_named_result_ops),
                  .abi_major = TTX_ABI_MAJOR,
                  .abi_minor = TTX_ABI_MINOR,
                },
            .rejected = named_rejected,
            .satisfied = named_satisfied,
          },
      .answered = false,
      .satisfied = false,
      .named = {},
    };
    const ttx_named_result named_result = {
      .operations = &view.operations,
      .owner = duplicate.get_abi().owner,
      .value = duplicate.get_abi().value,
    };
    duplicate.get_abi().operations->named(duplicate.get_abi(), named_result);
    if (!view.answered || view.satisfied) {
      fail("Named accepted duplicate completed routes.");
    }
  }

  if (TtxTest::relation(rust.echo, direct) != TTX_INTERFACE_REJECTED ||
      TtxTest::relation(ttx_unknown(), rust.echo) != TTX_INTERFACE_UNKNOWN) {
    fail("Directional or indeterminate Interface negotiation regressed.");
  }

  static const uint8_t arbitrary_bytes[] = {0xFF, 0x00, 0xA7};
  const ttx_borrowed_bytes arbitrary = {
    arbitrary_bytes,
    sizeof(arbitrary_bytes),
  };
  const ttx_abstract unknown = ttx_unknown();
  const ttx_abstract none = ttx_none();
  if (!ttx_abstract_same(
          TtxTest::resolve_concept(unknown, arbitrary), unknown) ||
      !ttx_abstract_same(TtxTest::resolve_concept(none, arbitrary), none) ||
      TtxTest::relation(none, none) != TTX_INTERFACE_EQUIVALENT ||
      TtxTest::relation(none, ttx_constant_requirement()) !=
          TTX_INTERFACE_SATISFIED) {
    fail("The canonical Unknown, None, and Constant laws did not close.");
  }

  context.operations->release(context);
  return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
