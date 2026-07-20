// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/invalid.hpp"
#include "ttx/model/binding.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/layouts/structured.hpp"
#include "ttx/model/projection.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Ttx::Model::Layouts;
using namespace Validation;

class FlowType final : public Type {
 public:
  FlowType(View::Bytes name, Structured layout = Structured())
      : name(name), layout(layout) {}

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
  Structured layout;
};

class FlowField final : public Addressable {
 public:
  FlowField(View::Bytes name, const Abstract& type) : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Abstract& override { return type; }

 private:
  View::Bytes name;
  const Abstract& type;
};

class FlowExpression final : public Expression {
 public:
  FlowExpression(View::Bytes name, const Abstract& type)
      : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Documentation::get_empty();
  }
  auto get_type() const -> const Abstract& override { return type; }
  auto get_inputs() const -> const Layout& override { return inputs; }

 private:
  View::Bytes name;
  const Abstract& type;
  inline static const Fluid inputs;
};

static Harness TtxFlow = {
  .name = "TTX::Flow"_view,
};

PERIMORTEM_UNIT_TEST(TtxFlow, projection) {
  FlowType real("Real"_view);
  FlowField r("r"_view, real);
  const Static::Vector<Reference<Addressable>, 1> fields = {{r}};
  FlowType color("Color"_view, Structured(fields));
  FlowExpression receiver("color"_view, color);
  Projection projection(receiver, r);
  FlowField missing("missing"_view, Invalid::get_invalid());
  Projection unresolved(receiver, missing);

  EXPECT(projection.is<Expression>());
  EXPECT(projection.is<Projection>());
  EXPECT(&projection.resolve() == &projection);
  EXPECT_TEXT(projection.get_name(), "r"_view);
  EXPECT(&projection.get_type() == &real);
  EXPECT(&projection.get_receiver() == &receiver);
  EXPECT(&projection.get_addressable() == &r);
  const Layout& inputs = projection.get_inputs();
  EXPECT_EQ(inputs.get_size(), Count(1));
  EXPECT(&inputs.get_abstract(0) == &receiver);
  EXPECT(inputs.get_abstract(1).is<Invalid>());
  EXPECT(projection.fits(real));
  EXPECT(unresolved.get_type().is<Invalid>());
  EXPECT_NOT(unresolved.fits(real));
}

PERIMORTEM_UNIT_TEST(TtxFlow, binding) {
  FlowType real("Real"_view);
  FlowExpression source("source"_view, real);
  Binding binding("x"_view, source);

  EXPECT(binding.is<Expression>());
  EXPECT(binding.is<Binding>());
  EXPECT(&binding.resolve() == &binding);
  EXPECT_TEXT(binding.get_name(), "x"_view);
  EXPECT(&binding.get_type() == &real);
  EXPECT(&binding.get_expression() == &source);
  const Layout& inputs = binding.get_inputs();
  EXPECT_EQ(inputs.get_size(), Count(1));
  EXPECT(&inputs.get_abstract(0) == &source);
  EXPECT(inputs.get_abstract(1).is<Invalid>());
  EXPECT(binding.fits(real));
}

PERIMORTEM_UNIT_TEST(TtxFlow, fluid_layout) {
  FlowType real("Real"_view);
  FlowExpression a("a"_view, real);
  FlowExpression b("b"_view, real);
  FlowExpression c("c"_view, real);
  const Static::Vector<Reference<Abstract>, 3> values = {{a, b, c}};
  Fluid flow(values);

  FlowField first("first"_view, real);
  FlowField second("second"_view, real);
  FlowField third("third"_view, real);
  const Static::Vector<Reference<Addressable>, 3> fields = {{
    first,
    second,
    third,
  }};
  Structured target(fields);

  EXPECT_EQ(flow.get_size(), Count(3));
  EXPECT(&flow.get_abstract(0) == &a);
  EXPECT(&flow.get_abstract(1) == &b);
  EXPECT(&flow.get_abstract(2) == &c);
  EXPECT(flow.get_abstract(3).is<Invalid>());
  EXPECT(flow.fits(target));
}

PERIMORTEM_UNIT_TEST(TtxFlow, named_layout) {
  FlowType real("Real"_view);
  FlowExpression first("first"_view, real);
  FlowExpression second("second"_view, real);
  Binding x("x"_view, first);
  Binding y("y"_view, second);
  const Static::Vector<Reference<Abstract>, 2> values = {{y, x}};
  Named named(values);

  FlowField x_field("x"_view, real);
  FlowField y_field("y"_view, real);
  const Static::Vector<Reference<Addressable>, 2> fields = {{x_field, y_field}};
  Structured target(fields);

  Binding duplicate("x"_view, second);
  const Static::Vector<Reference<Abstract>, 2> duplicates = {{x, duplicate}};
  Named ambiguous(duplicates);
  Binding unnamed({}, second);
  const Static::Vector<Reference<Abstract>, 2> empty_names = {{x, unnamed}};
  Named nameless(empty_names);

  EXPECT_EQ(named.get_size(), Count(2));
  EXPECT(&named.get_abstract(0) == &y);
  EXPECT(named.fits(target));
  EXPECT(&named.get_fitted(target, 0) == &x);
  EXPECT(&named.get_fitted(target, 1) == &y);
  EXPECT_NOT(ambiguous.fits(target));
  EXPECT_NOT(nameless.fits(target));
}
