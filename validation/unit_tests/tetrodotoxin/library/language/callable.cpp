// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/library/language/callables/self.hpp"
#include "tetrodotoxin/library/language/callables/static.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/layouts/structured.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;
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
  auto get_layout() const -> const Layout& override { return layout; }

 private:
  View::Bytes name;
  inline static const Layouts::Structured layout;
};

/// Static and Self callables share the same total signature contract.
class TestStatic final : public Language::Callables::Static {
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
class TestSelf final : public Language::Callables::Self {
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
  .name = "Ttx::Model::Callable"_view,
};

PERIMORTEM_UNIT_TEST(TtxCallable, callable_layouts) {
  CallableType counter("Counter"_view);
  CallableType count("Count"_view);
  Alias value("value"_view, count);
  Alias receiver("self"_view, counter);
  const Perimortem::Core::Static::Vector<Reference<const Abstract>, 1>
      static_values = {{value}};
  const Perimortem::Core::Static::Vector<Reference<const Abstract>, 1>
      self_values = {
        {receiver},
      };
  const Perimortem::Core::Static::Vector<Reference<const Abstract>, 1>
      result_values = {{counter}};
  Layouts::Named static_parameters(static_values);
  Layouts::Named self_parameters(self_values);
  Layouts::Fluid results(result_values);
  TestStatic static_callable("identity"_view, static_parameters, results);
  TestSelf self_callable("identity"_view, self_parameters, results);

  EXPECT_TEXT(static_callable.get_name(), self_callable.get_name());
  EXPECT_EQ(static_callable.get_parameters().get_size(), Count(1));
  EXPECT_EQ(self_callable.get_parameters().get_size(), Count(1));
  EXPECT(self_callable.get_parameters().get_abstract(0).visit(
      []() { return False; },
      [](const Abstract& parameter) {
        return parameter.get_name() == "self"_view ? True : False;
      }));
  EXPECT(self_callable.get_parameters().get_abstract(0).visit(
      []() { return False; },
      [&counter](const Abstract& parameter) {
        return &parameter.resolve() == &counter ? True : False;
      }));
  EXPECT(static_callable.is<Language::Callables::Static>());
  EXPECT(static_callable.is<Callable>());
  EXPECT(static_callable.is<Abstract>());
  EXPECT_NOT(static_callable.is<Language::Callables::Self>());
  EXPECT_NOT(static_callable.is<Type>());
  EXPECT(self_callable.is<Language::Callables::Self>());
  EXPECT(self_callable.is<Callable>());
  EXPECT_NOT(self_callable.is<Language::Callables::Static>());
  EXPECT(static_callable.visit<Callable>(
      [&](const Callable& callable) {
        return &callable == &static_callable ? True : False;
      },
      [](const Abstract&) { return False; }));
}
