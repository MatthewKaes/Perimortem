// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/callables/self.hpp"
#include "ttx/model/callables/static.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Ttx::Model::Callables;
using namespace Ttx::Model::Layouts;
using namespace Validation;

/// A callable may refer to Types without making invocation a Type concern.
class CallableType final : public Type {
 public:
  CallableType(View::Bytes name) : name(name) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_layout() const -> const Structured& override { return layout; }

 private:
  View::Bytes name;
  inline static const Structured layout;
};

/// Static and Self callables share the same total signature contract.
class TestStatic final : public Ttx::Model::Callables::Static {
 public:
  TestStatic(View::Bytes name, const Layout& parameters, const Layout& results)
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

/// Self is distinguished by contract identity, not a hidden receiver rewrite.
class TestSelf final : public Self {
 public:
  TestSelf(View::Bytes name, const Layout& parameters, const Layout& results)
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

static Harness TtxCallable = {
  .name = "TTX::Callable"_view,
};

PERIMORTEM_UNIT_TEST(TtxCallable, callable_layouts) {
  CallableType counter("Counter"_view);
  CallableType count("Count"_view);
  Alias value("value"_view, count);
  Alias receiver("self"_view, counter);
  const Perimortem::Core::Static::Vector<Reference<Abstract>, 1> static_values =
      {{value}};
  const Perimortem::Core::Static::Vector<Reference<Abstract>, 1> self_values = {
    {receiver},
  };
  const Perimortem::Core::Static::Vector<Reference<Abstract>, 1> result_values =
      {{counter}};
  Named static_parameters(static_values);
  Named self_parameters(self_values);
  Fluid results(result_values);
  TestStatic static_callable("identity"_view, static_parameters, results);
  TestSelf self_callable("identity"_view, self_parameters, results);

  EXPECT_TEXT(static_callable.get_name(), self_callable.get_name());
  EXPECT_EQ(static_callable.get_parameters().get_size(), Count(1));
  EXPECT_EQ(self_callable.get_parameters().get_size(), Count(1));
  EXPECT_TEXT(
      self_callable.get_parameters().get_abstract(0).get_name(), "self"_view);
  EXPECT(&self_callable.get_parameters().get_abstract(0).resolve() == &counter);
  EXPECT(static_callable.is<Ttx::Model::Callables::Static>());
  EXPECT(static_callable.is<Callable>());
  EXPECT(static_callable.is<Abstract>());
  EXPECT_NOT(static_callable.is<Self>());
  EXPECT_NOT(static_callable.is<Type>());
  EXPECT(self_callable.is<Self>());
  EXPECT(self_callable.is<Callable>());
  EXPECT_NOT(self_callable.is<Ttx::Model::Callables::Static>());
  EXPECT(&static_callable.assume<Callable>() == &static_callable);
}
