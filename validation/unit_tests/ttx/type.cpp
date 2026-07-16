// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/type.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/core/static/vector.hpp"

#include "ttx/abstraction/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Abstraction;
using namespace Ttx::Model;
using namespace Ttx::Model::Layouts;
using namespace Validation;

class ResolvingType final : public Type {
 public:
  ResolvingType(
      View::Bytes name,
      const Invalid& invalid,
      Structured layout = Structured())
      : name(name), invalid(invalid), layout(layout) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto resolve() const -> const Abstract& override {
    if (complete_state) {
      return *this;
    }
    return invalid;
  }
  auto resolve_context(View::Bytes) const -> const Abstract& override {
    if (complete_state) {
      return *this;
    }
    return invalid;
  }
  auto get_layout() const -> const Structured& override { return layout; }

  auto complete() -> void { complete_state = True; }

 private:
  View::Bytes name;
  const Invalid& invalid;
  Structured layout;
  Bool complete_state = False;
};

class TypeField final : public Addressable {
 public:
  TypeField(View::Bytes name, const Abstract& type) : name(name), type(type) {}

  auto get_name() const -> View::Bytes override { return name; }
  auto resolve() const -> const Abstract& override { return type.resolve(); }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    return type.resolve().resolve_context(route);
  }

 private:
  View::Bytes name;
  const Abstract& type;
};

static Harness TtxType = {
  .name = "TTX::Type"_view,
};

PERIMORTEM_UNIT_TEST(TtxType, incomplete_system_resolves_to_invalid) {
  Invalid invalid;
  ResolvingType reserved("Reserved"_view, invalid);
  const Type& type = reserved;

  EXPECT(&type.resolve() == &invalid);

  reserved.complete();

  EXPECT(&type.resolve() == &reserved);
  EXPECT(type.get_layout().is_empty());
}

PERIMORTEM_UNIT_TEST(TtxType, structured_layout_keeps_real_field_objects) {
  Invalid invalid;
  ResolvingType real("Real_32"_view, invalid);
  real.complete();
  TypeField x("x"_view, real);
  TypeField y("y"_view, real);
  Static::Vector<const Addressable*, 2> fields = {{&x, &y}};
  ResolvingType point("Point"_view, invalid, Structured(fields));

  EXPECT(&point.resolve() == &invalid);

  point.complete();

  const Addressable& first = point.get_layout().get_abstract(0);
  EXPECT(&first == &x);
  EXPECT(&point.get_layout().get_abstract(1) == &y);
  EXPECT(&first.resolve() == &real);
}
