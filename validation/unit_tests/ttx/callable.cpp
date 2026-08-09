// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/callable.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Validation;

class CallableType : public Type {
 public:
  explicit CallableType(View::Bytes name) : name(name) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }

 private:
  View::Bytes name;
};

class TestCallable : public Callable {
 public:
  TestCallable(
      View::Bytes name,
      const Layout& parameters,
      const Layout& results)
      : name(name), parameters(parameters), results(results) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_parameters() const -> const Layout& override { return parameters; }
  auto get_results() const -> const Layout& override { return results; }

 private:
  View::Bytes name;
  const Layout& parameters;
  const Layout& results;
};

static Harness CallableTests = {
  .name = "Ttx::Model::Callable"_view,
};

PERIMORTEM_UNIT_TEST(CallableTests, complete_layouts) {
  CallableType count("Count"_view);
  CallableType accepted("Accepted"_view);
  const Static::Vector<Reference<const Abstract>, 1> parameter_entries = {
    {count},
  };
  const Static::Vector<Reference<const Abstract>, 1> result_entries = {
    {accepted},
  };
  Layouts::Fluid parameters(parameter_entries);
  Layouts::Named results(result_entries);
  TestCallable callable("classify"_view, parameters, results);

  EXPECT(callable.is<Callable>());
  EXPECT(callable.is<Abstract>());
  EXPECT_NOT(callable.is<Type>());
  EXPECT_EQ(callable.get_parameters().get_size(), Count(1));
  EXPECT_EQ(callable.get_results().get_size(), Count(1));
  EXPECT(callable.get_parameters().get_abstract(0).visit(
      []() { return False; },
      [&](const Abstract& selected) { return Bool(&selected == &count); }));
  EXPECT(callable.get_results().get_abstract(0).visit(
      []() { return False; },
      [&](const Abstract& selected) { return Bool(&selected == &accepted); }));
}
