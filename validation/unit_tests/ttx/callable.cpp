// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "ttx/concept/alias.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
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
    return Comment::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_layout() const -> const Structured& override { return layout; }

 private:
  View::Bytes name;
  inline static const Structured layout;
};

/// An address is an Abstract fact and may remain Invalid before linkage.
class CallableAddress final : public Addressable {
 public:
  CallableAddress(View::Bytes name) : name(name) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Comment::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }

 private:
  View::Bytes name;
};

/// Static and Self callables share the same total layout and address contract.
class TestStatic final : public Ttx::Model::Callables::Static {
 public:
  TestStatic(
      View::Bytes name,
      const Layout& parameters,
      const Layout& results,
      const Abstract& address)
      : name(name),
        parameters(parameters),
        results(results),
        address(address) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Comment::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_parameters() const -> const Layout& override { return parameters; }
  auto get_results() const -> const Layout& override { return results; }
  auto get_address() const -> const Abstract& override { return address; }

 private:
  View::Bytes name;
  const Layout& parameters;
  const Layout& results;
  const Abstract& address;
};

/// Self is distinguished by contract identity, not a hidden receiver rewrite.
class TestSelf final : public Self {
 public:
  TestSelf(
      View::Bytes name,
      const Layout& parameters,
      const Layout& results,
      const Abstract& address)
      : name(name),
        parameters(parameters),
        results(results),
        address(address) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Comment::get_empty();
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return Invalid::get_invalid();
  }
  auto get_parameters() const -> const Layout& override { return parameters; }
  auto get_results() const -> const Layout& override { return results; }
  auto get_address() const -> const Abstract& override { return address; }

 private:
  View::Bytes name;
  const Layout& parameters;
  const Layout& results;
  const Abstract& address;
};

static Harness TtxCallable = {
  .name = "TTX::Callable"_view,
};

PERIMORTEM_UNIT_TEST(TtxCallable, callable_layouts) {
  CallableType counter("Counter"_view);
  CallableType count("Count"_view);
  CallableAddress address("identity"_view);
  Alias value("value"_view, count);
  Alias receiver("self"_view, counter);
  const Reference<Abstract> static_values[] = {value};
  const Reference<Abstract> self_values[] = {receiver};
  const Reference<Abstract> result_values[] = {counter};
  Named static_parameters(static_values);
  Named self_parameters(self_values);
  Fluid results(result_values);
  TestStatic static_callable(
      "identity"_view, static_parameters, results, address);
  TestSelf self_callable("identity"_view, self_parameters, results, address);

  EXPECT_TEXT(static_callable.get_name(), self_callable.get_name());
  EXPECT_EQ(static_callable.get_parameters().get_size(), Count(1));
  EXPECT_EQ(self_callable.get_parameters().get_size(), Count(1));
  EXPECT_TEXT(
      self_callable.get_parameters().get_abstract(0).get_name(), "self"_view);
  EXPECT(&self_callable.get_parameters().get_abstract(0).resolve() == &counter);
  EXPECT(&static_callable.get_address() == &address);
  EXPECT(&self_callable.get_address() == &address);
  EXPECT(static_callable.is<Ttx::Model::Callables::Static>());
  EXPECT(static_callable.is<Callable>());
  EXPECT(static_callable.is<Abstract>());
  EXPECT_NOT(static_callable.is<Self>());
  EXPECT_NOT(static_callable.is<Type>());
  EXPECT(self_callable.is<Self>());
  EXPECT(self_callable.is<Callable>());
  EXPECT_NOT(self_callable.is<Ttx::Model::Callables::Static>());
  EXPECT(address.is<Addressable>());
  EXPECT(&static_callable.as<Callable>() == &static_callable);
}

PERIMORTEM_UNIT_TEST(TtxCallable, unresolved_address) {
  Fluid parameters;
  Fluid results;
  TestStatic unresolved(
      "load"_view, parameters, results, Invalid::get_invalid());

  EXPECT(&unresolved.get_address() == &Invalid::get_invalid());
}
