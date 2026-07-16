// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/abstraction/alias.hpp"
#include "ttx/abstraction/invalid.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/layouts/structured.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Abstraction;
using namespace Ttx::Model;
using namespace Ttx::Model::Layouts;
using namespace Validation;

class LayoutType final : public Type {
 public:
  LayoutType(View::Bytes name, const Invalid& invalid)
      : name(name), invalid(invalid) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    return invalid;
  }
  auto get_layout() const -> const Structured& override { return layout; }

 private:
  View::Bytes name;
  const Invalid& invalid;
  Structured layout;
};

class LayoutField final : public Addressable {
 public:
  LayoutField(View::Bytes name, const Abstract& type)
      : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto resolve() const -> const Abstract& override { return type.resolve(); }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    return type.resolve().resolve_context(route);
  }

 private:
  View::Bytes name;
  const Abstract& type;
};

static Harness TtxLayout = {
  .name = "TTX::Layout"_view,
};

PERIMORTEM_UNIT_TEST(TtxLayout, fluid_is_ordered_abstract_flow) {
  Invalid invalid;
  LayoutType real("Real_32"_view, invalid);
  LayoutType bits("Bits_32"_view, invalid);
  Static::Vector<const Abstract*, 2> values = {{&real, &bits}};
  Fluid layout(values);

  EXPECT_EQ(layout.get_size(), Count(2));
  EXPECT(&layout.get_abstract(0) == &real);
  EXPECT(&layout.get_abstract(1).resolve() == &bits);
}

PERIMORTEM_UNIT_TEST(TtxLayout, structured_borrows_real_addressables) {
  Invalid invalid;
  LayoutType real("Real_32"_view, invalid);
  LayoutType bits("Bits_32"_view, invalid);
  LayoutField x("x"_view, real);
  LayoutField y("y"_view, bits);
  Static::Vector<const Addressable*, 2> fields = {{&x, &y}};
  Structured layout(fields);

  EXPECT_EQ(layout.get_size(), Count(2));
  EXPECT(&layout.get_abstract(0) == &x);
  EXPECT_TEXT(layout.get_abstract(0).get_name(), "x"_view);
  EXPECT(&layout.get_abstract(0).resolve() == &real);
  EXPECT(&layout.get_abstract(1).resolve() == &bits);
}

PERIMORTEM_UNIT_TEST(TtxLayout, layout_contracts_own_fitting) {
  Invalid invalid;
  LayoutType real("Real_32"_view, invalid);
  LayoutType bits("Bits_32"_view, invalid);
  LayoutField x("x"_view, real);
  LayoutField y("y"_view, bits);
  Alias named_x("x"_view, real);
  Alias named_y("y"_view, bits);
  Static::Vector<const Addressable*, 2> fields = {{&x, &y}};
  Static::Vector<const Abstract*, 2> positional = {{&real, &bits}};
  Static::Vector<const Abstract*, 2> reordered = {{&named_y, &named_x}};
  Structured structured(fields);
  Fluid fluid(positional);
  Named named(reordered);

  EXPECT(fluid.fits(structured));
  EXPECT(named.fits(structured));
  EXPECT(structured.fits(structured));
}

PERIMORTEM_UNIT_TEST(TtxLayout, named_fitting_rejects_ambiguous_facts) {
  Invalid invalid;
  LayoutType real("Real_32"_view, invalid);
  LayoutType bits("Bits_32"_view, invalid);
  LayoutField x("x"_view, real);
  LayoutField y("y"_view, bits);
  Alias first("x"_view, real);
  Alias duplicate("x"_view, bits);
  Static::Vector<const Addressable*, 2> fields = {{&x, &y}};
  Static::Vector<const Abstract*, 2> values = {{&first, &duplicate}};
  Structured structured(fields);
  Named named(values);

  EXPECT_NOT(named.fits(structured));
}

PERIMORTEM_UNIT_TEST(TtxLayout, structured_identity_is_not_shape_laundering) {
  Invalid invalid;
  LayoutType real("Real_32"_view, invalid);
  LayoutField first_x("x"_view, real);
  LayoutField second_x("x"_view, real);
  Static::Vector<const Addressable*, 1> first_fields = {{&first_x}};
  Static::Vector<const Addressable*, 1> same_fields = {{&first_x}};
  Static::Vector<const Addressable*, 1> other_fields = {{&second_x}};
  Structured first(first_fields);
  Structured same(same_fields);
  Structured other(other_fields);

  EXPECT(first.fits(same));
  EXPECT_NOT(first.fits(other));
}
