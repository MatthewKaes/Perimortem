// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/model/type.hpp"

#include "validation/unit_test.hpp"

#include "ttx/abstraction/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Abstraction;
using namespace Ttx::Model;
using namespace Ttx::Model::Layouts;
using namespace Validation;

/// Registration may reserve a stable Type object before its facts are ready.
/// Resolution remains total by returning Invalid until completion.
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

/// A field is a real Addressable object retained by its owner's layout.
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

PERIMORTEM_UNIT_TEST(TtxType, incomplete_type) {
  Invalid invalid;
  ResolvingType reserved("Reserved"_view, invalid);
  const Type& type = reserved;

  EXPECT(&type.resolve() == &invalid);

  reserved.complete();

  EXPECT(&type.resolve() == &reserved);
  EXPECT(type.get_layout().is_empty());
}

PERIMORTEM_UNIT_TEST(TtxType, type_fields) {
  Invalid invalid;
  ResolvingType real("Real_32"_view, invalid);
  real.complete();
  TypeField x("x"_view, real);
  TypeField y("y"_view, real);
  const Reference<Addressable> fields[] = {x, y};
  ResolvingType point("Point"_view, invalid, Structured(fields));

  EXPECT(&point.resolve() == &invalid);

  point.complete();

  const Addressable& first =
      point.get_layout().get_abstract(0).as<Addressable>();
  EXPECT(&first == &x);
  EXPECT(&point.get_layout().get_abstract(1) == &y);
  EXPECT(&first.resolve() == &real);
}
