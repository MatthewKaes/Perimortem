// Perimortem Engine
// Copyright © Matt Kaes

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/alias.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/layouts/structured.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Ttx::Model::Layouts;
using namespace Validation;

/// A Type owns its stable shape while Layout only exposes ordered Abstracts.
class LayoutType final : public Type {
 public:
  LayoutType(View::Bytes name, Structured layout = Structured())
      : name(name), layout(layout) {}

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
  Structured layout;
};

/// Structured layouts retain fields as Addressable facts instead of copying
/// their names and Types into a parallel member model.
class LayoutField final : public Addressable {
 public:
  LayoutField(View::Bytes name, const Abstract& type)
      : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto get_documentation() const -> const Documentation& override {
    return Comment::get_empty();
  }
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

PERIMORTEM_UNIT_TEST(TtxLayout, fluid_order) {
  LayoutType real("Real_32"_view);
  LayoutType bits("Unsigned_32"_view);
  const Static::Vector<Reference<Abstract>, 2> values = {{real, bits}};
  Fluid layout(values);

  EXPECT_EQ(layout.get_size(), Count(2));
  EXPECT(&layout.get_abstract(0) == &real);
  EXPECT(&layout.get_abstract(1).resolve() == &bits);
  EXPECT(&layout.get_abstract(2) == &Invalid::get_invalid());
}

PERIMORTEM_UNIT_TEST(TtxLayout, structured_fields) {
  LayoutType real("Real_32"_view);
  LayoutType bits("Unsigned_32"_view);
  LayoutField x("x"_view, real);
  LayoutField y("y"_view, bits);
  const Static::Vector<Reference<Addressable>, 2> fields = {{x, y}};
  Structured layout(fields);

  EXPECT_EQ(layout.get_size(), Count(2));
  EXPECT(&layout.get_abstract(0) == &x);
  EXPECT_TEXT(layout.get_abstract(0).get_name(), "x"_view);
  EXPECT(&layout.get_abstract(0).resolve() == &real);
  EXPECT(&layout.get_abstract(1).resolve() == &bits);
  EXPECT(layout.get_abstract(2).is<Invalid>());
}

PERIMORTEM_UNIT_TEST(TtxLayout, fitting_contracts) {
  LayoutType real("Real_32"_view);
  LayoutType bits("Unsigned_32"_view);
  LayoutField x("x"_view, real);
  LayoutField y("y"_view, bits);
  Alias named_x("x"_view, real);
  Alias named_y("y"_view, bits);
  const Static::Vector<Reference<Addressable>, 2> fields = {{x, y}};
  const Static::Vector<Reference<Abstract>, 2> positional = {{real, bits}};
  const Static::Vector<Reference<Abstract>, 2> reordered = {{named_y, named_x}};
  Structured structured(fields);
  Fluid fluid(positional);
  Named named(reordered);

  EXPECT(fluid.fits(structured));
  EXPECT(named.fits(structured));
  EXPECT(structured.fits(structured));
  EXPECT(&fluid.get_fitted(structured, 0) == &real);
  EXPECT(&named.get_fitted(structured, 0) == &named_x);
  EXPECT(&named.get_fitted(structured, 1) == &named_y);
  EXPECT(&structured.get_fitted(structured, 1) == &y);
  EXPECT(fluid.get_fitted(structured, 2).is<Invalid>());
}

PERIMORTEM_UNIT_TEST(TtxLayout, named_ambiguity) {
  LayoutType real("Real_32"_view);
  LayoutType bits("Unsigned_32"_view);
  LayoutField x("x"_view, real);
  LayoutField y("y"_view, bits);
  Alias first("x"_view, real);
  Alias duplicate("x"_view, bits);
  const Static::Vector<Reference<Addressable>, 2> fields = {{x, y}};
  const Static::Vector<Reference<Abstract>, 2> values = {{first, duplicate}};
  Structured structured(fields);
  Named named(values);

  EXPECT_NOT(named.fits(structured));
  EXPECT(named.get_fitted(structured, 0).is<Invalid>());
}

PERIMORTEM_UNIT_TEST(TtxLayout, structured_identity) {
  LayoutType real("Real_32"_view);
  LayoutField first_x("x"_view, real);
  LayoutField second_x("x"_view, real);
  const Static::Vector<Reference<Addressable>, 1> first_fields = {{first_x}};
  const Static::Vector<Reference<Addressable>, 1> same_fields = {{first_x}};
  const Static::Vector<Reference<Addressable>, 1> other_fields = {{second_x}};
  Structured first(first_fields);
  Structured same(same_fields);
  Structured other(other_fields);

  EXPECT(first.fits(same));
  EXPECT_NOT(first.fits(other));
  EXPECT(first.get_fitted(other, 0).is<Invalid>());
}
