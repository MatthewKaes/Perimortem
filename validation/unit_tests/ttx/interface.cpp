// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "validation/unit_test.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/interfaces/callable.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/type.hpp"

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
  TTX_CONSTEXPR_INVALID_CONTEXT;

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
  TTX_CONSTEXPR_INVALID_CONTEXT;

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
  Reference<const Abstract> value_entry(value);
  Reference<const Abstract> other_entry(other);
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
  Interfaces::Callable interface;

  EXPECT(
      interface.negotiate(requirement, equivalent) ==
      Interface::Relation::Equivalent);
  EXPECT(
      interface.negotiate(requirement, wrong_input) ==
      Interface::Relation::Rejected);
  EXPECT(
      interface.negotiate(requirement, wrong_result) ==
      Interface::Relation::Rejected);
  EXPECT(
      interface.negotiate(requirement, empty_result) ==
      Interface::Relation::Rejected);
  EXPECT(
      interface.negotiate(directional_requirement, directional_candidate) ==
      Interface::Relation::Satisfied);
  EXPECT_NOT(interface.accepts(requirement, value));
}
