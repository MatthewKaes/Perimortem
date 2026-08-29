// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "ttx/bootstrap/concept/unknown.hpp"
#include "ttx/bootstrap/model/callable.hpp"
#include "ttx/bootstrap/model/layouts/fluid.hpp"
#include "ttx/bootstrap/model/layouts/named.hpp"
#include "ttx/bootstrap/model/type.hpp"
#include "ttx/model/interfaces/callable.h"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Validation;

static Harness InterfaceTests = {
  .name = "TTX::Interface"_view,
};

class InterfaceType final : public Type {
 public:
  TTX_NAME(name);
  TTX_EMPTY_DOCUMENTATION();

  constexpr InterfaceType(View::Bytes name) : name(name) {}

 private:
  View::Bytes name;
};

class InterfaceCallable final : public Callable {
 public:
  InterfaceCallable(
      View::Bytes name,
      const Layout& parameters,
      const Layout& results)
      : name(name), parameters(parameters), results(results) {}

  TTX_NAME(name);
  TTX_EMPTY_DOCUMENTATION();

  constexpr auto get_parameters() const -> const Layout& override {
    return parameters;
  }

  constexpr auto get_results() const -> const Layout& override {
    return results;
  }

 private:
  View::Bytes name;
  const Layout& parameters;
  const Layout& results;
};

PERIMORTEM_UNIT_TEST(InterfaceTests, callable_negotiation) {
  InterfaceType value("Value"_view);
  InterfaceType other("Other"_view);
  const Abstract* value_entry = &value;
  const Abstract* other_entry = &other;
  Layouts::Fluid value_layout({&value_entry, 1});
  Layouts::Fluid other_layout({&other_entry, 1});
  Layouts::Fluid empty;
  View::Bytes result_name = "value"_view;
  Layouts::Named named_value(
      value_layout, View::Vector<View::Bytes>(&result_name, 1));
  InterfaceCallable requirement("Requirement"_view, value_layout, value_layout);
  InterfaceCallable equivalent("Equivalent"_view, value_layout, value_layout);
  InterfaceCallable wrong_input("WrongInput"_view, other_layout, value_layout);
  InterfaceCallable wrong_result(
      "WrongResult"_view, value_layout, other_layout);
  InterfaceCallable empty_result("EmptyResult"_view, value_layout, empty);
  InterfaceCallable directional_requirement(
      "DirectionalRequirement"_view, empty, named_value);
  InterfaceCallable directional_candidate(
      "DirectionalCandidate"_view, empty, value_layout);
  EXPECT(
      ttx_callable_negotiate(requirement.get_abi(), equivalent.get_abi()) ==
      TTX_INTERFACE_EQUIVALENT);
  EXPECT(
      ttx_callable_negotiate(requirement.get_abi(), wrong_input.get_abi()) ==
      TTX_INTERFACE_REJECTED);
  EXPECT(
      ttx_callable_negotiate(requirement.get_abi(), wrong_result.get_abi()) ==
      TTX_INTERFACE_REJECTED);
  EXPECT(
      ttx_callable_negotiate(requirement.get_abi(), empty_result.get_abi()) ==
      TTX_INTERFACE_REJECTED);
  EXPECT(
      ttx_callable_negotiate(
          directional_requirement.get_abi(), directional_candidate.get_abi()) ==
      TTX_INTERFACE_SATISFIED);
  EXPECT(
      ttx_callable_negotiate(requirement.get_abi(), value.get_abi()) ==
      TTX_INTERFACE_REJECTED);
}
